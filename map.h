#ifndef MAP_H
#define MAP_H

#include <stdint.h>

#include "log.h"

struct Player {
  uint16_t x = 500;
  uint16_t y = 500;
  uint16_t z = 50;
  uint16_t ang = 0;

  uint16_t dz = 0;

  void forward();
  void backward();
  void turn_left();
  void turn_right();
  void strafe_left();
  void strafe_right();

  static constexpr Log lspeed = Log(false, 4755);
  static constexpr uint16_t ang_speed = 1024;
};

extern Player player;

#endif // not MAP_H
