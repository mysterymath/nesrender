#include "map.h"

namespace {
extern Sector sectors[];
Wall walls[] = {
  {40512, 24960, true, 1, nullptr},
  {41408, 24960, false, 1, nullptr},
  {41408, 24640, false, 2, &sectors[1]},
  {41408, 24384, false, 1, nullptr},
  {41408, 24128, false, 1, nullptr},
  {40512, 24128, false, 1, nullptr},
  {41408, 24640, true, 3, nullptr},
  {41664, 24640, false, 0, &sectors[2]},
  {41664, 24384, false, 3, nullptr},
  {41408, 24384, false, 0, &sectors[0]},
  {41664, 24640, true, 2, nullptr},
  {42048, 24960, false, 1, nullptr},
  {42432, 24512, false, 2, nullptr},
  {42048, 24064, false, 1, nullptr},
  {41664, 24384, false, 0, &sectors[1]},
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
