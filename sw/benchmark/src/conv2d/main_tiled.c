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

    // core/cluster identification (make code more appealing)
    uint32_t cluster_idx = snrt_cluster_idx();
    uint32_t cluster_num = snrt_cluster_num();
    int32_t cluster_tcdm_offset = 0x40000;

    // input channels are split temporally to share data between clusters
    uint32_t tiling_factor = 2;

    // Allocate buffer memory in the TCDM and generate input data.
    /* double* ptr = snrt_cluster_memory().start; */
    double* ptr = (void*)(0x10000000 + cluster_tcdm_offset * cluster_idx);
    double* ifmap = ptr;
    // tiling_factor: split input channels, 2: double buffer
    ptr += IFMAP_SIZE / tiling_factor * 2;
    double* weights = ptr;
    // cluster_num: distribute output channels across clusters
    // i.e. one cluster needs to store CO/cluster_num
    ptr += WEIGHTS_SIZE / cluster_num;
    double* ofmap = ptr;
    // output channels are spatially accross cluster
    ptr += OFMAP_SIZE / cluster_num;
    double* ofmap_GM = ptr;

    // Initially transfer data from global memory
    if (snrt_is_dm_core()) {

        // initalize output with zeros
        memset(ofmap, 0, sizeof(double) * OFMAP_SIZE / cluster_num);

        /* // transfer ifmap and weights from global memory */
        /* snrt_dma_start_1d(ifmap, */
        /*                   ifmap_dram + IFMAP_SIZE / tiling_factor * cluster_idx, */
        /*                   sizeof(double) * IFMAP_SIZE / tiling_factor); */
        /* snrt_dma_start_1d(weights, */
        /*                   weights_dram + WEIGHTS_SIZE / cluster_num * cluster_idx, */
        /*                   sizeof(double) * WEIGHTS_SIZE / cluster_num); */
        /* snrt_dma_wait_all(); */
    }

    snrt_barrier();

    for (uint32_t t = 1; t < tiling_factor; t++) {

        // DMA cores load the next tile from other clusters
        if (snrt_is_dm_core()) {

            int32_t cluster_offset = cluster_tcdm_offset * (((cluster_idx + t) % tiling_factor) - cluster_idx);

            printf("%p %p\n", ifmap + (t % 2) * IFMAP_SIZE / tiling_factor,
                   (void*)(ifmap + ((t + 1) % 2) * IFMAP_SIZE / tiling_factor) + cluster_offset);

            // asynchronously transfer ifmap from other cluster into second buffer
            snrt_dma_start_1d(ifmap + (t % 2) * IFMAP_SIZE / tiling_factor,
                              (void*)(ifmap + ((t + 1) % 2) * IFMAP_SIZE / tiling_factor) + cluster_offset
                              ifmap_dram + IFMAP_SIZE / tiling_factor * cluster_idx,
                              sizeof(double) * IFMAP_SIZE / tiling_factor);
            /* /\* snrt_dma_start_1d(weights, *\/ */
            /*                   weights_dram + WEIGHTS_SIZE / cluster_num * cluster_idx, */
            /*                   sizeof(double) * WEIGHTS_SIZE / cluster_num); */
            snrt_dma_wait_all();
        }
        else {
            conv2d(ifmap + ((t + 1) % 2) * IFMAP_SIZE / tiling_factor,
                   ofmap, weights,
                   l.co / cluster_num, l.ci, l.oh, l.ow, l.ih, l.iw, l.fh, l.fw);
        }
    }

    conv2d(ifmap + (tiling_factor % 2) * IFMAP_SIZE / tiling_factor,
           ofmap, weights,
           l.co / cluster_num, l.ci, l.oh, l.ow, l.ih, l.iw, l.fh, l.fw);

    snrt_barrier();


    /* double* local_weights = weights + snrt_cluster_core_idx() * l.ci * l.fh * l.fw; */
    /* double* local_ofmap = ofmap + snrt_cluster_core_idx() * l.oh * l.ow; */

    /* if (snrt_is_dm_core()) { */
    /*     memset(ofmap,0,sizeof(double)*l.co*l.oh*l.ow/snrt_cluster_num()); */

    /*     snrt_dma_start_1d(ifmap, */
    /*                       ifmap_dram, */
    /*                       sizeof(double)*l.ci*l.ih*l.iw); */

    /*     snrt_dma_start_1d(weights, */
    /*                       weights_dram + l.co * l.ci * l.fh * l.fw / snrt_cluster_num() * snrt_cluster_idx(), */
    /*                       sizeof(double)*l.co*l.ci*l.fh*l.fw / snrt_cluster_num()); */
    /*     snrt_dma_wait_all(); */
    /*     /\* printf("finished transfer\n"); *\/ */
    /* } */
    /* else { */
    /*     /\* printf("waiting for end of transfer\n"); *\/ */
    /* } */
    /* snrt_barrier(); */

    /* if (snrt_is_compute_core()) { */
    /*     benchmark_get_cycle(); */
    /*     conv2d_ssr_frep_reordered(ifmap, local_ofmap, local_weights, */
    /*                               /\* l.co *\/1, l.ci, l.oh, l.ow, l.ih, l.iw, l.fh, l.fw); */
    /*     benchmark_get_cycle(); */
    /* } */
    /* else { */
    /*     /\* printf("waiting for end of computation\n"); *\/ */
    /* } */
    /* snrt_barrier(); */

    /* if (snrt_is_dm_core()) { */
    /*     snrt_dma_start_1d(ofmap_GM, */
    /*                       ofmap_dram + l.co * l.oh * l.ow * snrt_cluster_idx() / snrt_cluster_num(), */
    /*                       sizeof(double)*l.co * l.oh * l.ow / snrt_cluster_num()); */
    /*     snrt_dma_wait_all(); */

    /*     uint32_t errors = 0; */
    /*     for (uint32_t i = 0; i < l.co * l.oh * l.ow / snrt_cluster_num(); i++) { */
    /*         /\* if (ofmap_GM[i] != ofmap[i]) { *\/ */
    /*             /\* errors++; *\/ */
    /*         /\* } *\/ */
    /*         if(fabs(ofmap_GM[i] - ofmap[i]) > 0.001) { */
    /*             errors++; */
    /*         } */
    /*     } */
    /*     if (errors == 0) { */
    /*         printf("No Errors\n"); */
    /*     } else { */
    /*         printf("%d Errors\n"); */
    /*     } */

    /* } */

    /* snrt_barrier(); */

    /* return 0; */
}
