#ifndef OVERHEAD_H
#define OVERHEAD_H

#include <stdint.h>

#include "map.h"

namespace overhead {

void render(const Map &map);
void scale_up();
void scale_down();

}

#endif // not OVERHEAD_H
