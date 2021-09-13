// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "layer.h"
#include "data_gemm.h"
#include "gemm.h"
#include "utils.h"
#include "snrt.h"
#include "printf.h"

#define MAT_ROW_PADDING 1
#define MAT_PADDING 8

int main() {

    layer l1_gemm_l = gemm_l;

    l1_gemm_l.A = (double*)gemm_A_dram;
    l1_gemm_l.B = (double*)gemm_B_dram;
    l1_gemm_l.C = (double*)gemm_C_dram;

    volatile uint32_t cluster_num = snrt_cluster_num();
    volatile uint32_t cluster_id = snrt_cluster_idx();
    // uint32_t compute_num = snrt_cluster_compute_core_num();
    volatile uint32_t compute_num = 8;
    volatile uint32_t compute_id = snrt_cluster_compute_core_idx();

    double *mat_A, *mat_B, *mat_C;
    double *ptr = (double*)snrt_cluster_memory().start;

    mat_A = ptr;
    ptr += l1_gemm_l.M * (l1_gemm_l.K + MAT_ROW_PADDING) + MAT_PADDING;
    mat_B = ptr;
    ptr += (l1_gemm_l.K + MAT_ROW_PADDING) * l1_gemm_l.N;
    mat_C = ptr;
    ptr += l1_gemm_l.M * l1_gemm_l.N;

    // if (snrt_global_core_idx() == 0) {
    //     printf("L1 Utilization %p\n", ptr);
    //     printf("A %p\nB %p\nC %p\n", mat_A, mat_B, mat_C);
    //     printf("M %d N %d K %d\n", l1_gemm_l.M, l1_gemm_l.N, l1_gemm_l.K);
    // }

    snrt_global_barrier();

    if (snrt_is_dm_core()) {
        snrt_dma_txid_t txid_A = \
            snrt_dma_start_2d(mat_A,
                            l1_gemm_l.A,
                            sizeof(double)*l1_gemm_l.K,
                            sizeof(double)*(l1_gemm_l.K + MAT_ROW_PADDING),
                            sizeof(double)*l1_gemm_l.K,
                            l1_gemm_l.M);
        snrt_dma_txid_t txid_B = \
            snrt_dma_start_2d(mat_B,
                            l1_gemm_l.B,
                            sizeof(double)*l1_gemm_l.K,
                            sizeof(double)*(l1_gemm_l.K + MAT_ROW_PADDING),
                            sizeof(double)*l1_gemm_l.K,
                            l1_gemm_l.N);

        snrt_dma_txid_t txid_C = \
            snrt_dma_start_1d(mat_C, l1_gemm_l.C, sizeof(double)*l1_gemm_l.M*l1_gemm_l.N);

        snrt_dma_wait_all();
    }

    snrt_cluster_hw_barrier();

    if (snrt_is_compute_core() && snrt_cluster_compute_core_idx() < compute_num) {
        const uint32_t setup_SSR = 1;

        if (!l1_gemm_l.TA && !l1_gemm_l.TB) {
            printf("not yet implemented\n");
        }
        else if (!l1_gemm_l.TA && l1_gemm_l.TB) {
            volatile uint32_t A_offset = compute_id * (l1_gemm_l.K + MAT_ROW_PADDING);
            volatile uint32_t C_offset = compute_id * l1_gemm_l.N;
            volatile uint32_t ldA = compute_num * (l1_gemm_l.K + MAT_ROW_PADDING);
            volatile uint32_t ldB = l1_gemm_l.K + MAT_ROW_PADDING;

            // benchmark_get_cycle();
            gemm_fp64_tb_ssr_frep(l1_gemm_l.M/compute_num, l1_gemm_l.N, l1_gemm_l.K,
                                &mat_A[A_offset], ldA,
                                mat_B, ldB,
                                &mat_C[C_offset], l1_gemm_l.N,
                                l1_gemm_l.ALPHA, setup_SSR);
            uint32_t bubu = benchmark_get_cycle();
            snrt_cluster_hw_barrier();
        }
        else {
            printf("not yet implemented\n");
        }
    }
    else {
        snrt_cluster_hw_barrier();
    }
    snrt_cluster_hw_barrier();

    return 0;
}
