// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "benchmark.h"

void im2col(double *im, double *col, uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW, uint32_t TILE_CI, uint32_t TILE_CI_IDX);
