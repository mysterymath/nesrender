#ifndef MAP_H
#define MAP_H

#include <stdint.h>

class Player {
public:
  void forward();
  void backward();
  void turn_left();
  void turn_right();
  void strafe_left();
  void strafe_right();

  void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y);
  void to_vc_z(uint16_t z, int16_t *vc_z);

private:
  uint16_t x = 150;
  uint16_t y = 150;
  uint16_t z = 50;
  uint16_t ang = 0;

  static constexpr uint16_t speed = 5;
  static constexpr uint16_t ang_speed = 1024;
};

extern Player player;

#endif // not MAP_H
