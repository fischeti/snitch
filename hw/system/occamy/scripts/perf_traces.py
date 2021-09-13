#!/usr/bin/env python3

import re
import argparse
from typing import overload
from tqdm import tqdm
import numpy as np

TRACE_REGEX = r'\s*(\d*)\s+(\d*)\s+M\s(0x[0-9a-fA-F]{8}|\s*)\s(\w+\.?\w*|\s*).*#;\s.*?(\d+)'
trace_re = re.compile(TRACE_REGEX)


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument('-f', '--files', nargs='+', help='trace files')

    args = parser.parse_args()

    stats = []

    for i, f in enumerate(args.files):
        with open(f, 'r') as traces:
            num_lines = sum(1 for line in open(f, 'r'))
            frep_counter = 0
            frep_start, frep_end = 0, 0
            perf = {'n_frep_rep': 0,
                     'n_frep': 0,
                     'frep_cycles': [],
                     'frep_start': -1,
                     'frep_end': 0,
                     'section': []}

            current_time, current_cycle = 0, 0

            for line in traces:
                match = trace_re.match(line)
                if match is not None:
                    time, cycle, pc, instr, n_frep = match.groups()
                    if not len(time) == 0:
                        current_time, current_cycle = int(time), int(cycle)
                    if instr == 'frep':
                        frep_counter = int(n_frep)
                        perf['n_frep'] = int(n_frep)
                        perf['n_frep_rep'] += 1
                        if perf['frep_start'] == -1:
                            perf['frep_start'] = current_cycle
                        frep_start = current_cycle
                    if instr == 'fmadd.d':
                        frep_counter -= 1
                        if (frep_counter == 0):
                            frep_end = current_cycle
                            perf['frep_cycles'].append(frep_end-frep_start)
                            perf['frep_end'] = frep_end
                    if instr == 'csrr':
                        if re.search('mcycle', line):
                            perf['section'].append(current_cycle)

            assert frep_counter == 0, "not all frep instr captured"
            assert len(perf['section']) <= 2, "too many `csrr mcycle`"

            if (perf['n_frep_rep'] != 0):
                hart_stats = {
                    'total_frep_cycles': sum(perf['frep_cycles']),
                    'avg_frep_cycles': np.mean(perf['frep_cycles']),
                    'actual_frep_cycles': perf['frep_end'] - perf['frep_start'],
                    'overhead_cycles': perf['frep_end'] - perf['frep_start'] - sum(perf['frep_cycles']),
                    'section_cycles': perf['section'][1] - perf['section'][0],
                    'fpu_utilization_section': perf['n_frep']*perf['n_frep_rep']/(perf['section'][1] - perf['section'][0]),
                    'fpu_utilization_frep': perf['n_frep']*perf['n_frep_rep']/sum(perf['frep_cycles'])
                }

                stats.append(hart_stats)

                print(f"hartid {i} section [{perf['section'][0]},{perf['section'][1]}]")
                for key, value in hart_stats.items():
                    print(key, f"{value:.2f}")
                print("")

    print(f"Overall averaged stats")
    for key in hart_stats.keys():
        print(key, f"{np.mean([i[key] for i in stats]):.3f}")


if __name__ == '__main__':
    main()
