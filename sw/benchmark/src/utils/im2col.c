// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include "im2col.h"
#include "benchmark.h"

void im2col_dma(double *src, double *dst, uint32_t CO, uint32_t CI, uint32_t OH, uint32_t OW, uint32_t IH, uint32_t IW, uint32_t FH, uint32_t FW, uint32_t TILE_CI, uint32_t TILE_CI_IDX) {

    for (uint32_t oh = 0; oh < OH; oh++) {
        for (uint32_t ow = 0; ow < OW; ow++) {

            uint32_t src_offset = (oh * IW + ow) * CI;
            uint32_t dst_offset = FH * FW * CI / TILE_CI * ow + FH * FW * CI / TILE_CI * OW * oh;

            if (TILE_CI == 1) {
                snrt_dma_txid_t ifmap_txid = snrt_dma_start_2d(dst + dst_offset,  /* dst */
                                                               src + src_offset,  /* src */
                                                               sizeof(double) * FW * CI, /* size */
                                                               sizeof(double) * FW * CI, /* dst_stride */
                                                               sizeof(double) * IW * CI, /* src_stride */
                                                               FH /* repetitions */);
            } else {

                for (uint32_t fh = 0; fh < FH; fh++) {

                    /* printf("im2col %p %p\n", dst + dst_offset, src + src_offset); */


                    snrt_dma_txid_t ifmap_txid = snrt_dma_start_2d(dst + dst_offset,  /* dst */
                                                                   src + src_offset + CI / TILE_CI * TILE_CI_IDX,  /* src */
                                                                   sizeof(double) * CI / TILE_CI, /* size */
                                                                   sizeof(double) * CI / TILE_CI, /* dst_stride */
                                                                   sizeof(double) * CI, /* src_stride */
                                                                   FW /* repetitions */);

                    src_offset += IW * CI;
                    dst_offset += FW * CI / TILE_CI;

                }
            }
        }
    }

    snrt_dma_wait_all();

    return;
}
