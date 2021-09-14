#!/usr/bin/env python3

import struct
import re
from tqdm import tqdm
import argparse
import pathlib

FPU_OPS = ['fmadd', 'fmul', 'fadd', 'fld', 'fsd', 'fsub', 'fsgnjx', 'flt']


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--file',
                        required=True,
                        type=pathlib.Path)

    args = parser.parse_args()

    trace_file = args.file
    cwd = args.file.parent.resolve()

    with trace_file.open('r') as traces:
        num_lines = sum(1 for line in open(trace_file,'r'))
        for line in tqdm(traces, total=num_lines, ncols=100):
            hart = line.split()[2]
            if any(op in line for op in FPU_OPS):
                float_hex = re.finditer('([a-f0-9]{16})', line)
                for i in float_hex:
                    if '.s' in line:
                        sub = f"{struct.unpack('!f', bytes.fromhex(i.group(1)[8:]))[0]:.5}"
                    else:
                        sub = f"{struct.unpack('!d', bytes.fromhex(i.group(1)))[0]:.5}"
                    line = re.sub(i.group(1), sub, line)

            if 'unknown' in line:
                float_hex = re.finditer('(f[0-9]{1,2}[:=]{1})([a-f0-9]{16})', line)
                for i in float_hex:
                    vec2, vec1 = (struct.unpack('!f', bytes.fromhex(i.group(2)[:8]))[0], struct.unpack('!f', bytes.fromhex(i.group(2)[8:]))[0])
                    line = re.sub(i.group(2), f'[{vec2:.4f}, {vec1:.4f}]', line)
            trace_file = open(f'{cwd}/trace_hart_{hart}.txt', 'a')
            trace_file.write(line)
            trace_file.close()


if __name__ == '__main__':
    main()
