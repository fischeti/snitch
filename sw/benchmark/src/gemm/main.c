// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <math.h>
#include "data.h"
#include "gemm.h"
#include "layer.h"

/* #define PARALLEL */

int main() {

    uint32_t cluster_num = snrt_cluster_num();
    uint32_t cluster_id = snrt_cluster_idx();
#ifdef PARALLEL
    uint32_t compute_num = snrt_cluster_compute_core_num();
#else
    uint32_t compute_num = 1;
#endif
    uint32_t compute_id = snrt_cluster_compute_core_idx();

    /* layer l = { */
    /*     .type = GEMM, */
    /*     .A = (double*)A_dram, */
    /*     .B = (double*)B_dram, */
    /*     .C = (double*)C_dram, */
    /*     .M = MAT_M, */
    /*     .N = MAT_N, */
    /*     .K = MAT_K, */
    /*     .ldA = MAT_K * compute_num, */
    /*     .ldB = MAT_N, */
    /*     .ldC = MAT_N * compute_num */
    /* }; */

    layer l = {
        .type = GEMM,
        .A = (double*)A_dram,
        .B = (double*)B_dram,
        .C = (double*)C_dram,
        .M = MAT_M,
        .N = MAT_N,
        .K = MAT_K,
        .ldA = compute_num,
        .ldB = MAT_N,
        .ldC = MAT_N * compute_num
    };

    /* layer l = { */
    /*     .type = GEMM, */
    /*     .A = (double*)A_dram, */
    /*     .B = (double*)B_dram, */
    /*     .C = (double*)C_dram, */
    /*     .M = MAT_M, */
    /*     .N = MAT_N, */
    /*     .K = MAT_K, */
    /*     .ldA = MAT_K * compute_num, */
    /*     .ldB = MAT_K, */
    /*     .ldC = MAT_N * compute_num */
    /* }; */

    typedef struct cluster_mem_alloc_struct {
        double A[l.M][l.K];
        /* double padding1[4]; */
        double B[l.K][l.N];
        /* double padding2[8]; */
        double C[l.M][l.N];
    } cluster_mem_alloc;

    cluster_mem_alloc *mem = (void*)snrt_cluster_memory().start;

    if (snrt_is_dm_core()) {

        snrt_dma_txid_t txid_a = snrt_dma_start_1d(&mem->A, l.A, sizeof(double) * SIZE_A);
        snrt_dma_txid_t txid_b = snrt_dma_start_1d(&mem->B, l.B, sizeof(double) * SIZE_B);
        snrt_dma_txid_t txid_c = snrt_dma_start_1d(&mem->C, l.C, sizeof(double) * SIZE_C);

        snrt_dma_wait_all();
    }

    snrt_barrier();

    if (snrt_is_compute_core()) {
#ifdef PARALLEL
        uint32_t M_parallel = (l.M + compute_num - (compute_id + 1))/compute_num;
#else
        uint32_t M_parallel = (compute_id == 0)? l.M : 0;
#endif
        /* gemm_ssr_frep(M_parallel, l.N, l.K, (double*)&mem->A[compute_id], l.ldA, (double*)&mem->B, l.ldB, (double*)&mem->C[compute_id], l.ldC, alpha); */
        gemm_ta_ssr_frep(M_parallel, l.N, l.K, (double*)&mem->A[0][compute_id], l.ldA, (double*)&mem->B, l.ldB, (double*)&mem->C[compute_id], l.ldC, alpha, 1);
        /* gemm_tb_ssr_frep(M_parallel, l.N, l.K, (double*)&mem->A[compute_id], l.ldA, (double*)&mem->B, l.ldB, (double*)&mem->C[compute_id], l.ldC, alpha); */
    }

    snrt_barrier();


    uint32_t errors = 0;
    if (snrt_is_dm_core()) {

        typedef struct result_alloc_struct {
            double result[l.M][l.N];
        } result_alloc;

        result_alloc *result = (void*)snrt_cluster_memory().start;

        snrt_dma_txid_t txid_result = snrt_dma_start_1d(result, result_dram, sizeof(double) * SIZE_C);

        snrt_dma_wait_all();

        for (uint32_t m = 0; m < l.M; m++) {
            for (uint32_t n = 0; n < l.N; n++) {
                if (fabs(mem->C[m][n] - result->result[m][n]) > 0.001) {
                    errors++;
                }
            }
        }

        if (errors > 0) {
            printf("%d Errors\n", errors);
        } else {
            printf("No Errors\n");
        }

    }



    return 0;
}
