import sys
import re
import struct


def main():
    fpu_ops = ['fmadd', 'fmul', 'fadd', 'fld', 'fsd', 'fsub', 'fsgnjx', 'flt']

    for line in sys.stdin:
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
            for i in float_hex:
                vec2, vec1 = (struct.unpack('!f', bytes.fromhex(i.group(2)[:8]))[0], struct.unpack('!f', bytes.fromhex(i.group(2)[8:]))[0])
                line = re.sub(i.group(2), f'[{vec2:.4f}, {vec1:.4f}]', line)
        print(line, end='')


if __name__ == '__main__':
    main()
