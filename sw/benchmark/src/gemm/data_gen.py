#!/usr/bin/env python
import numpy as np
import sys
import argparse


def array_to_cstr(a):
    out = '{'
    if isinstance(a, np.ndarray):
        a = a.flat
    for el in a:
        out += f'{el}, '
    out = out[:-2] + '}'
    return out


def write_header(M: int, N: int, K: int, A: np.ndarray, B: np.ndarray, C: np.ndarray, alpha: float, result: np.ndarray, prec: str) -> None:

    with open('data.h', 'w') as fd:
        fd.write('#pragma once\n\n')
        fd.write('//#define BANSHEE\n\n')

        fd.write(f'#define MAT_M {M}\n')
        fd.write(f'#define MAT_N {N}\n')
        fd.write(f'#define MAT_K {K}\n')
        fd.write(f'#define SIZE_A {M * K}\n')
        fd.write(f'#define SIZE_B {K * N}\n')
        fd.write(f'#define SIZE_C {M * N}\n\n')

        fd.write(f'const {prec} alpha = {alpha};\n\n')
        fd.write(f'static {prec} A_dram[{M}][{K}] = ' + array_to_cstr(A) + ';\n\n\n')
        fd.write(f'static {prec} B_dram[{K}][{N}] = ' + array_to_cstr(B) + ';\n\n\n')
        fd.write(f'static {prec} C_dram[{M}][{N}] = ' + array_to_cstr(C) + ';\n\n\n')
        fd.write(f'static {prec} result_dram[{M}][{N}] = ' + array_to_cstr(result) + ';\n\n\n')


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('--rand', '-r', help="randomly chose dimensions of matrices A, B, C", action="store_true")
    parser.add_argument('--seed', '-s', default=False, const=42, nargs='?', help="fix seed of RNG for debugging", action="store")
    parser.add_argument('--alpha', '-a', default=1.0, nargs='?', type=float, help="scale C matrix with constant factor", action="store")
    parser.add_argument('--transpose', '-t', default='nt', type=str, help="stores Matrix transposed", action="store")
    parser.add_argument('--precision', '-p', default=64, type=int, help="define precision", action="store")


    args = parser.parse_args()

    print(args)

    if args.seed:
        np.random.seed(int(args.seed))

    M = 8
    N = 16
    K = 64


    if args.rand:
        M, N, K = np.random.randint(1, 32, 3)

    if args.precision == 32:
        dtype = np.float32
        dtype_str = "float"
    else:
        dtype = np.float64
        dtype_str = "double"

    print(M, N, K)

    A = np.random.randn(M, K).astype(dtype)
    B = np.random.randn(K, N).astype(dtype)
    C = np.random.randn(M, N).astype(dtype)

    result = np.matmul(A, B) + args.alpha*C

    if args.transpose in ['ta', 'tt']:
        A = A.T
    if args.transpose in ['tb', 'tt']:
        B = B.T

    write_header(M, N, K, A, B, C, args.alpha, result, dtype_str)


if __name__ == '__main__':
    main()
