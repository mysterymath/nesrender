#ifndef MAP_H
#define MAP_H

#include <stdint.h>

struct Player {
  uint16_t x = 150;
  uint16_t y = 150;
  uint16_t z = 50;
  uint16_t ang = 0;

  uint16_t dz = 0;

  void forward();
  void backward();
  void turn_left();
  void turn_right();
  void strafe_left();
  void strafe_right();

  static constexpr uint16_t speed = 5;
  static constexpr uint16_t ang_speed = 1024;
};

extern Player player;

#endif // not MAP_H
