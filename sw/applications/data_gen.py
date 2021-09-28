#!/usr/bin/env python
import numpy as np
import torch
import torch.nn as nn
import argparse
import pathlib
import hjson

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


def emit_header_file(layer_type: str, **kwargs):

    # n, ih, iw, ci = ifmap.shape
    # _, oh, ow, co = ofmap.shape

    file_path = pathlib.Path(__file__).parent / 'data'
    emit_str = ''

    if layer_type == 'Conv2d':
        file = file_path / 'data_conv2d.h'
        emit_str += emit_conv2d_layer(**kwargs)
    elif layer_type == 'GEMM':
        file = file_path / 'data_gemm.h'
        emit_str += emit_GEMM_layer(**kwargs)
    with file.open('w') as f:
        f.write(emit_str)
    #     # weights = None

    # file_path = os.path.dirname(sys.argv[0]) + '/include/'

    # with open(file_path + f'data_{layer_type.__name__[:-2].lower()}.h', 'w') as fd:
    #     fd.write('#include "layer.h"\n\n')
    #     fd.write(f'layer {layer_type.__name__[:-2].lower()}_l = {{\n')
    #     fd.write(f'\t.CO = {co},\n')
    #     fd.write(f'\t.CI = {ci},\n')
    #     fd.write(f'\t.IH = {ih},\n')
    #     fd.write(f'\t.IW = {iw},\n')
    #     fd.write(f'\t.OH = {oh},\n')
    #     fd.write(f'\t.OW = {ow},\n')
    #     fd.write(f'\t.FH = {fh},\n')
    #     fd.write(f'\t.FW = {fw}\n')
    #     fd.write('}};\n\n\n')

    #     fd.write(f'static double result[{oh}][{ow}][{co}] __attribute__((section(".data")));\n\n')
    #     fd.write(f'static double checksum[{oh}][{ow}] = ' + array_to_cstr(torch.sum(ofmap, dim=-1)) + ';\n\n\n')
    #     fd.write(f'static double ifmap_dram[{ih}][{iw}][{ci}] = ' + array_to_cstr(ifmap) + ';\n\n\n')
    #     for i, w in enumerate(weights):
    #         fd.write(f'static double weights{i}_dram')
    #         for s in w.shape:
    #             fd.write(f'[{s}]')
    #         fd.write(' = ' + array_to_cstr(w) + ';\n\n\n')
    #     fd.write(f'static double ofmap_dram[{oh}][{ow}][{co}] = ' + array_to_cstr(ofmap) + ';\n\n\n')


def emit_conv2d_layer(name='conv2d', **kwargs):
    ifmap = kwargs['ifmap']
    ofmap = kwargs['ofmap']
    weights = kwargs['weights']

    n, ih, iw, ci = ifmap.shape
    _, oh, ow, co = ofmap.shape
    _, fh, fw, _ = weights.shape

    layer_str = ''
    layer_str += '#include "layer.h"\n\n'
    layer_str += f'layer {name}_l = {{\n'
    layer_str += f'\t.CO = {co},\n'
    layer_str += f'\t.CI = {ci},\n'
    layer_str += f'\t.IH = {ih},\n'
    layer_str += f'\t.IW = {iw},\n'
    layer_str += f'\t.OH = {oh},\n'
    layer_str += f'\t.OW = {ow},\n'
    layer_str += f'\t.FH = {fh},\n'
    layer_str += f'\t.FW = {fw}\n'
    layer_str += '};\n\n\n'

    layer_str += f'static double {name}_result[{oh}][{ow}][{co}] __attribute__((section(".data")));\n\n'
    layer_str += f'static double {name}_checksum[{oh}][{ow}] = ' + array_to_cstr(torch.sum(ofmap, dim=-1)) + ';\n\n\n'
    layer_str += f'static double {name}_ifmap_dram[{ih}][{iw}][{ci}] = ' + array_to_cstr(ifmap) + ';\n\n\n'
    layer_str += f'static double {name}_weights_dram[{co}][{ci}][{fh}][{fw}] = ' + array_to_cstr(weights) + ';\n\n\n'
    layer_str += f'static double {name}_ofmap_dram[{oh}][{ow}][{co}] = ' + array_to_cstr(ofmap) + ';\n\n\n'

    return layer_str


def emit_linear_layer(input, weights, ofmap):

    layer_str = ''
    return layer_str


