// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "layer.h"
#include "data_conv.h"
#include "conv2d_layer.h"
#include "utils.h"
#include "snrt.h"
#include "math.h"
#include "printf.h"

int main() {

    conv_l.ifmap = (double*)ifmap_dram;
    conv_l.weights = (double*)weights0_dram;
    conv_l.ofmap = (double*)ofmap_dram;
    conv_l.TILE_CI = 32;
    conv_l.pad = (conv_l.FH-1) / 2;

    printf("ifmap %p weights %p\n", conv_l.ifmap, conv_l.weights);

    conv2d_layer(conv_l);

    snrt_global_barrier();

    check_layer(conv_l, (double*)checksum);

    snrt_global_barrier();

    return 0;
}
