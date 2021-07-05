import sys
import re
import struct


def main():
    fpu_ops = ['fmadd.d', 'fadd' 'fld', 'fsd', 'fsub.d', 'fsgnjx.d']

    for line in sys.stdin:
        if any(op in line for op in fpu_ops):
            matches = re.finditer('([a-f0-9]{16})', line)
            for i in matches:
                line = re.sub(i.group(1), str(struct.unpack('!d', bytes.fromhex(i.group(1)))[0]), line)
        print(line, end='')


if __name__ == '__main__':
    main()
