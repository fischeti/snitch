#!/usr/bin/env python
import numpy as np
import re, sys
import struct

np.random.seed(42)

BEGIN_RE = re.compile(r'.*begin dump: (.+)')
END_RE = re.compile(r'.*end dump')
DATA_RE = re.compile(r'\-?[0-9]+\.[0-9]+')

f_result = '/scratch/fischeti/snitch/hw/system/occamy/logs/results.txt'

def array_to_cstr(a):
  out = '{'
  if isinstance(a, np.ndarray):
      a = a.flat
  for el in a:
    out += '{}, '.format(el)
  out = out[:-2] + '}'
  return out

def write_header(ifmap, weights, ofmap):

    N, CI, IH, IW = ifmap.shape
    CO, _, FH, FW = weights.shape
    OH, OW = IH - (FH - 1), IW - (FW - 1)

    # strides = ssr_strides(ifmap, weights, ofmap)
    # print(strides)

    with open('../../include/conv2d.h', 'w') as fd:
        fd.write('#pragma once\n\n')
        fd.write('//#define BANSHEE\n\n')
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

        fd.write('struct layer_config layer = {\n')
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

        fd.write(f'static double ifmap_dram[{N * CI * IH * IW}] = ' + array_to_cstr(ifmap) + ';\n')
        fd.write(f'static double weights_dram[{CO * CI * FH * FW}] = ' + array_to_cstr(weights) + ';\n')
        fd.write(f'static double ofmap_dram[{N * CO * OH * OW}] = ' + array_to_cstr(ofmap) + ';\n')

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

def calc_offset(indexes, shape):
    offset = 0
    for i in range(len(indexes)):
        offset += indexes[i] * np.prod(shape[i+1:], dtype=int)
    return offset

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
    ci = 1
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
        write_header(ifmap, weights, ofmap)

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
