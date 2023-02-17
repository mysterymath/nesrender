#!/usr/bin/env python3

from collections import defaultdict
from pathlib import Path
import json
import subprocess

cycles = defaultdict(int)
with (Path.home() / '.config/mesen/trace').open('r') as file:
    for line in file:
        line = line.strip()
        (addr, cyc) = line.split()
        cycles[int(addr, 0)] += int(cyc)

addrs = "\n".join(map(str, cycles.keys()))

result = subprocess.run(['llvm-symbolizer', '--obj', 'render.nes.elf',
                         '--output-style', 'JSON'],
                        input=addrs, text=True, check=True, capture_output=True)
symbols = {}
for line in result.stdout.splitlines():
    j = json.loads(line)
    symbols[int(j['Address'], 0)] = j

fn_cycles = defaultdict(int)
loc_cycles = defaultdict(int)

for addr in cycles:
    s = symbols[addr]
    f = s['Symbol'][0]
    fn = f["FunctionName"]
    loc = f'{f["FileName"]}:{f["Line"]}:{f["Column"]}'
    fn_cycles[fn] += cycles[addr]
    loc_cycles[f'{fn} {loc}'] += cycles[addr]

print("\n".join(map(str, sorted(fn_cycles.items(), key=lambda x:x[1], reverse=True))))
print("\n".join(map(str, sorted(loc_cycles.items(), key=lambda x:x[1], reverse=True))))
