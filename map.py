from collections import namedtuple
from pathlib import Path
import struct
import sys

map_path = Path(sys.argv[1])

with map_path.open("rb") as f:
  b = f.read()

Header = namedtuple('Header', [
  'version',
  'posx',
  'posy',
  'posz',
  'ang',
  'cursectnum',
])
header_fmt = "<4i2H"
header_sz = struct.calcsize(header_fmt)
header = Header._make(struct.unpack(header_fmt, b[:header_sz]))
b = b[header_sz:]

def parse_list(typ, fmt, b):
  (num,) = struct.unpack("H",b[:2])
  b = b[2:]
  size = struct.calcsize(fmt)
  total_size = num * size
  return (list(map(typ._make, struct.iter_unpack(fmt, b[:total_size]))), b[total_size:])

sectors, b = parse_list(
  namedtuple('Sector', [
   'wallptr', 'wallnum',
   'ceilingz', 'floorz',
   'ceilingstat', 'floorstat',
   'ceilingpicnum', 'ceilingheinum',
   'ceilingshade',
   'ceilingpal', 'ceilingxpanning', 'ceilingypanning',
   'floorpicnum', 'floorheinum',
   'floorshade',
   'floorpal', 'floorxpanning', 'floorypanning',
   'visibility', 'filler',
   'lotag', 'hitag', 'extra',
  ]),
  '< 2h 2l 2h 2h b 3B 2h b 3B 2B 3h',
  b
)

walls, b = parse_list(
  namedtuple('Wall', [
   'x', 'y',
   'point2', 'nextwall', 'nextsector', 'cstat',
   'picnum', 'overpicnum',
   'shade',
   'pal', 'xrepeat', 'yrepeat', 'xpanning', 'ypanning',
   'lotag', 'hitag', 'extra',
  ]),
  '< 2l 4h 2h b 5B 3h',
  b
)

sprites, b = parse_list(
  namedtuple('Sprite', [
   'x', 'y', 'z',
   'cstat', 'picnum',
   'shade',
   'pal', 'clipdist', 'filler',
   'xrepeat', 'yrepeat',
   'xoffset', 'yoffset',
   'sectnum', 'statnum',
   'ang', 'owner', 'xvel', 'yvel', 'zvel',
   'lotag', 'hitag', 'extra',
  ]),
  '< 3l 2h b 3B 2B 2b 2h 5h 3h',
  b
)

def transform_x(x):
  return round(x / 4) + 32768 # i.e., / 2^17 * 2^15

def transform_y(y):
  return round(-y / 4) + 32768 # i.e., / 2^17 * 2^15

def transform_z(z):
  return round(-z / 2**6) + 32768 # i.e., / 2^14 * 2^10, but then scale by xy, i.e., / 2^2

def transform_ang(ang):
  if not ang:
    return 0
  return 65536 - ang * 2**5 # / 2^11 * 2^16 * -1


print('#include "map.h"\n')

print('static Wall walls[] = {')
chain_begin = 0
for i, w in enumerate(walls):
  x = transform_x(w.x)
  y = transform_y(w.y)
  begins_chain = 'true' if i == chain_begin else 'false'
  if w.point2 == chain_begin:
    chain_begin = i+1
  print(f'  {{{x}, {y}, {begins_chain}}},')
print('};\n');

print('static Sector sectors[] = {')
for s in sectors:
  floor_z = transform_z(s.floorz)
  ceiling_z = transform_z(s.ceilingz)
  print(f'  {{{floor_z}, {ceiling_z}, {s.wallnum}, &walls[{s.wallptr}]}},')
print('};\n');

print(f'static Map {map_path.stem}_map = {{')
player_x = transform_x(header.posx)
player_y = transform_y(header.posy)
player_z = transform_z(header.posz)
player_ang = transform_ang(header.ang)
print(f'  {player_x}, {player_y}, {player_z}, {player_ang},')
print(f'  &sectors[{header.cursectnum}],')
print(f'  {len(sectors)},')
print('};')
