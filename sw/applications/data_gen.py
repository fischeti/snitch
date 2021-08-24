#!/usr/bin/env python
import numpy as np
import sys
import os
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


def write_header(ifmap, weights, ofmap, layer_type, prec):
    n, ih, iw, ci = ifmap.shape
    _, oh, ow, co = ofmap.shape

    if layer_type == nn.Conv2d:
        _, fh, fw, _ = weights[0].shape
    elif layer_type == nn.BatchNorm2d:
        fh, fw = 0, 0
    elif layer_type == nn.MaxPool2d:
        fh, fw = weights
        # weights = None

    file_path = os.path.dirname(sys.argv[0]) + '/include/'

    prec_str = 'double' if prec == 64 else 'float'

    with open(file_path + f'data_{layer_type.__name__[:-2].lower()}.h', 'w') as fd:
        fd.write('#include "layer.h"\n\n')
        fd.write(f'layer {layer_type.__name__[:-2].lower()}_l = {{\n')
        fd.write(f'\t.CO = {co},\n')
        fd.write(f'\t.CI = {ci},\n')
        fd.write(f'\t.IH = {ih},\n')
        fd.write(f'\t.IW = {iw},\n')
        fd.write(f'\t.OH = {oh},\n')
        fd.write(f'\t.OW = {ow},\n')
        fd.write(f'\t.FH = {fh},\n')
        fd.write(f'\t.FW = {fw}\n')
        fd.write('};\n\n\n')

        fd.write(f'static {prec_str} result[{oh}][{ow}][{co}] __attribute__((section(".data")));\n\n')
        fd.write(f'static {prec_str} checksum[{oh}][{ow}] = ' + array_to_cstr(torch.sum(ofmap, dim=-1)) + ';\n\n\n')
        fd.write(f'static {prec_str} ifmap_dram[{ih}][{iw}][{ci}] = ' + array_to_cstr(ifmap) + ';\n\n\n')
        for i, w in enumerate(weights):
            fd.write(f'static {prec_str} weights{i}_dram')
            for s in w.shape:
                fd.write(f'[{s}]')
            fd.write(' = ' + array_to_cstr(w) + ';\n\n\n')
        fd.write(f'static {prec_str} ofmap_dram[{oh}][{ow}][{co}] = ' + array_to_cstr(ofmap) + ';\n\n\n')


def conv2d(ifmap, weights):
    n, ci, ih, iw = ifmap.shape
    co, _, fh, fw = weights.shape

    conv2d = nn.Conv2d(ci, co, (fh, fw), padding=((fh-1)//2, (fw-1)//2))
    conv2d.weight = nn.Parameter(torch.Tensor(weights), requires_grad=False)
    conv2d.bias = nn.Parameter(torch.zeros_like(conv2d.bias), requires_grad=False)
    ofmap = conv2d(ifmap)

    return ofmap, [weights.permute(0, 2, 3, 1)]


def max_pooling(ifmap, kernel):
    n, ci, ih, iw = ifmap.shape
    max_pool = nn.MaxPool2d(kernel_size=kernel)
    ofmap = max_pool(ifmap)

    return ofmap, torch.tensor([kernel, kernel])


def batchnorm(ifmap):
    n, ci, ih, iw = ifmap.shape
    bn = nn.BatchNorm2d(ci)
    bn.weight = nn.Parameter(torch.randn_like(bn.weight), requires_grad=False)
    bn.bias = nn.Parameter(torch.randn_like(bn.bias), requires_grad=False)
    bn.running_mean = nn.Parameter(torch.randn_like(bn.running_mean), requires_grad=False)
    bn.running_var = nn.Parameter(torch.rand_like(bn.running_var), requires_grad=False)
    # ofmap = bn(ifmap)
    gamma = bn.weight / torch.sqrt(bn.running_var + bn.eps)
    beta = bn.bias - bn.running_mean * bn.weight / torch.sqrt(bn.running_var + bn.eps)
    ofmap = ifmap * gamma.unsqueeze(-1).unsqueeze(-1) + beta.unsqueeze(-1).unsqueeze(-1)

    return ofmap, [gamma, beta]


def main():
    n = 1
    co = 8
    ci = 32
    fh = 3
    fw = 3
    ih = 8
    iw = 8

    prec = 32

    ifmap = torch.randn(n, ci, ih, iw, requires_grad=False)

    if 'Conv2d' in sys.argv:
        weights = torch.randn(co, ci, fh, fw, requires_grad=False)
        ofmap, weights = conv2d(ifmap, weights)
        module = nn.Conv2d

    elif 'BatchNorm' in sys.argv:
        ofmap, weights = batchnorm(ifmap)
        module = nn.BatchNorm2d

    elif 'MaxPool' in sys.argv:
        ofmap, weights = max_pooling(ifmap, fh)
    else:
        ofmap, weights = batchnorm(ifmap)
        module = nn.BatchNorm2d
        print('Please specify type of layer')

    # convert from CHW to HWC format
    ifmap = ifmap.permute(0, 2, 3, 1)
    ofmap = ofmap.permute(0, 2, 3, 1)
    write_header(ifmap, weights, ofmap, module, prec)


if __name__ == '__main__':
    main()
