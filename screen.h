#ifndef SCREEN_H
#define SCREEN_H

#include <stdint.h>

extern bool still_presenting;
extern uint8_t fb_cur[960];
extern uint8_t fb_next[960];

constexpr uint16_t screen_width = 64;
constexpr uint16_t screen_height = 60;

extern "C" void present();

#endif // not SCREEN_H
