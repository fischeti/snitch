// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "layer.h"

layer layer_config_conv2d_im2col(uint32_t compute_id, uint32_t compute_num, uint32_t CO, uint32_t CI, uint32_t IH, uint32_t IW, uint32_t OH, uint32_t OW, uint32_t FH, uint32_t FW, uint32_t TILE_CI) {

    layer l = {
        .CO = CO,
        .CI = CI,
        .IH = IH,
        .IW = IW,
        .OH = OH,
        .OW = OW,
        .FH = FH,
        .FW = FW,
        /* .ifmap = ifmap, */
        /* .weights = weights, */
        /* .ofmap = ofmap, */

        .im2col = 1,
        .M = OH*OW,
        .M_p = (OH*OW + compute_num - (compute_id + 1))/compute_num,
        .N = CO,
        .K = FH*FW*CI/TILE_CI,
        .A_offset = FH*FW*CI/TILE_CI*compute_id,
        .B_offset = 0,
        .C_offset = compute_id*CO,
        .ldA = FW*FH*CI/TILE_CI*compute_num,
        .ldB = FW*FH*CI/TILE_CI,
        .ldC = CO*compute_num,
        .ALPHA = 0.0
    };

    return l;
}
