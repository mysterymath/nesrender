#ifndef MAP_H
#define MAP_H

#include <stdint.h>

#include "log.h"

struct Player {
  uint32_t x = 500l * 256;
  uint32_t y = 500l * 256;
  uint32_t z = 50l * 256;
  uint16_t ang = 0;

  uint16_t dz = 0;

  void forward();
  void backward();
  void turn_left();
  void turn_right();
  void strafe_left();
  void strafe_right();

  static constexpr Log lspeed = Log(false, 17043);
  static constexpr uint16_t ang_speed = 256;
};

extern Player player;

#endif // not MAP_H
