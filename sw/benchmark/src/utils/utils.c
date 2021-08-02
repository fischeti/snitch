// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <string.h>
#include "utils.h"
#include "snrt.h"

void memset_dma(void *ptr, int32_t value, uint32_t len) {

    // set first 64bytes to value
    memset(ptr, value, 64);

    // DMA copy the the rest
    snrt_dma_txid_t memset_txid = snrt_dma_start_2d(ptr, ptr, 64, 64, 0, len / 64);
    snrt_dma_wait_all();

}
