// Copyright 2020 ETH Zurich and University of Bologna.
// Licensed under the Apache License, Version 2.0, see LICENSE for details.
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>

typedef float vfloat __attribute__ ((vector_size (8)));
// #define fma __builtin_pulp_vf32mac
#define lfsr_t uint32_t
#define LFSR_MASK UINT32_C(-1)
#define VLEN 2

int main()
{

    /*
     * FLH TEST
     */

    // asm volatile(
    //     "flh   ft0, 0(t0)           \n"
    //     ::: "memory", "t0", "ft0" );
    // return 0;

    /*
     * FCVT TEST ZERO
     */
    // asm volatile(
    //     "flh   ft0, 0(t0)             \n"
    //     "fcvt.h.w ft0, zero           \n"
    //     "fcvt.h.w ft1, zero           \n"
    //     ::: "memory", "ft0", "ft1", "ft2" );
    // return 0;

    /*
     * FCVT TEST NON-ZERO
     */


    /*
     * VFloatS TEST
     */

    volatile vfloat k = (vfloat) {1.23, 1.23};
    volatile vfloat l = (vfloat) {2.34, 0.1};

    volatile vfloat p;
    volatile vfloat o;
    // volatile vfloat p = k+l;

    asm volatile(
        "vfadd.s %[p], %[k], %[l]"
        : [p] "=f"(p) : [k] "f"(k), [l] "f"(l));
    asm volatile(
        "vfsub.s %[p], %[k], %[l]"
        : [o] "=f"(o) : [k] "f"(k), [l] "f"(l));

    return 0;

}

