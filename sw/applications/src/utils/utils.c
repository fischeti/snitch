// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "layer.h"
#include "snrt.h"
#include "utils.h"
#include "printf.h"
#include "math.h"
#include <string.h>

void check_layer(layer l, double* checksum) {

    // DMA Core compares result with a precomputed checksum
    if (snrt_is_dm_core() && snrt_cluster_idx() == 0) {

        volatile double result_buf[l.CO];
        volatile double ofmap_checksums[l.OH][l.OW];
        uint32_t errors = 0;
        uint32_t total = 0;

        snrt_dma_txid_t ofmap_checksum_txid = snrt_dma_start_1d((double*)ofmap_checksums, checksum, sizeof(double) * l.OW * l.OH);
        snrt_dma_wait_all();

        // setup SSRs
        snrt_ssr_loop_1d(SNRT_SSR_DM0, l.CO, sizeof(double));

        for (uint32_t oh = 0; oh < l.OH; oh++) {
            for (uint32_t ow = 0; ow < l.OW; ow++) {
                snrt_dma_txid_t result_txid = snrt_dma_start_1d((double*)result_buf, &l.ofmap[(oh*l.OW + ow)*l.CO], sizeof(double)*l.CO);
                snrt_dma_wait_all();

                double checksum_result = 0.0;
                uint32_t ssr = 1;

                if (ssr) {

                    snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_1D, result_buf);
                    snrt_ssr_enable();
                    register const uint32_t rep asm ("t0") = l.CO/8 - 1;
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

                    checksum_result = \
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
                    for (uint32_t co = 0; co < l.CO; co++) {
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
}

void check_layer_fp32(layer l, float* checksum) {

    // DMA Core compares result with a precomputed checksum
    if (snrt_is_dm_core() && snrt_cluster_idx() == 0) {

        volatile float result_buf[l.CO];
        volatile float ofmap_checksums[l.OH][l.OW];
        uint32_t errors = 0;
        uint32_t total = 0;

        snrt_dma_txid_t ofmap_checksum_txid = snrt_dma_start_1d((float*)ofmap_checksums, checksum, sizeof(float) * l.OW * l.OH);
        snrt_dma_wait_all();

        for (uint32_t oh = 0; oh < l.OH; oh++) {
            for (uint32_t ow = 0; ow < l.OW; ow++) {
                snrt_dma_txid_t result_txid = snrt_dma_start_1d((float*)result_buf, &l.ofmap[(oh*l.OW + ow)*l.CO * sizeof(float)], sizeof(float)*l.CO);
                snrt_dma_wait_all();

                float checksum_result = 0.0;

                for (uint32_t co = 0; co < l.CO; co++) {
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
}

void dma_memset(void *ptr, uint8_t value, uint32_t len) {

    // set first 64bytes to value
    // memset(ptr, value, 64);
    uint8_t *p = ptr;
    uint32_t nbytes = 64;
    while(nbytes--)
    {
        *p++ = value;
    }

    // DMA copy the the rest
    snrt_dma_txid_t memset_txid = snrt_dma_start_2d(ptr, ptr, 64, 64, 0, len / 64);
    snrt_dma_wait_all();

}
