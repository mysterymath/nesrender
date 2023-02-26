#ifndef PERSPECTIVE_H
#define PERSPECTIVE_H

#include "map.h"

namespace perspective {

void begin();
void end();
void render(const Map &map);

} // namespace perspective

#endif // not PERSPECTIVE_H
