"""Verify GPIO/SDA stimulus and address ACKs in the two recorded 1 ns VCDs."""
import argparse
import json
from pathlib import Path


def events(path):
    symbols = {}
    t = 0
    with Path(path).open() as stream:
        for line in stream:
            x = line.strip()
            if x.startswith('$timescale') and '1ns' not in x.replace(' ', ''):
                raise ValueError('expected 1 ns VCD')
            if x.startswith('$var'):
                parts = x.split()
                if parts[4] in ('D0', 'D1', 'D3', 'D5'):
                    symbols[parts[3]] = parts[4]
            elif x.startswith('#'):
                t = int(x[1:])
            elif len(x) == 2 and x[0] in '01' and x[1] in symbols:
                yield t, symbols[x[1]], int(x[0])


def inspect(path):
    levels = {'D0': 1, 'D1': 1, 'D3': 0, 'D5': 0}
    markers, sda, adc, addresses = [], [], [], []
    active, bits = False, []
    for t, name, value in events(path):
        previous = levels[name]
        if name == 'D5' and previous != value:
            markers.append((t, value))
        if name == 'D1' and previous != value:
            sda.append((t, value))
            if levels['D0'] and previous == 1 and value == 0:
                active, bits = True, []
        if name == 'D3' and previous == 0 and value == 1:
            adc.append(t)
        if name == 'D0' and previous == 0 and value == 1 and active:
            bits.append(levels['D1'])
            if len(bits) == 9:
                byte = 0
                for bit in bits[:8]:
                    byte = (byte << 1) | bit
                addresses.append({'end_ns': t, 'address': byte >> 1,
                                  'read': bool(byte & 1), 'ack': bits[8] == 0})
                active = False
        levels[name] = value
    return markers, sda, adc, addresses


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('sda_vcd')
    parser.add_argument('startup_vcd')
    args = parser.parse_args()
    marks, sda, adc, _ = inspect(args.sda_vcd)
    if marks[:2] != [(5_000_000_000, 1), (10_000_000_000, 0)]:
        raise ValueError('unexpected clamp markers')
    if [(t, v) for t, v in sda if 5_000_000_000 <= t <= 10_000_000_000] != [(5_000_000_000, 0), (10_000_000_000, 1)]:
        raise ValueError('SDA was not continuously low during clamp')
    during = [t for t in adc if 5_000_000_000 <= t <= 10_000_000_000]
    m2, _, _, addr = inspect(args.startup_vcd)
    if not m2 or m2[0] != (3_000_000_000, 1):
        raise ValueError('unexpected READY marker')
    first = [x for x in addr if x['address'] == 0x68][:4]
    if len(first) != 4 or [x['ack'] for x in first] != [False, False, False, True]:
        raise ValueError('expected three 0x68 NACKs then ACK')
    print(json.dumps({'clamp_ns': marks[:2], 'adc_count': len(during),
                      'adc_max_interval_ms': max((b-a)/1e6 for a, b in zip(during, during[1:])),
                      'ready_ns': m2[0][0], 'startup_address_frames': first}, indent=2))


if __name__ == '__main__':
    main()
