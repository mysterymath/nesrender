#!/usr/bin/env python3

import profile_pb2
import sys

locations = set()

profile = profile_pb2.Profile()
profile.string_table.extend(["", "cpu", "cycles"])

st = profile.sample_type.add()
st.type = 1
st.unit = 2
mapping = profile.mapping.add()
mapping.id = 1
mapping.memory_start = 0
mapping.memory_limit = 0x1000000

with open(sys.argv[1]) as file:
    for line in file:
        [addr, count] = line.strip().split()
        addr = int(addr, 0)
        count = int(count, 0)
        sample = profile.sample.add()
        sample.location_id.append(addr)
        sample.value.append(count)
        locations.add(addr)

for l in locations:
    loc = profile.location.add()
    loc.id = loc.address = l
    loc.mapping_id = 1

with open("trace.pb", "wb") as file:
    file.write(profile.SerializeToString())
