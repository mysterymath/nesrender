#include "map.h"

namespace {
extern Sector sectors[];
Wall walls[] = {
  {32192, 33216, Log(0, -32768), Log(1, 0), true, 0, nullptr},
  {33216, 33216, Log(1, 0), Log(0, -32768), false, 0, nullptr},
  {33216, 32256, Log(0, -32768), Log(0, 0), false, 0, nullptr},
  {32192, 32256, Log(0, 0), Log(0, -32768), false, 0, nullptr},
  {32384, 32896, Log(0, -32768), Log(1, 0), true, 0, nullptr},
  {32576, 32896, Log(0, 0), Log(0, -32768), false, 0, nullptr},
  {32576, 33088, Log(0, -32768), Log(0, 0), false, 0, nullptr},
  {32384, 33088, Log(1, 0), Log(0, -32768), false, 0, nullptr},
};

Sector sectors[] = {
  {32640, 32896, 0, 0, 8, &walls[0]},
};
}
Map outer_inner_map = {
  32768, 32768, 32768, 0,
  &sectors[0],
  1,
};
