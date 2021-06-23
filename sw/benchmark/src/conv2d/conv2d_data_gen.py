#!/usr/bin/env python
import numpy as np
import re, sys
import struct

np.random.seed(42)

BEGIN_RE = re.compile(r'.*begin dump: (.+)')
END_RE = re.compile(r'.*end dump')
DATA_RE = re.compile(r'\-?[0-9]+\.[0-9]+')

DOUBLE_RE = re.compile(r'[:=]{1}[a-f0-9]{16}]')

f_result = '/scratch/fischeti/snitch/hw/system/occamy/logs/results.txt'

def array_to_cstr(a):
  out = '{'
  if isinstance(a, np.ndarray):
      a = a.flat
  for el in a:
    out += '{}, '.format(el)
  out = out[:-2] + '}'
  return out

def write_header(ifmap, weights, ofmap, mem_layout):

    if mem_layout == 'chw':
        N, CI, IH, IW = ifmap.shape
    else:
        N, IH, IW, CI = ifmap.shape

    CO, _, FH, FW = weights.shape
    OH, OW = IH - (FH - 1), IW - (FW - 1)

    with open('../../include/data.h', 'w') as fd:
        fd.write('#pragma once\n\n')
        fd.write('//#define BANSHEE\n\n')
        fd.write(f'#define {mem_layout.upper()}\n\n')
        fd.write('struct layer_config {\n')
        fd.write(f'\tuint32_t n;\n')
        fd.write(f'\tuint32_t co;\n')
        fd.write(f'\tuint32_t ci;\n')
        fd.write(f'\tuint32_t fh;\n')
        fd.write(f'\tuint32_t fw;\n')
        fd.write(f'\tuint32_t ih;\n')
        fd.write(f'\tuint32_t iw;\n')
        fd.write(f'\tuint32_t oh;\n')
        fd.write(f'\tuint32_t ow;\n')
        fd.write('};\n\n')

        fd.write('struct layer_config l = {\n')
        fd.write(f'\t.n = {N},\n')
        fd.write(f'\t.co = {CO},\n')
        fd.write(f'\t.ci = {CI},\n')
        fd.write(f'\t.fh = {FH},\n')
        fd.write(f'\t.fw = {FW},\n')
        fd.write(f'\t.ih = {IH},\n')
        fd.write(f'\t.iw = {IW},\n')
        fd.write(f'\t.oh = {OH},\n')
        fd.write(f'\t.ow = {OW},\n')
        fd.write('};\n\n')

        fd.write(f'double* ofmap_addr = (void*)0x80040000;\n\n')

        fd.write(f'static double ifmap_dram[{N * CI * IH * IW}] = ' + array_to_cstr(ifmap) + ';\n\n\n')
        fd.write(f'static double weights_dram[{CO * CI * FH * FW}] = ' + array_to_cstr(weights) + ';\n\n\n')
        fd.write(f'static double ofmap_dram[{N * CO * OH * OW}] = ' + array_to_cstr(ofmap) + ';\n\n\n')

def conv2d(ifmap, weights):
    N, CI, IH, IW = ifmap.shape
    CO, _, FH, FW = weights.shape
    OH, OW = IH - (FH - 1), IW - (FW - 1)

    ofmap = np.zeros((N, CO, OH, OW))

    for n in range(N):
        for co in range(CO):
            for ci in range(CI):
                for oh in range(OH):
                    for ow in range(OW):
                        for fh in range(FH):
                            for fw in range(FW):
                                ofmap[n][co][oh][ow] += ifmap[n][ci][oh+fh][ow+fw] * weights[co][ci][fh][fw]

    return ofmap

def parse_dump():
    data_parsing = False
    data = {}
    var = ''

    result = []

    for line in open(f_result).readlines():

        data = struct.unpack('!d', bytes.fromhex(line))[0]
        if (data != 0.0):
            result += [data]

    return result


def main():
    n = 1
    co = 16
    ci = 8
    fh = 3
    fw = 3
    ih = 8
    iw = 8
    oh = ih - (fh - 1)
    ow = iw - (fw - 1)

    # ifmap = np.arange(n*ci*ih*iw).reshape(n, ci, ih, iw)
    ifmap = np.random.random((n, ci, ih, iw))
    # weights = np.ones((co, ci, fh, fw))
    weights = np.random.random((co, ci, fh, fw))

    ofmap = conv2d(ifmap, weights)

    if '-gen' in sys.argv:
        if '-chw' in sys.argv:
            write_header(ifmap, weights, ofmap, 'chw')
        else:
            # convert from CHW to HWC format
            ifmap = np.transpose(ifmap, (0, 2, 3, 1))
            # weights = np.transpose(weights, (0, 2, 3, 1))
            ofmap = np.transpose(ofmap, (0, 2, 3, 1))
            write_header(ifmap, weights, ofmap, 'hwc')

    if '-trace' in sys.argv:

        fpu_ops = ['fmadd.d', 'fadd' 'fld', 'fsd', 'fsub.d', 'fsgnjx.d']

        for line in sys.stdin:
            if any(op in line for op in fpu_ops):
                matches = re.finditer('([a-f0-9]{16})', line)
                for i in matches:
                    line = re.sub(i.group(1), str(struct.unpack('!d', bytes.fromhex(i.group(1)))[0]) , line)
            print(line, end='')

    if '-check' in sys.argv:
        result = parse_dump()
        assert len(result) == ofmap.size
        if not all(np.isclose(result, ofmap.flatten())):
            print('--------------------')
            print('-------\033[91m FAIL \033[00m-------')
            print('--------------------')
            exit(-1)
        else:
            print('--------------------')
            print('------\033[92m SUCCESS \033[00m-----')
            print('--------------------')

if __name__ == '__main__':
    main()
