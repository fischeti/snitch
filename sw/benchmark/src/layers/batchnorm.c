// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "benchmark.h"
#include "layer.h"
#include "batchnorm.h"

void batchnorm_fp64(layer l) {

    uint32_t cluster_num = snrt_cluster_num();
    uint32_t cluster_id = snrt_cluster_idx();
    uint32_t compute_num = snrt_cluster_compute_core_num();
    uint32_t compute_id = snrt_cluster_compute_core_idx();

    // Each cluster loads one tile of a row
    uint32_t ifmap_size = 2 * l.IW * l.TILE_CI;
    uint32_t weights_size = l.CI;
    uint32_t ofmap_size = 2 * l.IW * l.TILE_CI;

    double *ptr = snrt_cluster_memory().start;
    double *ifmap = ptr;
    ptr += ifmap_size;
    double *gamma = ptr;
    ptr += weights_size;
    double *beta = ptr;
    ptr += weights_size;
    double *ofmap = ptr;
    ptr += ofmap_size;

    uint32_t read_buf = 0;
    uint32_t write_buf = 0;

    uint32_t prev_oh;
    uint32_t prev_ow;
    uint32_t prev_ci0;

    for (uint32_t oh = cluster_id; oh < l.OH; oh+=cluster_num) {
        for (uint32_t ci0 = 0; ci0 < l.CI; ci0+=l.TILE_CI) {

            if (snrt_is_dm_core()) {

                // Load weights once in the beginning
                if (oh == cluster_id && ci0 == 0) {
                    snrt_dma_start_1d(gamma, l.gamma, sizeof(double) * l.CI);
                    snrt_dma_start_1d(beta, l.beta, sizeof(double) * l.CI);
                    snrt_dma_wait_all();
                }

                // Load some stuff
                if (l.TILE_CI == l.CI) {
                    // data layout is consecutively in memory
                    snrt_dma_start_1d(&ifmap[write_buf * ifmap_size/2], &l.ifmap[oh * l.IW * l.CI], sizeof(double) * l.IW * l.TILE_CI);
                }
                else {
                    // data is interleaved
                    snrt_dma_start_2d(&ifmap[write_buf * ifmap_size/2], /* dst */
                                      &l.ifmap[oh * l.IW * l.CI + ci0],  /* src */
                                      sizeof(double) * l.TILE_CI,       /* size */
                                      sizeof(double) * l.TILE_CI,       /* dst_stride */
                                      sizeof(double) * l.CI,            /* src_stride */
                                      l.IW);                            /* repetitions */
                }

                snrt_dma_wait_all();

                snrt_cluster_barrier();


                if (!(oh == cluster_id && ci0 == 0)) {

                    if (l.TILE_CI == l.CI) {
                        // data is stored consecutively
                        snrt_dma_start_1d(&l.ofmap[prev_oh * l.OW * l.CI], &ofmap[!read_buf * (ofmap_size/2)], sizeof(double) * l.IW * l.CI);
                    }
                    else {
                        // data is stored in interleaved layout
                        snrt_dma_start_2d(&l.ofmap[prev_oh * l.OW * l.CI + prev_ci0], /* dst */
                                          &ofmap[!read_buf * (ofmap_size/2)],   /* src */
                                          sizeof(double) * l.TILE_CI,           /* size */
                                          sizeof(double) * l.CI,                /* dst_stride */
                                          sizeof(double) * l.TILE_CI,           /* src_stride */
                                          l.IW);                                /* repetitions */
                    }
                }

                snrt_dma_wait_all();
                write_buf = !write_buf;
                read_buf = !read_buf;
                prev_ci0 = ci0;
                prev_oh = oh;
                /* prev_ow = ow; */
            }

            if (snrt_is_compute_core()) {

                register volatile double ft0 asm("ft0");
                register volatile double ft1 asm("ft1");
                register volatile double ft2 asm("ft2");
                asm volatile("" : "=f"(ft0), "=f"(ft1), "=f"(ft2));

                // initially setup SSRs
                if (oh == cluster_id && ci0 == 0) {
                    uint32_t ssr_b[2] = {l.OW, l.TILE_CI/compute_num};
                    uint32_t ssr_i[2] = {l.TILE_CI * 8, compute_num * 8};

                    snrt_ssr_loop_2d(SNRT_SSR_DM0, ssr_b[0], ssr_b[1], ssr_i[0], ssr_i[1]);
                    snrt_ssr_loop_2d(SNRT_SSR_DM1, ssr_b[0], ssr_b[1], ssr_i[0], ssr_i[1]);
                }


                // Wait for data
                snrt_cluster_barrier();

                snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_2D, &ifmap[write_buf * ofmap_size/2 + compute_id]);
                snrt_ssr_write(SNRT_SSR_DM1, SNRT_SSR_2D, &ofmap[write_buf * ofmap_size/2 + compute_id]);
                snrt_ssr_enable();


                for (uint32_t ci1 = compute_id; ci1 < l.TILE_CI; ci1+=compute_num) {
                    register volatile double g asm("ft3") = gamma[ci0 + ci1];
                    register volatile double b asm("ft4") = beta[ci0 + ci1];
                    register volatile double result;
                    register const uint32_t rep asm("t0") = l.OW - 1;

                    // frep over OW dimension
                    asm volatile(
                                 ".word (0 << 20)|(5 << 15)|(1 << 7)|(0b0001011 << 0) \n"
                                 "fmadd.d ft1, ft0, %[g], %[b] \n"
                                 :
                                 : [ g ] "f"(g), [ b ] "f"(b), [ r ] "r"(rep)
                                 :"ft0", "ft1"
                                 );
                }

                write_buf = !write_buf;
                read_buf = !read_buf;

                asm volatile("" ::"f"(ft0), "f"(ft1), "f"(ft2));
                asm volatile("nop\n");
                asm volatile("nop\n");
                asm volatile("nop\n");
                asm volatile("nop\n");
                asm volatile("nop\n");
                asm volatile("nop\n");
                asm volatile("nop\n");


                snrt_ssr_disable();

            }
        }
    }

    snrt_cluster_barrier();

    //Clean up

    if (snrt_is_dm_core()) {

        if (l.TILE_CI == l.CI) {
            // data is stored consecutively
            snrt_dma_start_1d(&l.ofmap[prev_oh * l.OW * l.CI], &ofmap[!read_buf * (ofmap_size/2)], sizeof(double) * l.IW * l.CI);
        }
        else {
            // data is stored in interleaved layout
            snrt_dma_start_2d(&l.ofmap[prev_oh * l.OW * l.CI + prev_ci0], /* dst */
                              &ofmap[!read_buf * (ofmap_size/2)],   /* src */
                              sizeof(double) * l.TILE_CI,           /* size */
                              sizeof(double) * l.CI,                /* dst_stride */
                              sizeof(double) * l.TILE_CI,           /* src_stride */
                              l.IW);                                /* repetitions */
        }

        snrt_dma_wait_all();
    }
}
