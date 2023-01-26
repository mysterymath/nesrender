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
  void fly_up();
  void fly_down();

  static constexpr Log lspeed = Log(false, 17043);
  static constexpr uint16_t ang_speed = 256;
};

struct Wall {
  uint16_t x, y; // Left
  bool begin_loop;
};

struct Sector {
  uint16_t floor_z, ceiling_z;
  uint8_t num_walls;
  Wall *walls;
};

struct Map {
  uint16_t player_x, player_y, player_z, player_ang;
  uint16_t num_sectors;
  Sector *sectors;
};

extern Player player;
extern Map test_map;

void load_map(const Map &map);

#endif // not MAP_H
