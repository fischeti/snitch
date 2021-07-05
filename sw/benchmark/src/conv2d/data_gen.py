#!/usr/bin/env python
import numpy as np
import sys

np.random.seed(42)


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
        n, ci, ih, iw = ifmap.shape
        co, _, fh, fw = weights.shape

    else:
        n, ih, iw, ci = ifmap.shape
        co, _, fh, fw = weights.shape

    oh, ow = ih - (fh - 1), iw - (fw - 1)

    with open('../../include/data.h', 'w') as fd:
        fd.write('#pragma once\n\n')
        fd.write('//#define BANSHEE\n\n')
        fd.write(f'#define {mem_layout.upper()}\n\n')

        fd.write(f'#define IFMAP_SIZE {n * ci * ih * iw}\n')
        fd.write(f'#define WEIGHTS_SIZE {co * ci * fh * fw}\n')
        fd.write(f'#define OFMAP_SIZE {n * co * oh * ow}\n\n')

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

        fd.write('} l = {\n')
        fd.write(f'\t.n = {n},\n')
        fd.write(f'\t.co = {co},\n')
        fd.write(f'\t.ci = {ci},\n')
        fd.write(f'\t.fh = {fh},\n')
        fd.write(f'\t.fw = {fw},\n')
        fd.write(f'\t.ih = {ih},\n')
        fd.write(f'\t.iw = {iw},\n')
        fd.write(f'\t.oh = {oh},\n')
        fd.write(f'\t.ow = {ow},\n')
        fd.write('};\n\n')

        fd.write(f'double* ofmap_addr = (void*)0x80040000;\n\n')

        if mem_layout == 'hwc':
            fd.write(f'static double ifmap_dram[{ih}][{iw}][{ci}] = ' + array_to_cstr(ifmap) + ';\n\n\n')
            fd.write(f'static double weights_dram[{co}][{ci}][{fh}][{fw}] = ' + array_to_cstr(weights) + ';\n\n\n')
            fd.write(f'static double ofmap_dram[{oh}][{ow}][{co}] = ' + array_to_cstr(ofmap) + ';\n\n\n')
        else:
            fd.write(f'static double ifmap_dram[{ci}][{ih}][{iw}] = ' + array_to_cstr(ifmap) + ';\n\n\n')
            fd.write(f'static double weights_dram[{co}][{ci}][{fh}][{fw}] = ' + array_to_cstr(weights) + ';\n\n\n')
            fd.write(f'static double ofmap_dram[{co}][{oh}][{ow}] = ' + array_to_cstr(ofmap) + ';\n\n\n')


def conv2d(ifmap, weights):
    n, ci, ih, iw = ifmap.shape
    co, _, fh, fw = weights.shape

    oh, ow = ih - (fh - 1), iw - (fw - 1)

    ofmap = np.zeros((n, co, oh, ow))

    for n0 in range(n):
        for co0 in range(co):
            for ci0 in range(ci):
                for oh0 in range(oh):
                    for ow0 in range(ow):
                        for fh0 in range(fh):
                            for fw0 in range(fw):
                                ofmap[n0][co0][oh0][ow0] += ifmap[n0][ci0][oh0 + fh0][ow0 + fw0] \
                                                            * weights[co0][ci0][fh0][fw0]

    return ofmap


def main():
    n = 1
    co = 32
    ci = 8
    fh = 3
    fw = 3
    ih = 8
    iw = 8

    ifmap = np.random.random((n, ci, ih, iw))
    weights = np.random.random((co, ci, fh, fw))
    ofmap = conv2d(ifmap, weights)

    if '-chw' in sys.argv:
        write_header(ifmap, weights, ofmap, 'chw')
    else:
        # convert from CHW to HWC format
        ifmap = np.transpose(ifmap, (0, 2, 3, 1))
        # weights = np.transpose(weights, (0, 2, 3, 1))
        ofmap = np.transpose(ofmap, (0, 2, 3, 1))
        write_header(ifmap, weights, ofmap, 'hwc')


if __name__ == '__main__':
    main()
