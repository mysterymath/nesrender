#!/usr/bin/env python3

import profile_pb2
import sys

locations = set()

profile = profile_pb2.Profile()
profile.string_table.extend(["", "cpu", "cycles"])
st = profile.sample_type.add()
st.type = 1
st.unit = 2

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

with open("trace.pb", "wb") as file:
    file.write(profile.SerializeToString())
