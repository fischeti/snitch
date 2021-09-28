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
    parser.add_argument('-p', '--prec',
                        type=int, default=64)

    args = parser.parse_args()

    trace_file = args.file
    cwd = args.file.parent.resolve()

    with trace_file.open('r') as traces:
        num_lines = sum(1 for line in open(trace_file,'r'))
        for line in tqdm(traces, total=num_lines, ncols=100):
            hart = line.split()[2]
            if any(op in line for op in FPU_OPS):
                if '.s.h' in line:
                    float_h_reg = re.finditer('f[0-9]{1,2}:([a-f0-9]{16})', line)
                    float_s_reg = re.finditer('f[0-9]{1,2}=([a-f0-9]{16})', line)
                    for i in float_h_reg:
                        sub = f"{struct.unpack('!e', bytes.fromhex(i.group(1)[-4:]))[0]:.5}"
                        line = re.sub(i.group(1), sub, line)
                    for i in float_s_reg:
                        sub = f"{struct.unpack('!f', bytes.fromhex(i.group(1)[8:]))[0]:.5}"
                        line = re.sub(i.group(1), sub, line)
                else:
                    float_hex = re.finditer('([a-f0-9]{16})', line)
                    for i in float_hex:
                        if '.s' in line:
                            sub = f"{struct.unpack('!f', bytes.fromhex(i.group(1)[8:]))[0]:.5}"
                        else:
                            sub = f"{struct.unpack('!d', bytes.fromhex(i.group(1)))[0]:.5}"
                        line = re.sub(i.group(1), sub, line)

            if 'unknown' in line:
                float_hex = re.finditer('(f[0-9]{1,2}[:=]{1})([a-f0-9]{16})', line)
                for i, g in enumerate(float_hex):
                    if args.prec == 32 or i >= 2:
                        vec1, vec0 = (struct.unpack('!f', bytes.fromhex(g.group(2)[:8]))[0], struct.unpack('!f', bytes.fromhex(g.group(2)[8:]))[0])
                        line = re.sub(g.group(2), f'[{vec1:.4f}, {vec0:.4f}]', line)
                    elif args.prec == 16:
                        vec3, vec2, vec1, vec0 = (struct.unpack('!e', bytes.fromhex(g.group(2)[:4]))[0],
                                                  struct.unpack('!e', bytes.fromhex(g.group(2)[4:8]))[0],
                                                  struct.unpack('!e', bytes.fromhex(g.group(2)[8:12]))[0],
                                                  struct.unpack('!e', bytes.fromhex(g.group(2)[12:]))[0])
                        line = re.sub(g.group(2), f'[{vec3:.4f}, {vec2:.4f}, {vec1:.4f}, {vec0:.4f}]', line)
            trace_file = open(f'{cwd}/trace_hart_{hart}.txt', 'a')
            trace_file.write(line)
            trace_file.close()


if __name__ == '__main__':
    main()
