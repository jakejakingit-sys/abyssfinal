#ifndef GAME_H
#define GAME_H
#include "physics.h"
#include "save.h"
#define EV_COIN 1
#define EV_HURT 2
#define EV_DEAD 4
#define EV_WIN 8
#define EV_RUNE 16
#define EV_GATE 32
#define EV_PULSE 64
#define MAX_CR 6
#define MAX_BONES 6
#define MAX_CRUMB 8
#define PULSE_COST 20
#define PULSE_CD 30
#define CRUMBLE_FRAMES 24
typedef struct { int x, y; int8_t dir; } Crawler;                 /* pixels */
typedef struct { int x, y; uint8_t t, on; } Bone;                 /* t>0: warning countdown, then falls */
typedef struct { uint16_t idx; uint8_t t; } Crumb;
typedef struct {
  Player p; LevelState ls; SaveData *save;
  int level, hp, energy;
  uint8_t invuln, regen_wait, pulse_cd, pulse_fx, bone_timer, energy_tick, ncr;
  bool pulse_prev, boss_done;
  uint16_t regen_tick, tick, rng, run_coins;
  Crawler cr[MAX_CR]; Bone bone[MAX_BONES]; Crumb crumb[MAX_CRUMB];
} Game;
void game_start_level(Game *g, SaveData *s, int level);
uint8_t game_update(Game *g, Input in);
int plat_off(unsigned tick);
#endif
