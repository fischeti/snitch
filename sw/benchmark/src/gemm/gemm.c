// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdio.h>
#include <stdint.h>
#include "gemm.h"
#include "benchmark.h"
#include "snrt.h"

typedef float vfloat __attribute__ ((vector_size (8)));

void gemm(uint32_t M, uint32_t N, uint32_t K,
          double* A, uint32_t ldA,
          double* B, uint32_t ldB,
          double*C, uint32_t ldC, double ALPHA) {



    for (uint32_t m = 0; m < M; m++) {
        for (uint32_t n = 0; n < N; n++) {
            register double c0 = ALPHA*C[m*ldC + n];
            for (uint32_t k = 0; k < K; k++) {
                c0 += A[k + m*ldA] * B[k*ldB + n];
            }
            C[m*ldC + n] = c0;
        }
    }
}

void gemm_ta(uint32_t M, uint32_t N, uint32_t K,
             double* A, uint32_t ldA,
             double* B, uint32_t ldB,
             double*C, uint32_t ldC, double ALPHA) {

    for (uint32_t m = 0; m < M; m++) {
        for (uint32_t n = 0; n < N; n++) {
            register double c0 = ALPHA*C[m*ldC + n];
            for (uint32_t k = 0; k < K; k++) {
                c0 += A[k*M*ldA + m*ldA] * B[k*ldB + n];
            }
            C[m*ldC + n] = c0;
        }
    }
}

void gemm_tb(uint32_t M, uint32_t N, uint32_t K,
             double* A, uint32_t ldA,
             double* B, uint32_t ldB,
             double*C, uint32_t ldC, double ALPHA) {

    for (uint32_t m = 0; m < M; m++) {
        for (uint32_t n = 0; n < N; n++) {
            register double c0 = ALPHA*C[m*ldC + n];
            for (uint32_t k = 0; k < K; k++) {
                c0 += A[k + m*ldA] * B[k + n*ldB];
            }
            C[m*ldC + n] = c0;
        }
    }
}

