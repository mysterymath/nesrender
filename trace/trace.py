#!/usr/bin/env python3
import profile_pb2
import sys

with open(sys.argv[1]) as file:
    for line in file:
        [addr, count] = line.strip().split()
        print(f'({addr}, {count})')

