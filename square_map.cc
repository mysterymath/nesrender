#include "map.h"

static Wall walls[] = {
  {32640, 32896, true},
  {32896, 32896, false},
  {32896, 32640, false},
  {32640, 32640, false},
};

static Sector sectors[] = {
  {32640, 32896, 4, &walls[0]},
};

Map square_map = {
  32768, 32768, 32768, 0,
  &sectors[0],
  1,
};