void gemm_ssr_frep(uint32_t M, uint32_t N, uint32_t K,
                   double* A, uint32_t ldA,
                   double* B, uint32_t ldB,
                   double*C, uint32_t ldC, double ALPHA, uint32_t setup_SSR) {


    register volatile double ft0 asm("ft0");
    register volatile double ft1 asm("ft1");
    register volatile double ft2 asm("ft2");
    asm volatile("" : "=f"(ft0), "=f"(ft1), "=f"(ft2));

    const uint32_t unroll = 8;

    uint32_t ssr0_b[4] = {unroll, K, N/unroll, M};
    uint32_t ssr0_i[4] = {0, 8, 0, 8*ldA};

    uint32_t ssr1_b[4] = {unroll, K, N/unroll, M};
    uint32_t ssr1_i[4] = {8, 8*ldB, 8*unroll, 0};

    snrt_ssr_loop_4d(SNRT_SSR_DM0,
                     ssr0_b[0], ssr0_b[1], ssr0_b[2], ssr0_b[3],
                     ssr0_i[0], ssr0_i[1], ssr0_i[2], ssr0_i[3]);

    snrt_ssr_loop_4d(SNRT_SSR_DM1,
                     ssr1_b[0], ssr1_b[1], ssr1_b[2], ssr1_b[3],
                     ssr1_i[0], ssr1_i[1], ssr1_i[2], ssr1_i[3]);

    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_4D, A);
    snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_4D, B);
    snrt_ssr_enable();

    benchmark_get_cycle();

    register const uint32_t Km1 asm("t0") = K - 1;

    for (uint32_t m = 0; m < M; m++) {
        uint32_t n = 0;
        for (uint32_t n0 = 0; n0 < N/unroll; n0++) {
            register double c0 = ALPHA*C[m*ldC + n + 0];
            register double c1 = ALPHA*C[m*ldC + n + 1];
            register double c2 = ALPHA*C[m*ldC + n + 2];
            register double c3 = ALPHA*C[m*ldC + n + 3];
            register double c4 = ALPHA*C[m*ldC + n + 4];
            register double c5 = ALPHA*C[m*ldC + n + 5];
            register double c6 = ALPHA*C[m*ldC + n + 6];
            register double c7 = ALPHA*C[m*ldC + n + 7];
            /* for (uint32_t k = 0; k < K; k++) { */

                /* for (uint32_t n1 = 0; n1 < unroll; n1++) { */
                    /* c0 += A[k + m*ldA] * B[k*ldB + n + 0]; */
                    /* c1 += A[k + m*ldA] * B[k*ldB + n + 1]; */
                    /* c2 += A[k + m*ldA] * B[k*ldB + n + 2]; */
                    /* c3 += A[k + m*ldA] * B[k*ldB + n + 3]; */
                    /* c4 += A[k + m*ldA] * B[k*ldB + n + 4]; */
                    /* c5 += A[k + m*ldA] * B[k*ldB + n + 5]; */
                    /* c6 += A[k + m*ldA] * B[k*ldB + n + 6]; */
                    /* c7 += A[k + m*ldA] * B[k*ldB + n + 7]; */
                /* } */

                asm volatile(
                             ".word (7 << 20)|(5 << 15)|(1 << 7)|(0b0001011 << 0) \n"
                             "fmadd.d %[c0], ft0, ft1, %[c0] \n"
                             "fmadd.d %[c1], ft0, ft1, %[c1] \n"
                             "fmadd.d %[c2], ft0, ft1, %[c2] \n"
                             "fmadd.d %[c3], ft0, ft1, %[c3] \n"
                             "fmadd.d %[c4], ft0, ft1, %[c4] \n"
                             "fmadd.d %[c5], ft0, ft1, %[c5] \n"
                             "fmadd.d %[c6], ft0, ft1, %[c6] \n"
                             "fmadd.d %[c7], ft0, ft1, %[c7] \n"
                             : [ c0 ] "+f"(c0),
                               [ c1 ] "+f"(c1),
                               [ c2 ] "+f"(c2),
                               [ c3 ] "+f"(c3),
                               [ c4 ] "+f"(c4),
                               [ c5 ] "+f"(c5),
                               [ c6 ] "+f"(c6),
                               [ c7 ] "+f"(c7)
                             : [ K ] "r"(Km1)
                             :"ft0", "ft1");


            /* } */
            C[m*ldC + n + 0] = c0;
            C[m*ldC + n + 1] = c1;
            C[m*ldC + n + 2] = c2;
            C[m*ldC + n + 3] = c3;
            C[m*ldC + n + 4] = c4;
            C[m*ldC + n + 5] = c5;
            C[m*ldC + n + 6] = c6;
            C[m*ldC + n + 7] = c7;
            n += unroll;
        }

        snrt_ssr_disable();

        for (; n < N; n++) {
            double c = C[m*ldC + n];
            for (uint32_t k = 0; k < K; k++) {
                c += A[k + m*ldA] * B[k*ldB + n];
            }
            C[m*ldC + n] = c;
        }

        snrt_ssr_enable();
    }

    snrt_ssr_disable();

    benchmark_get_cycle();


    asm volatile("" ::"f"(ft0), "f"(ft1), "f"(ft2));

}

