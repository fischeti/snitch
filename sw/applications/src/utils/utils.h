// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "snrt.h"
#include "layer.h"

uint32_t benchmark_get_cycle();
void snrt_dma_start_tracking();
void snrt_dma_stop_tracking();
uint32_t check_layer(layer l, double* checksum);
void dma_memset(void *ptr, uint8_t value, uint32_t len);
