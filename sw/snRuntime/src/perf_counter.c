// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0
#include "snrt.h"
#include "team.h"

void snrt_set_perf_counter(int cnt_id, int type) {
    volatile uint32_t *perf_cnt_enable = _snrt_team_current->root->periph_reg + cnt_id * 0x8;
    *perf_cnt_enable = (1 << type);
}

uint32_t snrt_get_perf_counter(int cnt_id) {
    volatile uint32_t *perf_cnt =  _snrt_team_current->root->periph_reg + 0x18 + cnt_id * 0x8;
    return *perf_cnt;
}
