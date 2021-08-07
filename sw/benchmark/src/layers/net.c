// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "benchmark.h"
#include "layer.h"
#include "conv2d.h"
#include "data.h"
#include "math.h"

int main() {

    // layer conv_layer = {
    //     .type = CONV2D,
    //     .CO = CONV2D_CO,
    //     .CI = CONV2D_CI,
    //     .IH = CONV2D_IH,
    //     .IW = CONV2D_IW,
    //     .OH = CONV2D_OH,
    //     .OW = CONV2D_OW,
    //     .FH = CONV2D_FH,
    //     .FW = CONV2D_FW,
    //     .ifmap = (double*)ifmap_dram,
    //     .weights = (double*)weights0_dram,
    //     .ofmap = (double*)result,
    //     .cluster2cluster = 0,
    //     .pad = CONV2D_PAD,
    //     .TILE_CI = 32,
    //     /* .cluster2cluster = 1 */
    // };

    //  layer pooling_layer = {
    //      .type = POOLING, 
    //      .CO = CONV2D_CO, 
    //      .CI = CONV2D_CI, 
    //      .IH = CONV2D_IH, 
    //      .IW = CONV2D_IW, 
    //      .OH = CONV2D_OH, 
    //      .OW = CONV2D_OW, 
    //      .FH = CONV2D_FH, 
    //      .FW = CONV2D_FW, 
    //      .ifmap = (double*)ifmap_dram, 
    //      .ofmap = (double*)result, 
    //      .TILE_CI = 32, 
    //      /\* .cluster2cluster = 1 *\/ 
    //  }; 

     layer batchnorm_layer = { 
         .type = BATCH_NORM, 
         .CO = CONV2D_CO, 
         .CI = CONV2D_CI, 
         .IH = CONV2D_IH, 
         .IW = CONV2D_IW, 
         .OH = CONV2D_OH, 
         .OW = CONV2D_OW, 
         .ifmap = (double*)ifmap_dram, 
         .ofmap = (double*)result, 
         .gamma = (double*)weights0_dram, 
         .beta = (double*)weights1_dram, 
         .TILE_CI = 32, 
         .cluster2cluster = 0
     }; 

    /* if (snrt_is_dm_core()) { */
    /*     printf("ifmap %p, weights %p, ofmap %p\n", conv_layer.ifmap, conv_layer.weights, conv_layer.ofmap); */
    /*     volatile double bubu = conv_layer.weights[0]; */
    /* } */

    conv2d_im2col_fp64(conv_layer);

    // DMA Core compares result with a precomputed checksum
    if (snrt_is_dm_core() && snrt_cluster_idx() == 0) {

        /* printf("hoi\n"); */
        volatile double result_buf[conv_layer.CO];
        volatile double ofmap_checksums[conv_layer.OH][conv_layer.OW];
        uint32_t errors = 0;
        uint32_t total = 0;

        snrt_dma_txid_t ofmap_checksum_txid = snrt_dma_start_1d((double*)ofmap_checksums, checksum, sizeof(checksum));
        snrt_dma_wait_all();

        for (uint32_t oh = 0; oh < conv_layer.OH; oh++) {
            for (uint32_t ow = 0; ow < conv_layer.OW; ow++) {
                snrt_dma_txid_t result_txid = snrt_dma_start_1d((double*)result_buf, &conv_layer.ofmap[(oh*conv_layer.OW + ow)*conv_layer.CO], sizeof(double)*conv_layer.CO);
                snrt_dma_wait_all();

                double checksum_result = 0.0;
                for (uint32_t co = 0; co < conv_layer.CO; co++) {
                    checksum_result += result_buf[co];
                }
                total++;
                if (fabs(checksum_result - ofmap_checksums[oh][ow]) > 0.001) {
                    errors++;
                }
            }
        }

        printf("%d/%d Errors\n", errors, total);
    }

    snrt_barrier();

    return 0;
}
