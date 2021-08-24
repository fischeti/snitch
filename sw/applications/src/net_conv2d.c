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

    conv_l.ifmap = (void*)ifmap_dram;
    conv_l.weights = (void*)weights0_dram;
    conv_l.ofmap = (void*)ofmap_dram;
    conv_l.TILE_CI = 32;
    conv_l.pad = (conv_l.FH-1) / 2;
    conv_l.dtype = FP32;

    conv2d_layer(conv_l);

    snrt_global_barrier();

    check_layer(conv_l, (void*)checksum);

    snrt_global_barrier();

    return 0;
}
