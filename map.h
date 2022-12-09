#ifndef MAP_H
#define MAP_H

#include <stdint.h>

void to_vc(uint16_t x, uint16_t y, int16_t *vc_x, int16_t *vc_y);
void to_vc_z(uint16_t z, int16_t *vc_z);

void player_forward();
void player_backward();
void player_turn_left();
void player_turn_right();
void player_strafe_left();
void player_strafe_right();

#endif // not MAP_H
