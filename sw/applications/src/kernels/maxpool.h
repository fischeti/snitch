// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "snrt.h"

void maxpool_fp64(double *ifmap,
                  double *ofmap,
                  uint32_t CI,
                  uint32_t FH,
                  uint32_t FW,
                  uint32_t compute_num,
                  uint32_t setup_SSR);
