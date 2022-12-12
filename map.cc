#include "map.h"

#include "trig.h"

Player player;

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
