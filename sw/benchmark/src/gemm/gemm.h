// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "benchmark.h"

void gemm(uint32_t M, uint32_t N, uint32_t K,
          double* A, uint32_t ldA,
          double* B, uint32_t ldB,
          double*C, uint32_t ldC, double ALPHA);

void gemm_ta(uint32_t M, uint32_t N, uint32_t K,
             double* A, uint32_t ldA,
             double* B, uint32_t ldB,
             double*C, uint32_t ldC, double ALPHA);

void gemm_tb(uint32_t M, uint32_t N, uint32_t K,
             double* A, uint32_t ldA,
             double* B, uint32_t ldB,
             double*C, uint32_t ldC, double ALPHA);

void gemm_ssr_frep(uint32_t M, uint32_t N, uint32_t K,
                   double* A, uint32_t ldA,
                   double* B, uint32_t ldB,
                   double*C, uint32_t ldC, double ALPHA, uint32_t setup_SSR);

void gemm_ta_ssr_frep(uint32_t M, uint32_t N, uint32_t K,
                      double* A, uint32_t ldA,
                      double* B, uint32_t ldB,
                      double*C, uint32_t ldC, double ALPHA, uint32_t setup_SSR);

void gemm_tb_ssr_frep(uint32_t M, uint32_t N, uint32_t K,
                      double* A, uint32_t ldA,
                      double* B, uint32_t ldB,
                      double*C, uint32_t ldC, double ALPHA, uint32_t setup_SSR);