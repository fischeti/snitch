// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "layer.h"
#include "data_batchnorm.h"
#include "batchnorm_layer.h"
#include "snrt.h"
#include "math.h"
#include "printf.h"
#include "utils.h"

int main() {

    batchnorm_l.ifmap = (double*)ifmap_dram;
    batchnorm_l.ofmap = (double*)ofmap_dram;
    batchnorm_l.gamma = (double*)weights0_dram;
    batchnorm_l.beta = (double*)weights1_dram;
    batchnorm_l.TILE_CI = 32;

    batchnorm_layer(batchnorm_l);

    snrt_global_barrier();

    check_layer(batchnorm_l, (double*)checksum);

    snrt_global_barrier();

    return 0;
}
