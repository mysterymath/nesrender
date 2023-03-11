#include "map.h"

namespace {
extern Sector sectors[];
Wall walls[] = {
  {32640, 32896, Log(0, -32768), Log(1, 0), true, 1, nullptr},
  {32896, 32896, Log(1, 0), Log(0, -32768), false, 2, nullptr},
  {32896, 32640, Log(0, -32768), Log(0, 0), false, 3, nullptr},
  {32640, 32640, Log(0, 0), Log(0, -32768), false, 0, nullptr},
};

Sector sectors[] = {
  {32640, 32896, 1, 0, 4, &walls[0]},
};
}
Map square_map = {
  32768, 32768, 32768, 0,
  &sectors[0],
  1,
};