void gemm_ta_ssr_frep(uint32_t M, uint32_t N, uint32_t K,
                      double* A, uint32_t ldA,
                      double* B, uint32_t ldB,
                      double*C, uint32_t ldC, double ALPHA, uint32_t setup_SSR) {


    register volatile double ft0 asm("ft0");
    register volatile double ft1 asm("ft1");
    register volatile double ft2 asm("ft2");
    asm volatile("" : "=f"(ft0), "=f"(ft1), "=f"(ft2));

    const uint32_t unroll = 8;

    if (setup_SSR ) {

        uint32_t ssr0_b[4] = {unroll, K, N/unroll, M};
        uint32_t ssr0_i[4] = {0, 8*M*ldA, 0, 8*ldA};

        uint32_t ssr1_b[4] = {unroll, K, N/unroll, M};
        uint32_t ssr1_i[4] = {8, 8*ldB, 8*unroll, 0};

        snrt_ssr_loop_4d(SNRT_SSR_DM0,
                         ssr0_b[0], ssr0_b[1], ssr0_b[2], ssr0_b[3],
                         ssr0_i[0], ssr0_i[1], ssr0_i[2], ssr0_i[3]);

        snrt_ssr_loop_4d(SNRT_SSR_DM1,
                         ssr1_b[0], ssr1_b[1], ssr1_b[2], ssr1_b[3],
                         ssr1_i[0], ssr1_i[1], ssr1_i[2], ssr1_i[3]);

    }

    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_4D, A);
    snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_4D, B);
    snrt_ssr_enable();

    benchmark_get_cycle();

    register const uint32_t Km1 asm("t0") = K - 1;

    for (uint32_t m = 0; m < M; m++) {
        uint32_t n = 0;
        for (uint32_t n0 = 0; n0 < N/unroll; n0++) {
            register double c0 = ALPHA*C[m*ldC + n + 0];
            register double c1 = ALPHA*C[m*ldC + n + 1];
            register double c2 = ALPHA*C[m*ldC + n + 2];
            register double c3 = ALPHA*C[m*ldC + n + 3];
            register double c4 = ALPHA*C[m*ldC + n + 4];
            register double c5 = ALPHA*C[m*ldC + n + 5];
            register double c6 = ALPHA*C[m*ldC + n + 6];
            register double c7 = ALPHA*C[m*ldC + n + 7];

            asm volatile(
                         ".word (7 << 20)|(5 << 15)|(1 << 7)|(0b0001011 << 0) \n"
                         "fmadd.d %[c0], ft0, ft1, %[c0] \n"
                         "fmadd.d %[c1], ft0, ft1, %[c1] \n"
                         "fmadd.d %[c2], ft0, ft1, %[c2] \n"
                         "fmadd.d %[c3], ft0, ft1, %[c3] \n"
                         "fmadd.d %[c4], ft0, ft1, %[c4] \n"
                         "fmadd.d %[c5], ft0, ft1, %[c5] \n"
                         "fmadd.d %[c6], ft0, ft1, %[c6] \n"
                         "fmadd.d %[c7], ft0, ft1, %[c7] \n"
                         : [ c0 ] "+f"(c0),
                           [ c1 ] "+f"(c1),
                           [ c2 ] "+f"(c2),
                           [ c3 ] "+f"(c3),
                           [ c4 ] "+f"(c4),
                           [ c5 ] "+f"(c5),
                           [ c6 ] "+f"(c6),
                           [ c7 ] "+f"(c7)
                         : [ K ] "r"(Km1)
                         :"ft0", "ft1");

            C[m*ldC + n + 0] = c0;
            C[m*ldC + n + 1] = c1;
            C[m*ldC + n + 2] = c2;
            C[m*ldC + n + 3] = c3;
            C[m*ldC + n + 4] = c4;
            C[m*ldC + n + 5] = c5;
            C[m*ldC + n + 6] = c6;
            C[m*ldC + n + 7] = c7;
            n += unroll;
        }

        snrt_ssr_disable();

        for (; n < N; n++) {

            double c = ALPHA*C[m*ldC + n];
            for (uint32_t k = 0; k < K; k++) {
                c += A[k*M*ldA + m*ldA] * B[k*ldB + n];
            }
            C[m*ldC + n] = c;
        }

        snrt_ssr_enable();
    }

    snrt_ssr_disable();

    benchmark_get_cycle();


    asm volatile("" ::"f"(ft0), "f"(ft1), "f"(ft2));

}

