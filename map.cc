#include "map.h"

#include "trig.h"

Player player;

void Player::to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y) {
  int16_t tx = x - this->x;
  int16_t ty = y - this->y;
  // The player is facing up in the overhead view.
  uint16_t ang = PI_OVER_2 - this->ang;
  *vc_x = mul_cos(ang, tx) - mul_sin(ang, ty);
  *vc_y = mul_sin(ang, tx) + mul_cos(ang, ty);
}

void Player::to_vc_z(uint16_t z, int16_t *vc_z) { *vc_z = z - this->z; }

void Player::forward() {
  x += mul_cos(ang, speed);
  y += mul_sin(ang, speed);
}
void Player::backward() {
  x += mul_cos(ang + PI, speed);
  y += mul_sin(ang + PI, speed);
}
void Player::turn_left() { ang += ang_speed; }
void Player::turn_right() { ang -= ang_speed; }
void Player::strafe_left() {
  x += mul_cos(ang + PI_OVER_2, speed);
  y += mul_sin(ang + PI_OVER_2, speed);
}
void Player::strafe_right() {
  x += mul_cos(ang - PI_OVER_2, speed);
  y += mul_sin(ang - PI_OVER_2, speed);
}
