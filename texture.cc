#include "texture.h"

const TextureSpan& TextureColumn::operator[](u8 index) const {
    return spans[index];
}

const TextureColumn& Texture::operator[](u8 index) const {
    return columns[index];
}