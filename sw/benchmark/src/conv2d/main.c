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

    // Allocate buffer memory in the TCDM and generate input data.
    double* ptr = snrt_cluster_memory().start;
    double* ifmap = ptr;
    ptr += l.ci * l.ih * l.iw;
    double* weights = ptr;
    ptr += l.co * l.ci * l.fh * l.fw;
    double* ofmap = ptr;
    ptr += l.co * l.oh * l.ow;
    double* ofmap_GM = ptr;

    double* local_weights = weights + snrt_cluster_core_idx() * l.ci * l.fh * l.fw;
    double* local_ofmap = ofmap + snrt_cluster_core_idx() * l.oh * l.ow;

    if (snrt_is_dm_core()) {
        memset(ofmap,0,sizeof(double)*l.co*l.oh*l.ow/snrt_cluster_num());

        snrt_dma_start_1d(ifmap,
                          ifmap_dram,
                          sizeof(double)*l.ci*l.ih*l.iw);

        snrt_dma_start_1d(weights,
                          weights_dram + l.co * l.ci * l.fh * l.fw / snrt_cluster_num() * snrt_cluster_idx(),
                          sizeof(double)*l.co*l.ci*l.fh*l.fw / snrt_cluster_num());
        snrt_dma_wait_all();
        /* printf("finished transfer\n"); */
    }
    else {
        /* printf("waiting for end of transfer\n"); */
    }
    snrt_barrier();

    if (snrt_is_compute_core()) {
        benchmark_get_cycle();
        conv2d_ssr_frep_reordered(ifmap, local_ofmap, local_weights,
                                  /* l.co */1, l.ci, l.oh, l.ow, l.ih, l.iw, l.fh, l.fw);
        benchmark_get_cycle();
    }
    else {
        /* printf("waiting for end of computation\n"); */
    }
    snrt_barrier();

    if (snrt_is_dm_core()) {
        snrt_dma_start_1d(ofmap_GM,
                          ofmap_dram + l.co * l.oh * l.ow * snrt_cluster_idx() / snrt_cluster_num(),
                          sizeof(double)*l.co * l.oh * l.ow / snrt_cluster_num());
        snrt_dma_wait_all();

        uint32_t errors = 0;
        for (uint32_t i = 0; i < l.co * l.oh * l.ow / snrt_cluster_num(); i++) {
            /* if (ofmap_GM[i] != ofmap[i]) { */
                /* errors++; */
            /* } */
            if(fabs(ofmap_GM[i] - ofmap[i]) > 0.001) {
                errors++;
            }
        }
        if (errors == 0) {
            printf("No Errors\n");
        } else {
            printf("%d Errors\n");
        }

    }

    snrt_barrier();

    return 0;
}
