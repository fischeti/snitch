// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <math.h>
#include <string.h>
/* #include "snrt.h" */

#include "data.h"
#include "conv2d.h"

int main() {

    uint32_t cluster_num = snrt_cluster_num();
    uint32_t cluster_id = snrt_cluster_idx();

    struct cluster_memory_alloc {
        double ifmap[l.ih][l.iw][l.ci];
        double weights[l.co][l.ci][l.fh][l.fw];
        double ofmap[l.oh][l.ow][l.co/cluster_num];
        double ofmap_GM[l.oh][l.ow][l.co/cluster_num];
    };

    struct cluster_memory_alloc *mem = (void*)snrt_cluster_memory().start;

    if (snrt_is_dm_core()) {
        /* printf("ifmap %p, weights %p\n", &mem->ifmap, &mem->weights); */

        memset(&mem->ofmap, 0, sizeof(double)*OFMAP_SIZE);

        snrt_dma_start_1d(&mem->ifmap,
                          ifmap_dram,
                          sizeof(double)*IFMAP_SIZE);

        snrt_dma_start_1d(&mem->weights,
                          weights_dram + WEIGHTS_SIZE / cluster_num * cluster_id,
                          sizeof(double) * WEIGHTS_SIZE / cluster_num);
        snrt_dma_wait_all();
        snrt_set_perf_counter(0, 1);
        snrt_set_perf_counter(1, 2);
    }


    snrt_barrier();

    if (snrt_is_compute_core()) {
        /* benchmark_get_cycle(); */
        conv2d_hwc_ssr_frep((double*)&mem->ifmap, (double*)&mem->ofmap, (double*)&mem->weights,
                            l.co / cluster_num, l.ci, l.oh, l.ow, l.ih, l.iw, l.fh, l.fw);
        /* benchmark_get_cycle(); */
    }
    else {
        printf("waiting for end of computation\n");
    }
    snrt_barrier();

    if (snrt_is_dm_core()) {
        uint32_t tcdm_access = snrt_get_perf_counter(0);
        uint32_t tcdm_congestion = snrt_get_perf_counter(1);
        printf("TCDM Access/Congestion: %d/%d\n", tcdm_access, tcdm_congestion);

        snrt_dma_start_2d(&mem->ofmap_GM, /* dst */
                          ofmap_dram + l.co / cluster_num * cluster_id, /* src */
                          sizeof(double) * l.co / cluster_num, /* size */
                          sizeof(double) * l.co / cluster_num, /* dst_stride */
                          sizeof(double) * l.co,         /* src_stride */
                          l.oh * l.ow /* repetitions */
                          );
        snrt_dma_wait_all();

        uint32_t errors = 0;

        for (uint32_t co = 0; co < l.co/cluster_num; co++) {
            for (uint32_t oh = 0; oh < l.oh; oh++) {
                for (uint32_t ow = 0; ow < l.ow; ow++) {
                    if(fabs(mem->ofmap_GM[oh][ow][co] - mem->ofmap[oh][ow][co]) > 0.001) {
                        errors++;
                        /* printf("OH %d OW %d CO %d\n", oh, ow, co); */
                    }
                }
            }
        }
        if (errors == 0) {
            printf("No Errors\n");
        } else {
            printf("%d Errors\n", errors);
        }

    }

    snrt_barrier();

    return 0;
}
