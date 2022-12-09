#include "map.h"

#include "trig.h"

uint16_t player_x = 150;
uint16_t player_y = 150;
uint16_t player_z = 50;
uint16_t player_ang = 0;

constexpr uint16_t speed = 5;
constexpr uint16_t ang_speed = 1024;

void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y) {
  int16_t tx = x - player_x;
  int16_t ty = y - player_y;
  // The player is facing up in the overhead view.
  uint16_t ang = PI_OVER_2 - player_ang;
  *vc_x = mul_cos(ang, tx) - mul_sin(ang, ty);
  *vc_y = mul_sin(ang, tx) + mul_cos(ang, ty);
}

void to_vc_z(uint16_t z, int16_t *vc_z) {
  *vc_z = z - player_z;
}

void player_forward() {
  player_x += mul_cos(player_ang, speed);
  player_y += mul_sin(player_ang, speed);
}
void player_backward() {
  player_x += mul_cos(player_ang + PI, speed);
  player_y += mul_sin(player_ang + PI, speed);
}
void player_turn_left() { player_ang += ang_speed; }
void player_turn_right() { player_ang -= ang_speed; }
void player_strafe_left() {
  player_x += mul_cos(player_ang + PI_OVER_2, speed);
  player_y += mul_sin(player_ang + PI_OVER_2, speed);
}
void player_strafe_right() {
  player_x += mul_cos(player_ang - PI_OVER_2, speed);
  player_y += mul_sin(player_ang - PI_OVER_2, speed);
}
