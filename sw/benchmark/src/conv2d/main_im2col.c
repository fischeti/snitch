// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
/* #include "im2col.h" */
#include "benchmark.h"
#include "data.h"
#include "layer.h"

#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

int main () {

    uint32_t cluster_num = snrt_cluster_num();
    uint32_t cluster_id = snrt_cluster_idx();
    uint32_t compute_num = snrt_cluster_compute_core_num();
    uint32_t compute_id = snrt_cluster_compute_core_idx();


    layer l = {
        .CO = CONV2D_CO,
        .CI = CONV2D_CI,
        .IH = CONV2D_IH,
        .IW = CONV2D_IW,
        .OH = CONV2D_OH,
        .OW = CONV2D_OW,
        .FH = CONV2D_FH,
        .FW = CONV2D_FW,
        .ifmap = (double*)ifmap_dram,
        .weights = (double*)weights0_dram,
        .ofmap = (double*)result,
        .cluster2cluster = 0,
        .pad = CONV2D_PAD,
        .TILE_CI = 32
    };

    typedef struct cluster_mem_alloc_struct {
        double im2col[2][compute_num][l.FW*l.FH*l.TILE_CI+1];
        double ifmap[2][l.FH][compute_num + l.FW - 1][l.TILE_CI];
        double weights[compute_num][l.FH*l.FW*l.TILE_CI+1];
        double result [2][compute_num][8];
    } cluster_mem_alloc;

    cluster_mem_alloc *mem = (void*)snrt_cluster_memory().start;

    uint32_t write_buf = 0;
    uint32_t read_buf = 0;

    int32_t oh_prev = -1;
    int32_t ow_prev = -1;

    /* snrt_barrier(); */

    // Distribute output channels across clusters
    for (uint32_t co = cluster_id*compute_num; co < l.CO; co+=cluster_num*compute_num){

        // Tile CI dimension
        for (uint32_t ci = 0; ci < l.CI; ci+= l.TILE_CI) {

            // Load weights in the beginning
            if (snrt_is_dm_core()) {

                /* printf("result %p-%p\n", result, &result[3][]); */

                // Weights are stored in CO x FH x FW x CI format with additional padding
                // (CI + 1) to prevent banking conflicts
                for (uint32_t _co = 0; _co < 8; _co++) {
                    snrt_dma_txid_t weight_txid = snrt_dma_start_2d(&mem->weights[_co], /* dst */
                                                                    &l.weights[(co+_co)*l.FH*l.FW*l.CI + ci], /* src */
                                                                    sizeof(double)*l.TILE_CI, /* size */
                                                                    sizeof(double)*l.TILE_CI, /* dst_stride */
                                                                    sizeof(double)*l.CI, /* src_stride */
                                                                    l.FH*l.FW /* repetitions */);
                }
                snrt_dma_wait_all();
            }

            // Iterate over pixels, outer loop iterates over tiles of columns in feature map,
            // inner loop iterates over rows. Each core processes one pixel at a time.
            // In case of cluster2cluster communication, each cluster in a quadrant starts with a different row.
            // The first time, all clusters load a different row from memory. In each subsequent iteration
            // the leading cluster loads a new row from main memory and the others load from the next cluster
            for(uint32_t ow = 0; ow < l.OW; ow+=compute_num) {

                for (uint32_t _oh = 0; _oh < l.OH; _oh++) {


                    // If cluster2cluster is enabled, each cluster starts with a different row,
                    // requires that OH is bigger than cluster_num (per quadrant at least)
                    /* uint32_t oh = (l.cluster2cluster)? (cluster_id + _oh) % l.OH : _oh; */
                    uint32_t oh = (cluster_id + _oh) % l.OH;

                    if (snrt_is_dm_core()) {

                        uint32_t n_ifmap_pixel_read = min(compute_num + l.FW - 1, l.IW - ow + (l.pad<<1));
                        uint32_t n_ofmap_pixel_read = min(compute_num, l.OW - ow);
                        uint32_t n_ofmap_pixel_write = min(compute_num, l.OW - ow_prev);

                        // Load the intermediate outputs from memory
                        if (ci != 0) {
                            snrt_dma_txid_t ofmap_txid = snrt_dma_start_2d(&mem->result[write_buf], /* dst */
                                                                           &l.ofmap[(oh*l.OW+ow)*l.CO + co], /* src */
                                                                           sizeof(double)*8, /* size */
                                                                           sizeof(double)*8, /* dst_stride */
                                                                           sizeof(double)*l.CO, /* src_stride */
                                                                           n_ofmap_pixel_read); /* repetitions */
                            snrt_dma_wait_all();
                        }

                        // The input feature map needs to be loaded from main memory in the following cases:
                        // 1) cluster2cluster communication is not enabled
                        // 2) The first iteration, every cluster loads a row from main memory
                        // 3) The leading cluster always loads rows from main memory
                        if (!l.cluster2cluster || _oh == 0 || cluster_id == 0) {

                            // Transfer in FH * (compute_num + FW - 1) pixels such that
                            // im2col transformation can be performed for every core

                            for (uint32_t fh = 0; fh < l.FH; fh++) {

                                // Fill horizontal lines with zeros for padding
                                if (oh + fh < l.pad || oh + fh >= l.IH + ((l.FH - 1)>>1)) {
                                    snrt_dma_memset((void*)&mem->ifmap[write_buf][fh], 0, sizeof(double)*l.TILE_CI*n_ifmap_pixel_read);
                                }
                                else {
                                    uint32_t padding_left = (ow < l.pad)? (l.pad - ow) : 0;
                                    uint32_t padding_right = (ow + compute_num + l.pad <= l.OW )? 0: n_ifmap_pixel_read - ((l.FW-1)>>1) - (l.IW - ow);

                                    // If there is need for padding, set whole buffer to zero
                                    if (padding_left || padding_right) {
                                        snrt_dma_memset((void*)&mem->ifmap[write_buf][fh], 0, sizeof(double)*(compute_num + l.FW - 1)*l.TILE_CI);
                                    }

                                    // Then fill in the rest of the values
                                    snrt_dma_txid_t ifmap_txid = snrt_dma_start_2d(&mem->ifmap[write_buf][fh][padding_left], /* dst */
                                                                                   (double*)&l.ifmap[((oh + fh - l.pad)*l.IW + ow - (l.pad - padding_left))*l.CI + ci], /* src */
                                                                                   sizeof(double)*l.TILE_CI, /* size */
                                                                                   sizeof(double)*l.TILE_CI, /* dst_stride */
                                                                                   sizeof(double)*l.CI, /* src_stride */
                                                                                   n_ifmap_pixel_read - padding_left - padding_right/* n_ifmap_pixel_read *//* repetitions */);
                                    snrt_dma_wait_all();
                                }
                            }
                        }

                        // Transfer tile from other cluster to memory
                        else {

                            // A cluster allways loads from the previous cluster
                            uint32_t src_cluster_id = cluster_id - 1;
                            cluster_mem_alloc *mem_src = (void*)mem - (cluster_id - src_cluster_id) * 0x00040000;


                            // Transfer in FH * (compute_num + FW - 1) pixels such that
                            // im2col transformation can be performed for every core
                            snrt_dma_txid_t ifmap_txid = snrt_dma_start_1d(&mem->ifmap[write_buf], &mem_src->ifmap[!write_buf], sizeof(double)*n_ifmap_pixel_read*l.TILE_CI*l.FH);
                        }
                        snrt_dma_wait_all();


                        // Reshuffle and write data to the im2col buffer by the DMA
                        for (uint32_t n = 0; n < compute_num; n++) {

                            // only construct im2col matrix for leftover pixels
                            if (ow + n < l.OW) {

                                snrt_dma_txid_t im2col_txid = snrt_dma_start_2d((double*)&mem->im2col[write_buf][n], /* dst */
                                                                                &mem->ifmap[read_buf][0][n], /* src */
                                                                                sizeof(double)*l.FW*l.TILE_CI, /* size */
                                                                                sizeof(double)*l.FW*l.TILE_CI, /* dst_stride */
                                                                                sizeof(double)*(compute_num + l.FW - 1)*l.TILE_CI, /* src_stride */
                                                                                l.FH /* repetitions */);
                            }
                        }

                        // Wait for im2col transform to end, and synchronize with compute cores
                        snrt_dma_wait_all();
                        snrt_cluster_barrier();


                        // Transfer back the output feature maps

                        /* printf("write to memory oh %d ow %d n_pixel %d\n", oh, ow, n_pixel_write); */

                        if (oh_prev + ow_prev >= 0) {

                            /* printf("Writing back result (%d pixels) oh %d ow %d from %p->%p\n", n_ofmap_pixel_write, oh_prev, ow_prev, &mem->result[!read_buf], &l.ofmap[(oh_prev*l.OW+ow_prev)*l.CO + co]); */
                            snrt_dma_txid_t ofmap_txid = snrt_dma_start_2d(&l.ofmap[(oh_prev*l.OW+ow_prev)*l.CO + co], /* dst */
                                                                           &mem->result[!read_buf], /* src */
                                                                           sizeof(double)*8, /* size */
                                                                           sizeof(double)*l.CO, /* dst_stride */
                                                                           sizeof(double)*8, /* src_stride */
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

                        /* printf("core %d/%d cluster %d/%d\n", compute_id, compute_num, cluster_id, cluster_num); */

                        // Wait until DMA core has finished the im2col transform
                        snrt_cluster_barrier();

                        // Each core performs a matrix multiplication on the im2col buffer
                        // Of size (1 x FHxFWxCI) x (FHxFWxCI x 8), 8 represents CO and is the
                        // unrolling factor needed to prevent RAW conflicts.
                        if (ow + compute_id < l.OW) {

                            uint32_t setup_SSR = (ci == 0 && ow == 0 && _oh == 0)? 1 : 0;

                            if (ci != 0 && l.TILE_CI != l.CI) {
                                gemm_tb_ssr_frep(1, 8, l.FH*l.FW*l.TILE_CI,
                                                 (double*)&mem->im2col[read_buf][compute_id], 0,
                                                 &mem->weights, l.FH*l.FW*l.TILE_CI+1,
                                                 &mem->result[write_buf][compute_id], 0, 1.0, setup_SSR);

                            }
                            else {
                                gemm_tb_ssr_frep(1, 8, l.FH*l.FW*l.TILE_CI,
                                                 (double*)&mem->im2col[read_buf][compute_id], 0,
                                                 &mem->weights, l.FH*l.FW*l.TILE_CI+1,
                                                 &mem->result[write_buf][compute_id], 0, 0.0, setup_SSR);

                            }

                        }
                        // Toggle read and write buffer
                        read_buf = !read_buf;
                        write_buf = !write_buf;
                    }
                }
            }

            snrt_cluster_barrier();


            // Transfer back last output tile
            if (snrt_is_dm_core()) {

                /* printf("transfering last tile\n"); */

                snrt_dma_txid_t ofmap_txid = snrt_dma_start_2d(&l.ofmap[(oh_prev*l.OW+ow_prev)*l.CO+co], /* dst */
                                                               &mem->result[!read_buf], /* src */
                                                               sizeof(double)*8, /* size */
                                                               sizeof(double)*l.CO, /* dst_stride */
                                                               sizeof(double)*8, /* src_stride */
                                                               min(compute_num, l.OW - ow_prev)); /* repetitions */
                snrt_dma_wait_all();
            }
        }

    }

    snrt_barrier();

    // DMA Core compares result with a precomputed checksum
    if (snrt_is_dm_core() && cluster_id == 0) {

        /* printf("hoi\n"); */
        volatile double result_buf[l.CO];
        volatile double ofmap_checksums[l.OH][l.OW];
        uint32_t errors = 0;
        uint32_t total = 0;

        snrt_dma_txid_t ofmap_checksum_txid = snrt_dma_start_1d((double*)ofmap_checksums, checksum, sizeof(checksum));
        snrt_dma_wait_all();

        for (uint32_t oh = 0; oh < l.OH; oh++) {
            for (uint32_t ow = 0; ow < l.OW; ow++) {
                snrt_dma_txid_t result_txid = snrt_dma_start_1d((double*)result_buf, &l.ofmap[(oh*l.OW + ow)*l.CO], sizeof(double)*l.CO);
                snrt_dma_wait_all();

                double checksum_result = 0.0;
                for (uint32_t co = 0; co < l.CO; co++) {
                    checksum_result += result_buf[co];
                }
                total++;
                if (fabs(checksum_result - ofmap_checksums[oh][ow]) > 0.001) {
                    errors++;
                }
            }
        }

        printf("%d/%d Errors\n", errors, total);
    }

    snrt_barrier();

    return 0;
}
