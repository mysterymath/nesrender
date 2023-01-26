#include "map.h"

static Wall walls[] = {
    {400, 400, true},  {400, 600, false}, {600, 600, false},
    {500, 500, false}, {600, 400, false}, {430, 430, true},
    {420, 420, false}, {430, 410, false}, {440, 420, false},
};

static Sector sectors[] = {{20, 80, 9, walls}};

Map test_map = {500, 500, 50, 0, &sectors[0], sizeof(sectors) / sizeof(Sector)};
