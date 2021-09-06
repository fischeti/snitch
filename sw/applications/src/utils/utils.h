// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

uint64_t benchmark();
void check_layer(layer l, double* checksum);
void check_layer_fp32(layer l, float* checksum);
void dma_memset(void *ptr, uint8_t value, uint32_t len);