def emit_GEMM_layer(name='gemm', **kwargs):
    mat_A = kwargs['A']
    mat_B = kwargs['B']
    mat_C = kwargs['C']
    result = kwargs['result']

    m = kwargs['M']
    n = kwargs['N']
    k = kwargs['K']

    layer_str = ''
    layer_str += '#include "layer.h"\n\n'
    layer_str += f'layer {name}_l = {{\n'
    layer_str += '\t.type = GEMM,\n'
    layer_str += f'\t.M = {m},\n'
    layer_str += f'\t.N = {n},\n'
    layer_str += f'\t.K = {k},\n'
    layer_str += f'\t.TA = {int(kwargs["ta"])},\n'
    layer_str += f'\t.TB = {int(kwargs["tb"])},\n'
    layer_str += f'\t.ALPHA = {kwargs["alpha"]},\n'
    layer_str += f'\t.dtype = FP{kwargs["prec"]}\n'
    layer_str += '};\n\n\n'

    ctypes = {
        '64': 'double',
        '32': 'float',
        '16': '__fp16',
        '8': 'char'
    }

    dtype = ctypes[str(kwargs['prec'])]

    layer_str += f'static {dtype} {name}_A_dram [{m}][{k}] = ' + array_to_cstr(mat_A) + ';\n\n\n'
    layer_str += f'static {dtype} {name}_B_dram [{k}][{n}] = ' + array_to_cstr(mat_B) + ';\n\n\n'
    layer_str += f'static {dtype} {name}_C_dram [{m}][{n}] = ' + array_to_cstr(mat_C) + ';\n\n\n'
    layer_str += f'static {dtype} {name}_result[{m}][{n}] __attribute__((section(".data")));\n\n'
    layer_str += f'static {dtype} {name}_checksum[{m}] = ' + array_to_cstr(torch.sum(result, dim=-1)) + ';\n\n\n'

    return layer_str


def conv2d(ifmap, weights, padding=1, stride=1):
    n, ci, ih, iw = ifmap.shape
    co, _, fh, fw = weights.shape

    conv2d = nn.Conv2d(ci, co, (fh, fw), padding=((fh-1)//2, (fw-1)//2))
    conv2d.weight = nn.Parameter(weights, requires_grad=False)
    conv2d.bias = nn.Parameter(torch.zeros_like(conv2d.bias, dtype=weights.dtype), requires_grad=False)
    ofmap = conv2d(ifmap)

    return ofmap


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

    parser = argparse.ArgumentParser(description='Generate data for kernels')
    parser.add_argument(
        "-c",
        "--cfg",
        type=pathlib.Path,
        required=True,
        help='Select param config file kernel'
    )

    args = parser.parse_args()

    with args.cfg.open() as f:
        param = hjson.loads(f.read())

    if param['prec'] == 64:
        dtype = torch.float64
    elif param['prec'] == 16:
        dtype = torch.float16
    else:
        dtype = torch.float32

    dtype = torch.float32

    if param['kernel'] == 'Conv2d':
        ifmap = torch.randn(1, param['channels']['in'],
                            param['input_dim']['height'],
                            param['input_dim']['width'], requires_grad=False, dtype=dtype)
        weights = torch.randn(param['channels']['out'],
                              param['channels']['in'],
                              param['filter']['height'],
                              param['filter']['width'], requires_grad=False, dtype=dtype)

        ofmap = conv2d(ifmap, weights,
                       padding=param['filter']['padding'],
                       stride=param['filter']['stride'])

        # convert from CHW to HWC format
        ifmap = ifmap.permute(0, 2, 3, 1)
        ofmap = ofmap.permute(0, 2, 3, 1)
        weights = weights.permute(0, 2, 3, 1)
        kwargs = {'ifmap': ifmap, 'weights': weights, 'ofmap': ofmap}
        emit_header_file('Conv2d', **kwargs)

    elif param['kernel'] == 'GEMM':
        mat_A = torch.randn(param['M'], param['K'], requires_grad=False, dtype=dtype)
        mat_B = torch.randn(param['K'], param['N'], requires_grad=False, dtype=dtype)
        mat_C = torch.randn(param['M'], param['N'], requires_grad=False, dtype=dtype)

        result = param['alpha'] * mat_C + torch.matmul(mat_A, mat_B)

        if param['transpose_A']:
            mat_A = mat_A.T
        if param['transpose_B']:
            mat_B = mat_B.T

        kwargs = {
            'A': mat_A,
            'B': mat_B,
            'C': mat_C,
            'result': result,
            'M': param['M'],
            'N': param['N'],
            'K': param['K'],
            'ta': param['transpose_A'],
            'tb': param['transpose_B'],
            'alpha': param['alpha'],
            'prec': param['prec']
            }

        emit_header_file('GEMM', **kwargs)

    # elif 'BatchNorm' in sys.argv:
    #     ofmap, weights = batchnorm(ifmap)
    #     module = nn.BatchNorm2d

    # elif 'MaxPool' in sys.argv:
    #     ofmap, weights = max_pooling(ifmap, fh)
    #     module = nn.MaxPool2d

    # else:
    #     ofmap, weights = batchnorm(ifmap)
    #     module = nn.BatchNorm2d
    #     print('Please specify type of layer')


if __name__ == '__main__':
    main()
