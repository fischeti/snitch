// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "snrt.h"

void batchnorm_fp64(double *ifmap, 
                    double *gamma, 
                    double *beta,
                    double *ofmap, 
                    uint32_t OW, 
                    uint32_t CI,
                    uint32_t compute_num,
                    uint32_t setup_SSR
                    );