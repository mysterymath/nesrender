#ifndef LOGO_H
#define LOGO_H
#include "texture.h"
extern const u8 logo_bytes[];
#define logo (*reinterpret_cast<const Texture*>(logo_bytes))
#endif // not LOGO_H
