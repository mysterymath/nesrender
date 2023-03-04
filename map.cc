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

void Player::collide(const Sector &s) {
  const Wall *loop_begin = nullptr;
  for (uint16_t i = 0; i < s.num_walls; i++) {
    Wall *w = &s.walls[i];
    if (w->begin_loop)
      loop_begin = w;
    const Wall *next = (i + 1 == s.num_walls || s.walls[i + 1].begin_loop)
                           ? loop_begin
                           : &s.walls[i + 1];
    // TODO: Quickly limit the number of walls we have to consider.

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
    // position, produces the pushback vector.  Such a function should be affine
    // in world space. If the dot product of the produced vector and the normal
    // is negative, the player didn't collide with the wall.
    //
    // Let's translate and the coordinate system so that the wall starts at the
    // origin.
    int16_t rel_x = player.x - w->x;
    int16_t rel_y = player.y - w->y;

    // Our pushback is then just the inverse of the projection of the player's
    // position onto the unit normal: (p . un) -un
    //
    // TODO: Precompute the unit normals?
    //
    // We can avoid needing to compute a sqrt by using a non-unit normal; the
    // equation then becomes:
    //
    // -((p . n) / (n . n))n

    // We get n by applying a -90 deg CCW rotation matrix to (dx, dy):
    int16_t dx = next->x - w->x;
    int16_t dy = next->y - w->y;
    // [ cos -90 -sin -90 ] [ dx ]
    // [ sin -90  cos -90 ] [ dy ]
    // [  0 1 ] [ dx ]
    // [ -1 0 ] [ dy ]
    int16_t nx = dy;
    int16_t ny = -dx;

    // We can simulate a unit normal numerically without square root by scaling
    // onto the box -1, 1.
    Log lnx = nx;
    Log lny = ny;
    if (lnx.abs() >= lny.abs()) {
      lny /= lnx;
      lnx /= lnx;
    } else {
      lnx /= lny;
      lny /= lny;
    }
    // The norm squared then ranges from 1 to 2.
    Log norm_sq =
        (lnx * lnx * Log::pow2(12) + lny * lny * Log::pow2(12)) / Log::pow2(12);

    Log proj = -(Log(rel_x) * lnx + Log(rel_y) * lny) / norm_sq;

    // We want to compute dist = proj / ||n||, since that's the length of the
    // correction vector, which indicates whether or not collision should
    // trigger. But computing ||n|| requires a square root.  Instead, we can
    // square both sides and compute proj^2 / norm_sq.
    Log dist_sq = proj * proj / norm_sq;

    Log player_width = Log(30);
    if (dist_sq > -(player_width * player_width))
      continue;

    player.x += proj * lnx;
    player.y += proj * lny;
  }
}

void load_map(const Map &map) {
  player.x = (uint32_t)map.player_x << 8;
  player.y = (uint32_t)map.player_y << 8;
  player.z = (uint32_t)map.player_z << 8;
  player.ang = map.player_ang;
}