void gemm_tb_ssr_frep(uint32_t M, uint32_t N, uint32_t K,
                      double* A, uint32_t ldA,
                      double* B, uint32_t ldB,
                      double*C, uint32_t ldC, double ALPHA, uint32_t setup_SSR) {

    register volatile double ft0 asm("ft0");
    register volatile double ft1 asm("ft1");
    register volatile double ft2 asm("ft2");
    asm volatile("" : "=f"(ft0), "=f"(ft1), "=f"(ft2));

    const uint32_t unroll = 8;

    benchmark_get_cycle();

    if (setup_SSR) {

        uint32_t ssr0_b[4] = {unroll, K, N/unroll, M};
        uint32_t ssr0_i[4] = {0, 8, 0, 8*ldA};

        uint32_t ssr1_b[4] = {unroll, K, N/unroll, M};
        uint32_t ssr1_i[4] = {8*ldB, 8, 8*ldB*unroll, 0};

        snrt_ssr_loop_4d(SNRT_SSR_DM0,
                         ssr0_b[0], ssr0_b[1], ssr0_b[2], ssr0_b[3],
                         ssr0_i[0], ssr0_i[1], ssr0_i[2], ssr0_i[3]);

        snrt_ssr_loop_4d(SNRT_SSR_DM1,
                         ssr1_b[0], ssr1_b[1], ssr1_b[2], ssr1_b[3],
                         ssr1_i[0], ssr1_i[1], ssr1_i[2], ssr1_i[3]);
    }

    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_4D, A);
    snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_4D, B);
    snrt_ssr_enable();

    register const uint32_t Km1 asm("t0") = K - 1;

    for (uint32_t m = 0; m < M; m++) {
        uint32_t n = 0;
        for (uint32_t n0 = 0; n0 < N/unroll; n0++) {
            register double c0 = ALPHA*C[m*ldC + n + 0];
            register double c1 = ALPHA*C[m*ldC + n + 1];
            register double c2 = ALPHA*C[m*ldC + n + 2];
            register double c3 = ALPHA*C[m*ldC + n + 3];
            register double c4 = ALPHA*C[m*ldC + n + 4];
            register double c5 = ALPHA*C[m*ldC + n + 5];
            register double c6 = ALPHA*C[m*ldC + n + 6];
            register double c7 = ALPHA*C[m*ldC + n + 7];

            asm volatile(
                         ".word (7 << 20)|(5 << 15)|(1 << 7)|(0b0001011 << 0) \n"
                         "fmadd.d %[c0], ft0, ft1, %[c0] \n"
                         "fmadd.d %[c1], ft0, ft1, %[c1] \n"
                         "fmadd.d %[c2], ft0, ft1, %[c2] \n"
                         "fmadd.d %[c3], ft0, ft1, %[c3] \n"
                         "fmadd.d %[c4], ft0, ft1, %[c4] \n"
                         "fmadd.d %[c5], ft0, ft1, %[c5] \n"
                         "fmadd.d %[c6], ft0, ft1, %[c6] \n"
                         "fmadd.d %[c7], ft0, ft1, %[c7] \n"
                         : [ c0 ] "+f"(c0),
                           [ c1 ] "+f"(c1),
                           [ c2 ] "+f"(c2),
                           [ c3 ] "+f"(c3),
                           [ c4 ] "+f"(c4),
                           [ c5 ] "+f"(c5),
                           [ c6 ] "+f"(c6),
                           [ c7 ] "+f"(c7)
                         : [ K ] "r"(Km1)
                         :"ft0", "ft1");

            C[m*ldC + n + 0] = c0;
            C[m*ldC + n + 1] = c1;
            C[m*ldC + n + 2] = c2;
            C[m*ldC + n + 3] = c3;
            C[m*ldC + n + 4] = c4;
            C[m*ldC + n + 5] = c5;
            C[m*ldC + n + 6] = c6;
            C[m*ldC + n + 7] = c7;
            n += unroll;
        }

        snrt_ssr_disable();

        for (; n < N; n++) {
            double c = ALPHA*C[m*ldC + n];
            for (uint32_t k = 0; k < K; k++) {
                c += A[k + m*ldA] * B[k + n*ldB];
            }
            C[m*ldC + n] = c;
        }

        snrt_ssr_enable();
    }

    snrt_ssr_disable();

    benchmark_get_cycle();


    asm volatile("" ::"f"(ft0), "f"(ft1), "f"(ft2));

}


