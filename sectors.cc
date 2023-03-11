#include "map.h"

namespace {
extern Sector sectors[];
Wall walls[] = {
  {40512, 24960, Log(0, -32768), Log(1, 0), true, 1, nullptr},
  {41408, 24960, Log(1, 0), Log(0, -32768), false, 1, nullptr},
  {41408, 24640, Log(1, 0), Log(0, -32768), false, 2, &sectors[1]},
  {41408, 24384, Log(1, 0), Log(0, -32768), false, 1, nullptr},
  {41408, 24128, Log(0, -32768), Log(0, 0), false, 1, nullptr},
  {40512, 24128, Log(0, 0), Log(0, -32768), false, 1, nullptr},
  {41408, 24640, Log(0, -32768), Log(1, 0), true, 3, nullptr},
  {41664, 24640, Log(1, 0), Log(0, -32768), false, 0, &sectors[2]},
  {41664, 24384, Log(0, -32768), Log(0, 0), false, 3, nullptr},
  {41408, 24384, Log(0, 0), Log(0, -32768), false, 0, &sectors[0]},
  {41664, 24640, Log(0, -1318), Log(1, -779), true, 2, nullptr},
  {42048, 24960, Log(1, -814), Log(1, -1269), false, 1, nullptr},
  {42432, 24512, Log(1, -814), Log(0, -1269), false, 2, nullptr},
  {42048, 24064, Log(0, -1318), Log(0, -779), false, 1, nullptr},
  {41664, 24384, Log(0, 0), Log(0, -32768), false, 0, &sectors[1]},
};

Sector sectors[] = {
  {32640, 32896, 0, 0, 6, &walls[0]},
  {32688, 32896, 2, 2, 4, &walls[6]},
  {32640, 32960, 3, 1, 5, &walls[10]},
};
}
Map sectors_map = {
  40960, 24576, 32768, 160,
  &sectors[0],
  3,
};
