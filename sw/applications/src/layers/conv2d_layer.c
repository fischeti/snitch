// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "layer.h"
#include "conv2d_layer.h"
#include "gemm.h"
#include "utils.h"
#include "printf.h"
#include "snrt.h"

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

void conv2d_layer(layer l) {

    uint32_t cluster_num = snrt_cluster_num();
    uint32_t cluster_id = snrt_cluster_idx();
    uint32_t compute_num = snrt_cluster_compute_core_num();
    uint32_t compute_id = snrt_cluster_compute_core_idx();

    const uint32_t cluster_per_quadrant = min(4, cluster_num);

    // typedef struct cluster_mem_alloc_struct {
    //     double im2col[2][compute_num][l.FW*l.FH*l.TILE_CI+1];
    //     double ifmap[2][l.FH][compute_num + l.FW - 1][l.TILE_CI];
    //     double weights[compute_num][l.FH*l.FW*l.TILE_CI+1];
    //     double ofmap[2][compute_num][8];
    //     volatile uint32_t synch_flag[2];
    // } cluster_mem_alloc;

    // im2col[2][compute_num][l.FW*l.FH*l.TILE_CI+1];
    uint32_t im2col_row_stride = l.FW*l.FH*l.TILE_CI+1;
    uint32_t im2col_mat_stride = im2col_row_stride * compute_num;
    uint32_t im2col_size = 2 * im2col_mat_stride;

    // ifmap[2][l.FH][compute_num + l.FW - 1][l.TILE_CI];
    uint32_t ifmap_col_stride = l.TILE_CI;
    uint32_t ifmap_row_stride = ifmap_col_stride * (compute_num + l.FW - 1);
    uint32_t ifmap_stride = ifmap_row_stride * l.FH;
    uint32_t ifmap_size = 2 * ifmap_stride;

    // weights[compute_num][l.FH*l.FW*l.TILE_CI+1];
    uint32_t weights_co_stride = l.FH*l.FW*l.TILE_CI+1;
    uint32_t weights_size = compute_num * weights_co_stride;

    // ofmap[2][compute_num][8];
    uint32_t ofmap_co_stride = 8;
    uint32_t ofmap_stride = compute_num * ofmap_co_stride;
    uint32_t ofmap_size = 2 * ofmap_stride;


    void *ptr = (void *)snrt_cluster_memory().start;
    void *im2col = ptr;
    ptr += im2col_size * l.dtype;
    void *ifmap = ptr;
    ptr += ifmap_size * l.dtype;
    void *weights = ptr;
    ptr += weights_size * l.dtype;
    void *ofmap = ptr;
    ptr += ofmap_size * l.dtype;
    uint32_t *synch_flag = (void*)ptr;

    printf("im2col %p ifmap %p weights %p ofmap %p\n", im2col, ifmap, weights, ofmap);

    uint32_t write_buf = 0;
    uint32_t read_buf = 0;

    int32_t oh_prev = -1;
    int32_t ow_prev = -1;

    snrt_global_barrier();

    // Distribute output channels across clusters
    for (uint32_t co = cluster_id*compute_num; co < l.CO; co+=cluster_num*compute_num){

        // Tile CI dimension
        for (uint32_t ci = 0; ci < l.CI; ci+= l.TILE_CI) {

            // Load weights in the beginning
            if (snrt_is_dm_core()) {

                // Weights are stored in CO x FH x FW x CI format with additional padding
                // (CI + 1) to prevent banking conflicts
                for (uint32_t _co = 0; _co < 8; _co++) {

                    printf("transfer %x bytes from %p to %p\n", l.dtype*l.CI*l.FH*l.FW, &l.weights[(co+_co)*l.FH*l.FW*l.CI], &weights[_co * weights_co_stride * l.dtype]);

                    if (l.TILE_CI == l.CI) {
                        snrt_dma_txid_t weight_txid = \
                            snrt_dma_start_1d(&weights[_co * weights_co_stride * l.dtype], /* dst */
                                              &l.weights[(co+_co)*l.FH*l.FW*l.CI * l.dtype], /* src */
                                              l.dtype*l.CI*l.FH*l.FW /* size */);
                    }
                    else {
                        snrt_dma_txid_t weight_txid = \
                            snrt_dma_start_2d(&weights[_co * weights_co_stride * l.dtype], /* dst */
                                              &l.weights[((co+_co)*l.FH*l.FW*l.CI + ci) * l.dtype], /* src */
                                              l.dtype*l.TILE_CI, /* size */
                                              l.dtype*l.TILE_CI, /* dst_stride */
                                              l.dtype*l.CI, /* src_stride */
                                              l.FH*l.FW /* repetitions */);
                    }
                }
                volatile float bubu1 = ((float*)weights)[0];
                volatile float bubu2 = ((float*)weights)[1];
                volatile float bubu5 = ((float*)weights)[288];
                volatile float bubu3 = ((float*)weights)[289];
                volatile float bubu4 = ((float*)weights)[290];
                snrt_dma_wait_all();
            }

            // Iterate over pixels, outer loop iterates over tiles of columns in feature map,
            // inner loop iterates over rows. Each core processes one pixel at a time.
            // In case of cluster2cluster communication, each cluster in a quadrant starts with a different row.
            // The first time, all clusters load a different row from memory. In each subsequent iteration
            // the leading cluster loads a new row from main memory and the others load from the next cluster
            for(uint32_t ow = 0; ow < l.OW; ow+=compute_num) {

                if (l.cluster2cluster) {
                    synch_flag[0] = 0;
                    synch_flag[1] = 0;
                }

                for (uint32_t _oh = 0; _oh < l.OH; _oh++) {

                    // If cluster2cluster is enabled, each cluster starts with a different row,
                    // requires that OH is bigger than cluster_num (per quadrant at least)
                    uint32_t oh = ((cluster_per_quadrant - 1) - (cluster_id % cluster_per_quadrant) + _oh) % l.OH;


                    if (snrt_is_dm_core()) {


                        uint32_t n_ifmap_pixel_read = min(compute_num + l.FW - 1, l.IW - ow + (l.pad<<1));
                        uint32_t n_ofmap_pixel_read = min(compute_num, l.OW - ow);
                        uint32_t n_ofmap_pixel_write = min(compute_num, l.OW - ow_prev);

                        // Load the intermediate outputs from memory
                        if (ci != 0) {
                            snrt_dma_txid_t ofmap_txid = \
                                snrt_dma_start_2d(&ofmap[write_buf * ofmap_stride * l.dtype], /* dst */
                                                  &l.ofmap[((oh*l.OW+ow)*l.CO + co) * l.dtype], /* src */
                                                  l.dtype*8, /* size */
                                                  l.dtype*8, /* dst_stride */
                                                  l.dtype*l.CO, /* src_stride */
                                                  n_ofmap_pixel_read); /* repetitions */
                            snrt_dma_wait_all();
                        }


                        if (l.cluster2cluster) {
                            // All except last cluster need to wait until
                            // cluster synch flag is cleared
                            if (cluster_id % cluster_per_quadrant != cluster_per_quadrant - 1) {
                                while (synch_flag[write_buf * l.dtype]);
                            }
                        }

                        // The input feature map needs to be loaded from main memory in the following cases:
                        // 1) cluster2cluster communication is not enabled
                        // 2) The first iteration, every cluster loads a row from main memory
                        // 3) The leading cluster always loads rows from main memory
                        if (!l.cluster2cluster || _oh == 0 || cluster_id % cluster_per_quadrant == 0) {

                            // Transfer in FH * (compute_num + FW - 1) pixels such that
                            // im2col transformation can be performed for every core

                            for (uint32_t fh = 0; fh < l.FH; fh++) {

                                // Fill horizontal lines with zeros for padding
                                if (oh + fh < l.pad || oh + fh >= l.IH + ((l.FH - 1)>>1)) {
                                    dma_memset(&ifmap[(write_buf * ifmap_stride + fh * ifmap_row_stride) * l.dtype], 0, l.dtype*l.TILE_CI*n_ifmap_pixel_read);
                                }
                                else {
                                    uint32_t padding_left = (ow < l.pad)? (l.pad - ow) : 0;
                                    uint32_t padding_right = (ow + compute_num + l.pad <= l.OW )? 0: n_ifmap_pixel_read - ((l.FW-1)>>1) - (l.IW - ow);

                                    // If there is need for padding, set whole buffer to zero
                                    if (padding_left || padding_right) {
                                        dma_memset(&ifmap[(write_buf * ifmap_stride + fh * ifmap_row_stride) * l.dtype], 0, l.dtype*(compute_num + l.FW - 1)*l.TILE_CI);
                                    }

                                    // Then fill in the rest of the values
                                    snrt_dma_txid_t ifmap_txid = \
                                        snrt_dma_start_2d(&ifmap[(write_buf * ifmap_stride + fh * ifmap_row_stride + padding_left * ifmap_col_stride) * l.dtype], /* dst */
                                                          (double*)&l.ifmap[(((oh + fh - l.pad)*l.IW + ow - (l.pad - padding_left))*l.CI + ci) * l.dtype], /* src */
                                                          l.dtype*l.TILE_CI, /* size */
                                                          l.dtype*l.TILE_CI, /* dst_stride */
                                                          l.dtype*l.CI, /* src_stride */
                                                          n_ifmap_pixel_read - padding_left - padding_right/* n_ifmap_pixel_read *//* repetitions */);
                                    snrt_dma_wait_all();
                                }
                            }


                        }

                        // Transfer tile from other cluster to memory
                        else {

                            // A cluster always loads from the previous cluster
                            uint32_t cluster_offset =  - (cluster_id - 1) * 0x00040000;
                            uint32_t *src_synch_flag = synch_flag + cluster_offset;
                            double *src_ifmap = ifmap + cluster_offset;

                            // Wait until previous cluster has released data
                            if (l.cluster2cluster && (cluster_id % cluster_per_quadrant) != 0) {
                                while(src_synch_flag[!write_buf] == 0);
                            }

                            // Transfer in FH * (compute_num + FW - 1) pixels such that
                            // im2col transformation can be performed for every core
                            snrt_dma_txid_t ifmap_txid = \
                                snrt_dma_start_1d(&ifmap[write_buf * ifmap_stride],
                                                  &src_ifmap[!write_buf * ifmap_stride],
                                                  l.dtype*n_ifmap_pixel_read*l.TILE_CI*l.FH);
                            snrt_dma_wait_all();

                            // clear synch flag of src cluster
                            if (l.cluster2cluster && (cluster_id % cluster_per_quadrant) != 0) {
                                src_synch_flag[(!write_buf) * l.dtype] = 0;
                            }

                        }

                        // New data is produced
                        if (l.cluster2cluster) {
                            synch_flag[write_buf * l.dtype] = 1;
                        }


                        // Reshuffle and write data to the im2col buffer by the DMA
                        for (uint32_t n = 0; n < compute_num; n++) {

                            // only construct im2col matrix for leftover pixels
                            if (ow + n < l.OW) {

                                snrt_dma_txid_t im2col_txid = \
                                    snrt_dma_start_2d(&im2col[(write_buf * im2col_mat_stride + n * im2col_row_stride) * l.dtype], /* dst */
                                                      &ifmap[(read_buf * ifmap_stride + n * ifmap_col_stride) * l.dtype], /* src */
                                                      l.dtype*l.FW*l.TILE_CI, /* size */
                                                      l.dtype*l.FW*l.TILE_CI, /* dst_stride */
                                                      l.dtype*(compute_num + l.FW - 1)*l.TILE_CI, /* src_stride */
                                                      l.FH /* repetitions */);
                            }
                        }

                        // Wait for im2col transform to end, and synchronize with compute cores
                        snrt_dma_wait_all();
                        snrt_cluster_sw_barrier();

                        // Transfer back the output feature maps
                        if (oh_prev + ow_prev >= 0) {

                            // printf("transfering %p to %p\n", &ofmap[!read_buf * ofmap_stride], &l.ofmap[(oh_prev*l.OW+ow_prev)*l.CO+co]);

                            snrt_dma_txid_t ofmap_txid = \
                                snrt_dma_start_2d(&l.ofmap[((oh_prev*l.OW+ow_prev)*l.CO + co) * l.dtype], /* dst */
                                                  &ofmap[(!read_buf * ofmap_stride) * l.dtype], /* src */
                                                  l.dtype*8, /* size */
                                                  l.dtype*l.CO, /* dst_stride */
                                                  l.dtype*8, /* src_stride */
                                                  n_ofmap_pixel_write); /* repetitions */
                            snrt_dma_wait_all();

                        }
                        oh_prev = oh;
                        ow_prev = ow;

                        // Toggle write and read buffer
                        write_buf = !write_buf;
                        read_buf = !read_buf;

                    }

                    if (snrt_is_compute_core()) {

                        // Wait until DMA core has finished the im2col transform
                        snrt_cluster_sw_barrier();

                        // Each core performs a matrix multiplication on the im2col buffer
                        // Of size (1 x FHxFWxCI) x (FHxFWxCI x 8), 8 represents CO and is the
                        // unrolling factor needed to prevent RAW conflicts.
                        if (ow + compute_id < l.OW) {

                            uint32_t setup_SSR = (ci == 0 && ow == 0 && _oh == 0)? 1 : 0;

                            if (ci != 0 && l.TILE_CI != l.CI) {
                                gemm_fp32simd_mac_tb_ssr_frep(1, 8, l.FH*l.FW*l.TILE_CI,
                                                 &im2col[(read_buf * im2col_mat_stride + compute_id * im2col_row_stride) * l.dtype], 0,
                                                 weights, l.FH*l.FW*l.TILE_CI+1,
                                                 &ofmap[(write_buf * ofmap_stride + compute_id * ofmap_co_stride) * l.dtype], 0, 1.0, setup_SSR);

                            }
                            else {
                                gemm_fp32simd_mac_tb_ssr_frep(1, 8, l.FH*l.FW*l.TILE_CI,
                                                 &im2col[(read_buf * im2col_mat_stride + compute_id * im2col_row_stride) * l.dtype], 0,
                                                 weights, l.FH*l.FW*l.TILE_CI+1,
                                                 &ofmap[(write_buf * ofmap_stride + compute_id * ofmap_co_stride) * l.dtype], 0, 0.0, setup_SSR);

                            }

                        }
                        // Toggle read and write buffer
                        read_buf = !read_buf;
                        write_buf = !write_buf;
                    }
                }
            }

            snrt_cluster_sw_barrier();


            // Transfer back last output tile
            if (snrt_is_dm_core()) {

                // printf("transfering %p to %p\n", &ofmap[!read_buf * ofmap_stride], &l.ofmap[(oh_prev*l.OW+ow_prev)*l.CO+co]);

                snrt_dma_txid_t ofmap_txid = \
                    snrt_dma_start_2d(&l.ofmap[((oh_prev*l.OW+ow_prev)*l.CO+co) * l.dtype], /* dst */
                                      &ofmap[(!read_buf * ofmap_stride) * l.dtype], /* src */
                                      l.dtype*8, /* size */
                                      l.dtype*l.CO, /* dst_stride */
                                      l.dtype*8, /* src_stride */
                                      min(compute_num, l.OW - ow_prev)); /* repetitions */
                snrt_dma_wait_all();
            }
        }
    }

    snrt_global_barrier();

}