void gemm_tb_ssr_frep_vec(uint32_t M, uint32_t N, uint32_t K,
                       float* A, uint32_t ldA,
                       float* B, uint32_t ldB,
                       float*C, uint32_t ldC, float ALPHA, uint32_t setup_SSR) {

    register volatile double ft0 asm("ft0");
    register volatile double ft1 asm("ft1");
    register volatile double ft2 asm("ft2");
    asm volatile("" : "=f"(ft0), "=f"(ft1), "=f"(ft2));

    const uint32_t unroll = 8;

    benchmark_get_cycle();

    if (setup_SSR) {

        uint32_t ssr0_b[4] = {unroll, K, N/2/unroll, M};
        uint32_t ssr0_i[4] = {0, sizeof(float), 0, sizeof(float)*ldA};

        uint32_t ssr1_b[4] = {unroll, K, N/2/unroll, M};
        uint32_t ssr1_i[4] = {sizeof(float)*2, sizeof(float)*ldB, sizeof(float)*unroll*2, 0};

        snrt_ssr_loop_4d(SNRT_SSR_DM0,
                         ssr0_b[0], ssr0_b[1], ssr0_b[2], ssr0_b[3],
                         ssr0_i[0], ssr0_i[1], ssr0_i[2], ssr0_i[3]);

        snrt_ssr_loop_4d(SNRT_SSR_DM1,
                         ssr1_b[0], ssr1_b[1], ssr1_b[2], ssr1_b[3],
                         ssr1_i[0], ssr1_i[1], ssr1_i[2], ssr1_i[3]);
    }

    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_4D, A);
    snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_4D, B);
    snrt_ssr_enable();

    register const uint32_t Km1 asm("t0") = K - 1;

    for (uint32_t m = 0; m < M; m++) {
        uint32_t n = 0;
        for (uint32_t n0 = 0; n0 < N/2/unroll; n0++) {
            /* float *ptr = (float*)&C[m*ldC + n]; */

            /* register vfloat c0 = (vfloat) {C[m*ldC + n + 0]*ALPHA, 0}; */
            /* register vfloat c1 = (vfloat) {C[m*ldC + n + 1]*ALPHA, 0}; */
            /* register vfloat c2 = (vfloat) {C[m*ldC + n + 2]*ALPHA, 0}; */
            /* register vfloat c3 = (vfloat) {C[m*ldC + n + 3]*ALPHA, 0}; */
            /* register vfloat c4 = (vfloat) {C[m*ldC + n + 4]*ALPHA, 0}; */
            /* register vfloat c5 = (vfloat) {C[m*ldC + n + 5]*ALPHA, 0}; */
            /* register vfloat c6 = (vfloat) {C[m*ldC + n + 6]*ALPHA, 0}; */
            /* register vfloat c7 = (vfloat) {C[m*ldC + n + 7]*ALPHA, 0}; */

            /* register double c0 = ALPHA*C[m*ldC + n + 0]; */
            /* register double c1 = ALPHA*C[m*ldC + n + 1]; */
            /* register double c2 = ALPHA*C[m*ldC + n + 2]; */
            /* register double c3 = ALPHA*C[m*ldC + n + 3]; */
            /* register double c4 = ALPHA*C[m*ldC + n + 4]; */
            /* register double c5 = ALPHA*C[m*ldC + n + 5]; */
            /* register double c6 = ALPHA*C[m*ldC + n + 6]; */
            /* register double c7 = ALPHA*C[m*ldC + n + 7]; */

            vfloat* _C = (vfloat*)&C[m*ldC + n];

            register vfloat c0 = _C[0];
            register vfloat c1 = _C[1];
            register vfloat c2 = _C[2];
            register vfloat c3 = _C[3];
            register vfloat c4 = _C[4];
            register vfloat c5 = _C[5];
            register vfloat c6 = _C[6];
            register vfloat c7 = _C[7];

            /* asm volatile( */
            /*              ".word (7 << 20)|(5 << 15)|(1 << 7)|(0b0001011 << 0) \n" */
            /*              "vfmac.r.s %[c0], ft1, ft0 \n" */
            /*              "vfmac.r.s %[c1], ft1, ft0 \n" */
            /*              "vfmac.r.s %[c2], ft1, ft0 \n" */
            /*              "vfmac.r.s %[c3], ft1, ft0 \n" */
            /*              "vfmac.r.s %[c4], ft1, ft0 \n" */
            /*              "vfmac.r.s %[c5], ft1, ft0 \n" */
            /*              "vfmac.r.s %[c6], ft1, ft0 \n" */
            /*              "vfmac.r.s %[c7], ft1, ft0 \n" */
            /*              : [ c0 ] "+f"(c0), */
            /*                [ c1 ] "+f"(c1), */
            /*                [ c2 ] "+f"(c2), */
            /*                [ c3 ] "+f"(c3), */
            /*                [ c4 ] "+f"(c4), */
            /*                [ c5 ] "+f"(c5), */
            /*                [ c6 ] "+f"(c6), */
            /*                [ c7 ] "+f"(c7) */
            /*              : [ K ] "r"(Km1) */
            /*              :"ft0", "ft1"); */

            _C[0] = c0;
            _C[1] = c1;
            _C[2] = c2;
            _C[3] = c3;
            _C[4] = c4;
            _C[5] = c5;
            _C[6] = c6;
            _C[7] = c7;
            n += unroll*2;
        }

        snrt_ssr_disable();

        for (; n < N; n++) {
            double c = ALPHA*C[m*ldC + n];
            for (uint32_t k = 0; k < K; k++) {
                c += A[k + m*ldA] * B[k + n*ldB];
            }
            C[m*ldC + n] = c;
        }

        snrt_ssr_enable();
    }

    snrt_ssr_disable();

    benchmark_get_cycle();


    asm volatile("" ::"f"(ft0), "f"(ft1), "f"(ft2));

}