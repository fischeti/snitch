// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <snrt.h>
#include <stdio.h>

int main(int core_id, int core_num, void *spm_start, void *spm_end) {
    uint32_t num_cores = 18;
    uint32_t actual_core = snrt_hartid();
    for (uint32_t i = 1; i <= num_cores; i++) {
        snrt_barrier();
        if (i == actual_core) {
            printf("Hello from core %i of %i\n", actual_core, num_cores);
        }
    }


    /* snrt_barrier(); */
    /* if (snrt_hartid() == 3) { */
    /*     printf("%d %d %d \n", snrt_hartid(), core_id, core_num); */
    /* } */
    /* /\* printf("Hello World\n");*\/ */
    return 0;
}
