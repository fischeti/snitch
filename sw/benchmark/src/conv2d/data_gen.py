#!/usr/bin/env python
import numpy as np
import sys
import torch
import torch.nn as nn

np.random.seed(42)
torch.manual_seed(42)

def array_to_cstr(a):
    out = '{'
    if isinstance(a, np.ndarray):
        a = a.flat
    if isinstance(a, torch.Tensor):
        a = a.numpy().flat
    for el in a:
        out += '{}, '.format(el)
    out = out[:-2] + '}'
    return out


def write_header(ifmap, weights, ofmap, mem_layout):
    if mem_layout == 'chw':
        n, ci, ih, iw = ifmap.shape
        co, _, fh, fw = weights.shape
        _, _, oh, ow = ofmap.shape

    else:
        n, ih, iw, ci = ifmap.shape
        co, fh, fw, ci = weights.shape
        _, oh, ow, _ = ofmap.shape

    with open('../../include/data.h', 'w') as fd:
        fd.write('//#define BANSHEE\n\n')
        fd.write(f'#define {mem_layout.upper()}\n\n')

        fd.write(f'#define IFMAP_SIZE {n * ci * ih * iw}\n')
        fd.write(f'#define WEIGHTS_SIZE {co * ci * fh * fw}\n')
        fd.write(f'#define OFMAP_SIZE {n * co * oh * ow}\n\n')

        fd.write(f'#define CONV2D_CO {co}\n')
        fd.write(f'#define CONV2D_CI {ci}\n')
        fd.write(f'#define CONV2D_IH {ih}\n')
        fd.write(f'#define CONV2D_IW {iw}\n')
        fd.write(f'#define CONV2D_OH {oh}\n')
        fd.write(f'#define CONV2D_OW {ow}\n')
        fd.write(f'#define CONV2D_FH {fh}\n')
        fd.write(f'#define CONV2D_FW {fw}\n')
        fd.write(f'#define CONV2D_PAD {(fh-1)//2}\n')
        fd.write(f'#define SIZE_IFMAP {ci * ih * iw}\n')
        fd.write(f'#define SIZE_WEIGHTS {co * ci * fh * fw}\n\n')

        if mem_layout == 'hwc':
            fd.write(f'static double result[{oh}][{ow}][{co}] __attribute__((section(".data")));\n\n')
            fd.write(f'static double checksum[{oh}][{ow}] = ' + array_to_cstr(torch.sum(ofmap, dim=-1)) + ';\n\n\n')
            fd.write(f'static double ifmap_dram[{ih}][{iw}][{ci}] = ' + array_to_cstr(ifmap) + ';\n\n\n')
            fd.write(f'static double weights_dram[{co}][{fh}][{fw}][{ci}] = ' + array_to_cstr(weights) + ';\n\n\n')
            fd.write(f'static double ofmap_dram[{oh}][{ow}][{co}] = ' + array_to_cstr(ofmap) + ';\n\n\n')
        else:
            fd.write(f'static double ifmap_dram[{ci}][{ih}][{iw}] = ' + array_to_cstr(ifmap) + ';\n\n\n')
            fd.write(f'static double weights_dram[{co}][{ci}][{fh}][{fw}] = ' + array_to_cstr(weights) + ';\n\n\n')
            fd.write(f'static double ofmap_dram[{co}][{oh}][{ow}] = ' + array_to_cstr(ofmap) + ';\n\n\n')

def conv2d(ifmap, weights):
    n, ci, ih, iw = ifmap.shape
    co, _, fh, fw = weights.shape

    conv2d = nn.Conv2d(ci, co, (fh, fw), padding=((fh-1)//2, (fw-1)//2))
    conv2d.weight = nn.Parameter(torch.Tensor(weights), requires_grad=False)
    conv2d.bias = nn.Parameter(torch.zeros_like(conv2d.bias), requires_grad=False)
    ofmap = conv2d(ifmap)

    return ofmap


def main():
    n = 1
    co = 32
    ci = 128
    fh = 3
    fw = 3
    ih = 8
    iw = 8

    ifmap = torch.randn(n, ci, ih, iw, requires_grad=False)
    weights = torch.randn(co, ci, fh, fw, requires_grad=False)
    ofmap = conv2d(ifmap, weights)

    if '-chw' in sys.argv:
        write_header(ifmap, weights, ofmap, 'chw')
    else:
        # convert from CHW to HWC format
        ifmap = ifmap.permute(0, 2, 3, 1)
        weights = weights.permute(0, 2, 3, 1)
        ofmap = ofmap.permute(0, 2, 3, 1)
        write_header(ifmap, weights, ofmap, 'hwc')


if __name__ == '__main__':
    main()
