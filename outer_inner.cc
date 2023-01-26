#include "map.h"

static Wall walls[] = {
  {32192, 33216, true},
  {33216, 33216, false},
  {33216, 32256, false},
  {32192, 32256, false},
  {32384, 32896, true},
  {32576, 32896, false},
  {32576, 33088, false},
  {32384, 33088, false},
};

static Sector sectors[] = {
  {32640, 32896, 8, &walls[0]},
};

static Map outer_inner_map = {
  32768, 32768, 32768, 0,
  &sectors[0],
  1,
};
