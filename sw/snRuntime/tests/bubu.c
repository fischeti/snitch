// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <snrt.h>
#include <stdio.h>

int main(int core_id, int core_num, void *spm_start, void *spm_end) {

    for (uint32_t i = 0; i < snrt_global_core_num(); i++) {
        snrt_quadrant_barrier();
        if (i == snrt_hartid() - 1) {
            printf("Core %d\n", snrt_hartid());
        }
    }

    /* if (snrt_hartid() == 1) { */
    /*     printf("Bubu %d %d\n", core_id, snrt_hartid()); */
    /*     asm volatile ( */
    /*                   "lui     t0, 0xa0000  \n" */
    /*                   /\* "addi    t0, t0, 0x610 \n" *\/ */
    /*                   "li      t1, 1   \n" */
    /*                   /\* "sw      t1, 0(t0)  \n" *\/ */
    /*                   /\* "lw      t1, 0(t0)\n" *\/ */
    /*                   "amoadd.w  t2, t1, (t0)\n" */
    /*                   "lw t1, 0(t0)\n" */
    /*                   "addi t0, t1, 0\n" */
    /*                   ); */
    /* } */

    /* uint32_t x = snrt_global_core_idx(); */
    /* uint32_t y = snrt_global_core_num(); */
    /* for (uint32_t i = 0; i < y; i++) { */
    /*     snrt_quadrant_barrier(); */
    /*     if (i == x) { */
    /*         printf("Hello from core %i of %i\n", x, y); */
    /*     } */
    /* } */
    return 0;
}
