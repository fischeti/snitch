// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "benchmark.h"
#include "layer.h"
#include "pooling.h"
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

    /* layer pooling_layer = { */
    /*     .type = POOLING, */
    /*     .CO = CONV2D_CO, */
    /*     .CI = CONV2D_CI, */
    /*     .IH = CONV2D_IH, */
    /*     .IW = CONV2D_IW, */
    /*     .OH = CONV2D_OH, */
    /*     .OW = CONV2D_OW, */
    /*     .FH = CONV2D_FH, */
    /*     .FW = CONV2D_FW, */
    /*     .ifmap = (double*)ifmap_dram, */
    /*     .ofmap = (double*)result, */
    /*     .TILE_CI = 32, */
    /*     /\* .cluster2cluster = 1 *\/ */
    /* }; */

    /* layer batchnorm_layer = { */
    /*     .type = BATCH_NORM, */
    /*     .CO = CONV2D_CO, */
    /*     .CI = CONV2D_CI, */
    /*     .IH = CONV2D_IH, */
    /*     .IW = CONV2D_IW, */
    /*     .OH = CONV2D_OH, */
    /*     .OW = CONV2D_OW, */
    /*     .ifmap = (double*)ifmap_dram, */
    /*     .ofmap = (double*)result, */
    /*     .gamma = (double*)weights0_dram, */
    /*     .beta = (double*)weights1_dram, */
    /*     .TILE_CI = 32, */
    /*     /\* .cluster2cluster = 1 *\/ */
    /* }; */

    /* if (snrt_is_dm_core()) { */
    /*     printf("ifmap %p, weights %p, ofmap %p\n", conv_layer.ifmap, conv_layer.weights, conv_layer.ofmap); */
    /*     volatile double bubu = conv_layer.weights[0]; */
    /* } */

    /* conv2d_im2col_fp64(conv_layer); */

    /* max_pooling_fp64(conv_layer); */

    batchnorm_fp64(batchnorm_layer);

    layer check_layer = batchnorm_layer;

    snrt_barrier();


    // DMA Core compares result with a precomputed checksum
    if (snrt_is_dm_core() && snrt_cluster_idx() == 0) {

        volatile double result_buf[check_layer.CO];
        volatile double ofmap_checksums[check_layer.OH][check_layer.OW];
        uint32_t errors = 0;
        uint32_t total = 0;

        snrt_dma_txid_t ofmap_checksum_txid = snrt_dma_start_1d((double*)ofmap_checksums, checksum, sizeof(checksum));
        snrt_dma_wait_all();

        // setup SSRs
        snrt_ssr_loop_1d(SNRT_SSR_DM0, check_layer.CO, sizeof(double));

        for (uint32_t oh = 0; oh < check_layer.OH; oh++) {
            for (uint32_t ow = 0; ow < check_layer.OW; ow++) {
                snrt_dma_txid_t result_txid = snrt_dma_start_1d((double*)result_buf, &check_layer.ofmap[(oh*check_layer.OW + ow)*check_layer.CO], sizeof(double)*check_layer.CO);
                snrt_dma_wait_all();

                double checksum_result = 0.0;
                uint32_t ssr = 1;

                if (ssr) {

                    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_1D, result_buf);
                    snrt_ssr_enable();
                    register const uint32_t rep asm ("t0") = check_layer.CO/8 - 1;
                    register volatile double checksum_result0 = 0.0;
                    register volatile double checksum_result1 = 0.0;
                    register volatile double checksum_result2 = 0.0;
                    register volatile double checksum_result3 = 0.0;
                    register volatile double checksum_result4 = 0.0;
                    register volatile double checksum_result5 = 0.0;
                    register volatile double checksum_result6 = 0.0;
                    register volatile double checksum_result7 = 0.0;


                    // frep over OW dimension
                    asm volatile(
                                 ".word (7 << 20)|(5 << 15)|(1 << 7)|(0b0001011 << 0) \n"
                                 "fadd.d %[sum0], ft0, %[sum0] \n"
                                 "fadd.d %[sum1], ft0, %[sum1] \n"
                                 "fadd.d %[sum2], ft0, %[sum2] \n"
                                 "fadd.d %[sum3], ft0, %[sum3] \n"
                                 "fadd.d %[sum4], ft0, %[sum4] \n"
                                 "fadd.d %[sum5], ft0, %[sum5] \n"
                                 "fadd.d %[sum6], ft0, %[sum6] \n"
                                 "fadd.d %[sum7], ft0, %[sum7] \n"
                                 : [ sum0 ] "+f"(checksum_result0),
                                   [ sum1 ] "+f"(checksum_result1),
                                   [ sum2 ] "+f"(checksum_result2),
                                   [ sum3 ] "+f"(checksum_result3),
                                   [ sum4 ] "+f"(checksum_result4),
                                   [ sum5 ] "+f"(checksum_result5),
                                   [ sum6 ] "+f"(checksum_result6),
                                   [ sum7 ] "+f"(checksum_result7)
                                 : [ r ] "r"(rep)
                                 :"ft0", "ft1", "ft2"
                                 );


                    snrt_ssr_disable();

                    checksum_result =\
                        checksum_result0 +
                        checksum_result1 +
                        checksum_result2 +
                        checksum_result3 +
                        checksum_result4 +
                        checksum_result5 +
                        checksum_result6 +
                        checksum_result7;
                }
                else {
                    for (uint32_t co = 0; co < check_layer.CO; co++) {
                        checksum_result += result_buf[co];
                    }
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
