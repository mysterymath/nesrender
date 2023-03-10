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
  //for (uint16_t i = 0; i < sector->num_walls; i++) {
  uint16_t i = 0;
    Wall *w = &sector->walls[i];
    if (w->begin_loop)
      loop_begin = w;
    const Wall *next =
        (i + 1 == sector->num_walls || sector->walls[i + 1].begin_loop)
            ? loop_begin
            : &sector->walls[i + 1];

    printf("Player: %d %d\n", px, py);
    printf("Wall: %d %d -> %d %d\n", w->x, w->y, next->x, next->y);

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
    // position, produces the pushback vector.

    // Start by producing a non-unit normal vector, n.
    // We get n by applying a -90 deg CCW rotation matrix to (dx, dy):
    int16_t dx = next->x - w->x;
    int16_t dy = next->y - w->y;
    // [ cos -90 -sin -90 ] [ dx ]
    // [ sin -90  cos -90 ] [ dy ]
    // [  0 1 ] [ dx ]
    // [ -1 0 ] [ dy ]
    int16_t nx = dy;
    int16_t ny = -dx;
    printf("D: %d %d\n", dx, dy);
    printf("N: %d %d\n", nx, ny);

    // Now, we'd like the pushback function to be linear. It must thus be zero
    // at the origin, so set the origin one player's width along the normal
    // vector.

    int16_t rel_x = px - w->x;
    int16_t rel_y = py - w->y;

    printf("Rel: %d %d\n", rel_x, rel_y);

    // Our pushback is then just the inverse of the projection of the player's
    // position onto the unit normal: (p . un) -un
    //
    // TODO: Precompute the unit normals?
    //
    // We can avoid needing to compute a sqrt by using a non-unit normal; the
    // equation then becomes:
    //
    // -((p . n) / (n . n))n


    // We can simulate a unit normal numerically without square root by scaling
    // onto the box -1, 1.
    Log lnx = nx;
    Log lny = ny;
    if (lnx.abs() >= lny.abs()) {
      lny /= lnx.abs();
      lnx /= lnx.abs();
    } else {
      lnx /= lny.abs();
      lny /= lny.abs();
    }

    printf("lnx, lny: %d %d %d %d\n", lnx.sign, lnx.exp, lny.sign, lny.exp);

    // The norm squared then ranges from 1 to 2.
    Log norm_sq =
        (lnx * lnx * Log::pow2(12) + lny * lny * Log::pow2(12)) / Log::pow2(12);

    printf("norm_sq: %d %d\n", norm_sq.sign, norm_sq.exp);

    Log proj = -Log(Log(rel_x) * lnx + Log(rel_y) * lny) / norm_sq;

    printf("proj: %d %d\n", proj.sign, proj.exp);

    Log dist_sq = proj * proj / norm_sq;

    // We want to compute dist = proj / ||n||, since that's the length of the
    // correction vector, which indicates whether or not collision should
    // trigger. But computing ||n|| requires a square root.  Instead, we can
    // square both sides and compute proj^2 / norm_sq.
    //
    // TODO: This is only in range if proj < 256
    Log player_width = Log(30);
    if (proj.sign || proj * proj / norm_sq > player_width * player_width)
      //continue;
      return;

    player.x += proj * lnx;
    player.y += proj * lny;
  //}
}

void load_map(const Map &map) {
  player.x = (uint32_t)map.player_x << 8;
  player.y = (uint32_t)map.player_y << 8;
  player.z = (uint32_t)map.player_z << 8;
  player.ang = map.player_ang;
  player.sector = map.player_sector;
}
