// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "benchmark.h"

void conv2d(double* ifmap, double* ofmap, double* weights,
            uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW);

void conv2d_hwc(double* ifmap, double* ofmap, double* weights,
                uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW);

void conv2d_hwc_ssr_frep(double* ifmap, double* ofmap, double* weights,
                    uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW);

void conv2d_ssr(double* ifmap, double* ofmap, double* weights,
                uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW);

void conv2d_ssr_frep(double* ifmap, double* ofmap, double* weights,
                     uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW);
void conv2d_ssr_frep_reordered(double* ifmap, double* ofmap, double* weights,
                               uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW);
