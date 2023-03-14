#include "map.h"

#include <stdio.h>

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

void Player::collide() {
  int16_t px = player.x / 256;
  int16_t py = player.y / 256;

  const Wall *loop_begin = nullptr;
  constexpr int16_t player_width = 64;
  constexpr Log lplayer_width = Log::pow2(6);
  for (uint16_t i = 0; i < sector->num_walls; i++) {
    Wall *w = &sector->walls[i];
    if (w->begin_loop)
      loop_begin = w;
    const Wall *next =
        (i + 1 == sector->num_walls || sector->walls[i + 1].begin_loop)
            ? loop_begin
            : &sector->walls[i + 1];

    // Do a quick series of AABB tests to see if the player is definitely
    // outside the wall's hitbox.
    if (w->nx.sign && (w->x < next->x ? px < w->x - player_width
                                      : px < next->x - player_width))
      continue;
    if (!w->nx.sign && (w->x > next->x ? px > w->x + player_width
                                       : px > next->x + player_width))
      continue;
    if (w->ny.sign && (w->y < next->y ? py < w->y - player_width
                                      : py < next->y - player_width))
      continue;
    if (!w->ny.sign && (w->y > next->y ? py > w->y + player_width
                                       : py > next->y + player_width))
      continue;

    // Each wall has a normal that faces towards the sector, perpendicular
    // to the wall.
    //
    // We can imagine creating a hitbox region parallel to the wall that extends
    // towards the normal. If the width of the region is the same size as the
    // player's hit radius, then a hit circle around the player intersects the
    // wall iff the player's center position is inside such a wall hitbox.
    //
    // A simple way to clip the player's movements to the sector is to iterate
    // over each wall and check if the player is inside the wall's hitbox. If
    // so, the smallest vector in the direction of the normal is added to the
    // player's new position in order to move the player out of the hitbox.
    // This produces the gliding effect observed in 3D games when moving against
    // an wall at an angle.
    //
    // We can consider a function for each wall that, given the player's
    // position, produces the pushback vector.

    // We'd like the pushback function to be linear; that means it must be zero
    // at the origin. To achieve this, set the origin to one player's width from
    // the wall's starting coordingate in the direction of the unit normal.
    int16_t rel_x = px - (w->x + w->nx * lplayer_width);
    int16_t rel_y = py - (w->y + w->ny * lplayer_width);
    Log lrx = rel_x;
    Log lry = rel_y;

    // From here, the pushback vector is just the inverse of the vector
    // projection onto the unit normal.
    // pushback_scale = rel . n
    // pushback = pushback_scale n
    Log pushback_scale = lrx * -w->nx - lry * w->ny;

    // If the pushback is towards the wall (away from the normal), then the
    // player didn't collide with the wall.
    if (pushback_scale.sign)
      continue;

    // Reaching here means that the player is in the half-space on the other
    // side of the wall, but the player may still be to the left or right of the
    // wall's hitbox. Projecting onto the wall's direction gives an indicator
    // for the left hitbox. The wall's direction is given by rotating the normal
    // 90 degrees CCW.
    // [ cos 90 -sin 90 ] [ nx ]
    // [ sin 90  cos 90 ] [ ny ]
    //
    // [ 0 -1 ] [ nx ]
    // [ 1  0 ] [ ny ]
    Log dx = -w->ny;
    Log dy = w->nx;
    if (lrx * dx + lry * dy < 0)
      continue;

    // Similarly, shift the origin over to the right side of the hitbox, and the
    // same relation holds.
    rel_x -= next->x - w->x;
    rel_y -= next->y - w->y;

    if (Log(rel_x) * dx + Log(rel_y) * dy > 0)
      continue;

    int16_t pushback_x = pushback_scale * w->nx;
    int16_t pushback_y = pushback_scale * w->ny;

    player.x += pushback_x * 256;
    player.y += pushback_y * 256;
  }
}

void load_map(const Map &map) {
  player.x = (uint32_t)map.player_x << 8;
  player.y = (uint32_t)map.player_y << 8;
  player.z = (uint32_t)map.player_z << 8;
  player.ang = map.player_ang;
  player.sector = map.player_sector;
}
