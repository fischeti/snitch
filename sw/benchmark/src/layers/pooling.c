// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "benchmark.h"
#include "layer.h"
#include "pooling.h"

void max_pooling_fp64(layer l) {

    uint32_t cluster_num = snrt_cluster_num();
    uint32_t cluster_id = snrt_cluster_idx();
    uint32_t compute_num = snrt_cluster_compute_core_num();
    uint32_t compute_id = snrt_cluster_compute_core_idx();

    // Each cluster loads one tile of kernel size
    uint32_t ifmap_size = 2 * l.FH * l.FW * l.TILE_CI;
    uint32_t ofmap_size = 2 * l.TILE_CI;

    double *ptr = snrt_cluster_memory().start;
    double *ifmap = ptr;
    ptr += ifmap_size;
    double *ofmap = ptr;
    ptr += ofmap_size;

    uint32_t read_buf = 0;
    uint32_t write_buf = 0;

    uint32_t prev_oh;
    uint32_t prev_ow;
    uint32_t prev_ci0;

    // tiles are distributed across clusters
    for (uint32_t tile = cluster_id; tile < l.OH * l.OW; tile+=cluster_num) {

        for (uint32_t ci0 = 0; ci0 < l.CI; ci0+=l.TILE_CI) {

            uint32_t oh = tile / l.OW;
            uint32_t ow = tile % l.OW;

            if (snrt_is_dm_core()) {

                for (uint32_t fh = 0; fh < l.FH; fh++) {

                    snrt_dma_start_2d(&ifmap[write_buf * (ifmap_size/2) + fh * l.FW * l.TILE_CI], /* dst */
                                      &l.ifmap[((oh * l.FH + fh) * l.IW + ow * l.FW)*l.CI + ci0], /* src */
                                      sizeof(double) * l.TILE_CI, /* size */
                                      sizeof(double) * l.TILE_CI, /* dst_stride */
                                      sizeof(double) * l.CI, /* src_stride */
                                      l.FW /* repetitions */);
                }
                snrt_dma_wait_all();

                // synchronize with compute cores after loading data
                snrt_cluster_barrier();

                if (!(tile == cluster_id && ci0 == 0)) {

                    snrt_dma_start_2d(&l.ofmap[(prev_oh * l.OW + prev_ow)*l.CI + prev_ci0], /* dst */
                                      &ofmap[!read_buf * (ofmap_size/2)], /* src */
                                      sizeof(double) * l.TILE_CI, /* size */
                                      sizeof(double) * l.CI, /* dst_stride */
                                      sizeof(double) * l.TILE_CI, /* src_stride */
                                      1 /* repetitions */);


                }

                snrt_dma_wait_all();
                write_buf = !write_buf;
                read_buf = !read_buf;
                prev_ci0 = ci0;
                prev_oh = oh;
                prev_ow = ow;
            }

            if (snrt_is_compute_core()) {

                // wait for data to arrive
                snrt_cluster_barrier();

                for (uint32_t ci1 = compute_id; ci1 < l.TILE_CI; ci1+=compute_num) {

                    register volatile double max = ifmap[read_buf * ifmap_size/2 + ci1];
                    for (uint32_t fh = 0; fh < l.FH; fh++) {
                        for (uint32_t fw = 0; fw < l.FW; fw++) {
                            if (ifmap[((read_buf * l.FH + fh) * l.FW + fw)*l.TILE_CI + ci1] > max) {
                                max = ifmap[((read_buf * l.FH + fh) * l.FW + fw)*l.TILE_CI + ci1];
                            }
                        }
                    }
                    ofmap[write_buf * ofmap_size/2 + ci1] = max;
                }

                write_buf = !write_buf;
                read_buf = !read_buf;
            }

        }


    }

    snrt_cluster_barrier();

    if (snrt_is_dm_core()) {

        snrt_dma_start_2d(&l.ofmap[(prev_oh * l.OW + prev_ow)*l.CI + prev_ci0], /* dst */
                          &ofmap[!read_buf * (ofmap_size/2)], /* src */
                          sizeof(double) * l.TILE_CI, /* size */
                          sizeof(double) * l.CI, /* dst_stride */
                          sizeof(double) * l.TILE_CI, /* src_stride */
                          1 /* repetitions */);
    }

    snrt_dma_wait_all();
}