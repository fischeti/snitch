// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include "benchmark.h"
#include "layer.h"
#include "pooling.h"

void max_pooling_fp64(layer l) {

    uint32_t cluster_num = snrt_cluster_num();
    uint32_t cluster_id = snrt_cluster_idx();
    uint32_t compute_num = snrt_cluster_compute_core_num();
    uint32_t compute_id = snrt_cluster_compute_core_idx();

    typedef struct cluster_mem_alloc_struct {
        double
    } cluster_mem_alloc;

    cluster_mem_alloc *mem = (void*)snrt_cluster_memory().start;

}
