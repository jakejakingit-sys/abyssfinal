#ifndef PHYSICS_H
#define PHYSICS_H

#include "level.h"

#define PW 10
#define PH 14

#define ACC 6
#define FRIC 6
#define MAXV 64

#define GRAV 9
#define JUMPV (-90)
#define JUMPCUT (-40)
#define MAXFALL 112

#define COYOTE_F 7
#define JBUF_F 7
#define CLIMB_V 24

#define EV_SPIKE 1
#define EV_PIT 2

typedef struct {
  int x, y, vx, vy;
  uint8_t ground, coyote, jbuf, jheld, ladder, drop, face, on_plat;
} Player;

typedef struct {
  bool left, right, up, down, jump, pulse;
} Input;

void player_spawn(Player *p, const LevelDef *d);
uint8_t player_step(Player *p, const LevelState *l, Input in);
int floor_div(int a, int b);
bool player_overlaps(const Player *p, const LevelState *l, uint8_t tile);

#endif
