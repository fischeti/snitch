import sys
import re
import struct
import os
from tqdm import tqdm


def main():
    fpu_ops = ['fmadd', 'fmul', 'fadd', 'fld', 'fsd', 'fsub', 'fsgnjx', 'flt']

    path = os.path.dirname(os.path.realpath(__file__))
    file = f'{path}/../logs/banshee/traces.txt'

    with open(file, 'r') as traces:
        num_lines = sum(1 for line in open(file, 'r'))
        for line in tqdm(traces, total=num_lines, ncols=100):
            hart = line.split()[2]
            if any(op in line for op in fpu_ops):
                float_hex = re.finditer('([a-f0-9]{16})', line)
                for i in float_hex:
                    if '.s' in line:
                        sub = f"{struct.unpack('!f', bytes.fromhex(i.group(1)[8:]))[0]:.5}"
                    else:
                        sub = f"{struct.unpack('!d', bytes.fromhex(i.group(1)))[0]:.5}"
                    line = re.sub(i.group(1), sub, line)

            if 'unknown' in line:
                float_hex = re.finditer('(f[0-9]{1,2}[:=]{1})([a-f0-9]{16})', line)
                if 'fp16' in sys.argv:
                    for i in float_hex:
                        vec = (struct.unpack('!e', bytes.fromhex(i.group(2)[:4]))[0], struct.unpack('!e', bytes.fromhex(i.group(2)[4:8]))[0], struct.unpack('!e', bytes.fromhex(i.group(2)[8:12]))[0], struct.unpack('!e', bytes.fromhex(i.group(2)[12:16]))[0])
                        line = re.sub(i.group(2), f'[{vec[0]:.4f}, {vec[1]:.4f}, {vec[2]:.4f}, {vec[3]:.4f}]', line)
                else:
                    for i in float_hex:
                        vec2, vec1 = (struct.unpack('!f', bytes.fromhex(i.group(2)[:8]))[0], struct.unpack('!f', bytes.fromhex(i.group(2)[8:]))[0])
                        line = re.sub(i.group(2), f'[{vec2:.4f}, {vec1:.4f}]', line)
            trace_file = open(f'{path}/../logs/banshee/trace_hart_{hart}.txt', 'a')
            trace_file.write(line)
            trace_file.close()


if __name__ == '__main__':
    main()
