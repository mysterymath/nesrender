#include "map.h"

#include "trig.h"

Player player;

void Player::forward() {
  x += lcos(ang) * lspeed;
  y += lsin(ang) * lspeed;
}
void Player::backward() {
  x -= lcos(ang) * lspeed;
  y -= lsin(ang) * lspeed;
}
void Player::turn_left() { ang += ang_speed; }
void Player::turn_right() { ang -= ang_speed; }
void Player::strafe_left() {
  x += lcos(ang + PI_OVER_2) * lspeed;
  y += lsin(ang + PI_OVER_2) * lspeed;
}
void Player::strafe_right() {
  x -= lcos(ang + PI_OVER_2) * lspeed;
  y -= lsin(ang + PI_OVER_2) * lspeed;
}
void Player::fly_up() { z += lspeed; }
void Player::fly_down() { z -= lspeed; }

void load_map(const Map &map) {
  player.x = (uint32_t)map.player_x << 8;
  player.y = (uint32_t)map.player_y << 8;
  player.z = (uint32_t)map.player_z << 8;
  player.ang = map.player_ang;
}

static Wall walls[] = {
    {400, 400, true},  {400, 600, false}, {600, 600, false},
    {500, 500, false}, {600, 400, false}, {430, 430, true},
    {420, 420, false}, {430, 410, false}, {440, 420, false},
};

static Sector sectors[] = {{20, 80, 9, walls}};

Map test_map = {
    500, 500, 50, 0, &sectors[0], sizeof(sectors) / sizeof(Sector)};
