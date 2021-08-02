// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include "math.h"
#include "data.h"
#include "layer.h"
#include "snrt.h"

typedef float vfloat __attribute__ ((vector_size (8)));

int main() {

    volatile vfloat rs1 = (vfloat) {1.0, 2.0};
    volatile vfloat rs2 = (vfloat) {3.0, 4.0};

    volatile vfloat rd = (vfloat) {5.0, 6.0};

    uint32_t cluster_id = snrt_cluster_idx();
    uint32_t cluster_num = snrt_cluster_num();
    uint32_t compute_id = snrt_cluster_compute_core_idx();
    /* uint32_t compute_num = snrt_cluster_compute_core_num(); */
    uint32_t compute_num = 1;

    layer l =    {
        .type = GEMM,
        .A = (float*)A_dram,
        .B = (float*)B_dram,
        .C = (float*)C_dram,
        .M = MAT_M,
        .N = MAT_N,
        .K = MAT_K,
        .ldA = MAT_K * compute_num,
        .ldB = MAT_N,
        .ldC = MAT_N * compute_num
    };


    typedef struct cluster_mem_alloc_struct {
        float A[l.M][l.K];
        float B[l.K][l.N];
        float C[l.M][l.N];
    } cluster_mem_alloc;

    cluster_mem_alloc *mem = (void*)snrt_cluster_memory().start;


    snrt_dma_txid_t txid_a = snrt_dma_start_1d(&mem->A, l.A, sizeof(float) * SIZE_A);
    snrt_dma_txid_t txid_b = snrt_dma_start_1d(&mem->B, l.B, sizeof(float) * SIZE_B);
    snrt_dma_txid_t txid_c = snrt_dma_start_1d(&mem->C, l.C, sizeof(float) * SIZE_C);

    snrt_dma_wait_all();

    float alpha = 1.0;
    uint32_t setup_SSR = 1;

    gemm_tb_ssr_frep_vec(l.M, l.N, l.K, (float*)&mem->A[compute_id], l.ldA, (float*)&mem->B, l.ldB, (float*)&mem->C[compute_id], l.ldC, alpha, setup_SSR);

    volatile uint32_t errors = 0;

    snrt_dma_txid_t ofmap_tid = snrt_dma_start_1d(mem->A, result_dram, sizeof(float)*l.M*l.N);
    snrt_dma_wait_all();

    for (uint32_t m = 0; m < l.M; m++) {
        for (uint32_t n = 0; n < l.N; n++) {
            if (fabs(mem->A[0][m*l.M + n] - mem->C[0][m*l.M + n]) > 0.001) {
                errors++;
            }
        }
    }

    /* printf("%d Errors\n", errors); */

    return 0;
}
