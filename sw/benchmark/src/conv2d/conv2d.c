// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include "conv2d.h"

void conv2d(double* ifmap, double* ofmap, double* weights,
            uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW)  {
    for (uint32_t co = 0; co < CO; co++) {
        for (uint32_t ci = 0; ci < CI; ci++) {
            for (uint32_t oh = 0; oh < OH; oh++) {
                for (uint32_t ow = 0; ow < OW; ow++) {
                    for (uint32_t fh = 0; fh < FH; fh++) {
                        for (uint32_t fw = 0; fw < FW; fw++) {
                            uint32_t ofmap_index = co * OH * OW + oh * OW + ow;
                            uint32_t ifmap_index = ci * IH * IW + (oh + fh) * IW + (ow + fw);
                            uint32_t weights_index = co * CI * FH * FW + ci * FH * FW + fh * FW + fw;
                            ofmap[ofmap_index] += ifmap[ifmap_index] * weights[weights_index];
                        }
                    }
                }
            }
        }
    }
}

void conv2d_ssr(double* ifmap, double* ofmap, double* weights,
                uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW)  {

    uint32_t ssr0_b[4] = {FW, FH, OW, OH};
    uint32_t ssr0_i[4] = {1 * 8, IW * 8, 1 * 8, IW * 8};

    uint32_t ssr1_b[4] = {FW, FH, OW, OH};
    uint32_t ssr1_i[4] = {1 * 8, FW * 8, 0 * 8, 0 * 8};

    register volatile double ft0 asm("ft0");
    register volatile double ft1 asm("ft1");
    asm volatile("" : "=f"(ft0), "=f"(ft1));

    for (uint32_t co = 0; co < CO; co++) {
        for (uint32_t ci = 0; ci < CI; ci++) {

            snrt_ssr_loop_4d(SNRT_SSR_DM0,
                             ssr0_b[0], ssr0_b[1], ssr0_b[2], ssr0_b[3],
                             ssr0_i[0], ssr0_i[1], ssr0_i[2], ssr0_i[3]);
            snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_4D, ifmap + ci * IH * IW);

            snrt_ssr_loop_4d(SNRT_SSR_DM1,
                             ssr1_b[0], ssr1_b[1], ssr1_b[2], ssr1_b[3],
                             ssr1_i[0], ssr1_i[1], ssr1_i[2], ssr1_i[3]);
            snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_4D, weights + co * CI * FH * FW + ci * FH * FW);

            snrt_ssr_enable();

            for (uint32_t oh = 0; oh < OH; oh++) {
                for (uint32_t ow = 0; ow < OW; ow++) {
                    uint32_t ofmap_index = co * OH * OW + oh * OW + ow;
                    register double c0 = ofmap[ofmap_index];
                    for (uint32_t fh = 0; fh < FH; fh++) {
                        for (uint32_t fw = 0; fw < FW; fw++) {
                            asm volatile(
                                         "fmadd.d %[c0], ft0, ft1, %[c0] \n"
                                         : [ c0 ] "+f"(c0)::"ft0", "ft1");
                        }
                    }
                    ofmap[ofmap_index] = c0;
                }
            }

            snrt_ssr_disable();
        }
    }
}

void conv2d_ssr_frep(double* ifmap, double* ofmap, double* weights,
                     uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW)  {

    uint32_t ssr0_b[4] = {OW, FW, FH, OH};
    uint32_t ssr0_i[4] = {1 * 8, 1 * 8, IW * 8, IW * 8};

    uint32_t ssr1_b[4] = {OW, FW, FH, OH};
    uint32_t ssr1_i[4] = {0 * 8, 1 * 8, FW * 8, 0 * 8};

    register volatile double ft0 asm("ft0");
    register volatile double ft1 asm("ft1");
    asm volatile("" : "=f"(ft0), "=f"(ft1));

    for (uint32_t co = 0; co < CO; co++) {
        for (uint32_t ci = 0; ci < CI; ci++) {

            snrt_ssr_loop_4d(SNRT_SSR_DM0,
                             ssr0_b[0], ssr0_b[1], ssr0_b[2], ssr0_b[3],
                             ssr0_i[0], ssr0_i[1], ssr0_i[2], ssr0_i[3]);
            snrt_ssr_read(SNRT_SSR_DM0, SNRT_SSR_4D, ifmap + ci * IH * IW);

            snrt_ssr_loop_4d(SNRT_SSR_DM1,
                             ssr1_b[0], ssr1_b[1], ssr1_b[2], ssr1_b[3],
                             ssr1_i[0], ssr1_i[1], ssr1_i[2], ssr1_i[3]);
            snrt_ssr_read(SNRT_SSR_DM1, SNRT_SSR_4D, weights + co * CI * FH * FW + ci * FH * FW);

            snrt_ssr_enable();

            for (uint32_t oh = 0; oh < OH; oh++) {
                uint32_t ofmap_index = co * OH * OW + oh * OW;
                register double c0 = ofmap[ofmap_index+0];
                register double c1 = ofmap[ofmap_index+1];
                register double c2 = ofmap[ofmap_index+2];
                register double c3 = ofmap[ofmap_index+3];
                register double c4 = ofmap[ofmap_index+4];
                register double c5 = ofmap[ofmap_index+5];
                for (uint32_t fh = 0; fh < FH; fh++) {
                    for (uint32_t fw = 0; fw < FW; fw++) {
                        for (uint32_t ow = 0; ow < OW/6; ow++) {

                            asm volatile(
                                         "fmadd.d %[c0], ft0, ft1, %[c0] \n"
                                         "fmadd.d %[c1], ft0, ft1, %[c1] \n"
                                         "fmadd.d %[c2], ft0, ft1, %[c2] \n"
                                         "fmadd.d %[c3], ft0, ft1, %[c3] \n"
                                         "fmadd.d %[c4], ft0, ft1, %[c4] \n"
                                         "fmadd.d %[c5], ft0, ft1, %[c5] \n"
                                         : [ c0 ] "+f"(c0),  [ c1 ] "+f"(c1),  [ c2 ] "+f"(c2),
                                           [ c3 ] "+f"(c3),  [ c4 ] "+f"(c4),  [ c5 ] "+f"(c5)::"ft0", "ft1");
                        }
                    }
                    ofmap[ofmap_index+0] = c0;
                    ofmap[ofmap_index+1] = c1;
                    ofmap[ofmap_index+2] = c2;
                    ofmap[ofmap_index+3] = c3;
                    ofmap[ofmap_index+4] = c4;
                    ofmap[ofmap_index+5] = c5;
                }
            }
            snrt_ssr_disable();
        }
    }
}
