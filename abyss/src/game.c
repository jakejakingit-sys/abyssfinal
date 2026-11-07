#include "game.h"
#include "abilities.h"
#include "upgrades.h"
#define SPIKE_DMG 20
#define CRAWLER_DMG 12
#define BONE_DMG 15
int plat_off(unsigned t) { unsigned v = t % 192; return v < 96 ? (int)v - 48 : 144 - (int)v; }
static uint16_t rnd(Game *g) { uint16_t x = g->rng; x ^= x << 7; x ^= x >> 9; x ^= x << 8; g->rng = x ? x : 0xACE1; return g->rng; }
static void reset_run(Game *g) {
  const LevelDef *d = g->ls.def;
  level_reset_runtime(&g->ls);
  player_spawn(&g->p, d);
  g->hp = upgrade_max_hp(g->save); g->energy = upgrade_max_energy(g->save);
  g->invuln = g->regen_wait = g->pulse_cd = g->pulse_fx = g->energy_tick = 0; g->tick = 0; g->bone_timer = 90;
  g->boss_done = false; g->pulse_prev = false; g->ncr = 0; g->ls.nplats = 0;
  for (int i = 0; i < MAX_CRUMB; i++) g->crumb[i].t = 0;
  for (int i = 0; i < MAX_BONES; i++) g->bone[i].on = 0;
  for (int i = 0; i < d->nent; i++) {
    const EntDef *e = &d->ents[i];
    if (e->type == ENT_CRAWLER && g->ncr < MAX_CR) { g->cr[g->ncr++] = (Crawler){e->tx * 16 + 1, e->ty * 16 + 6, 1}; }
    else if (e->type == ENT_PLATFORM && g->ls.nplats < MAX_PLATS) {
      MPlat *m = &g->ls.plats[g->ls.nplats++];
      m->base = e->tx * 16 - 8; m->y = e->ty * 16; m->x = m->base + plat_off(0); m->dx = 0;
    }
  }
}
void game_start_level(Game *g, SaveData *s, int level) {
  g->save = s; g->level = level; g->rng = 0xACE1; g->run_coins = 0; g->regen_tick = 0;
  level_load(&g->ls, level, s->treasure[level]);
  s->abilities |= abilities_for_level(level);
  reset_run(g);
}
static void hurt(Game *g, int dmg, uint8_t *ev) {
  dmg -= upgrade_armor(g->save); if (dmg < 1) dmg = 1;
  g->hp -= dmg; g->invuln = 45; g->regen_wait = 120;
  g->p.vy = -60; g->p.vx = g->p.face ? -MAXV : MAXV; g->p.ladder = 0;
  *ev |= EV_HURT;
}
static bool px_is(const LevelState *l, int px, int py, int a, int b, int c, int d) {
  uint8_t t = level_tile(l, floor_div(px, 16), floor_div(py, 16));
  return t == a || t == b || t == c || t == d;
}
static bool overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) { return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by; }
uint8_t game_update(Game *g, Input in) {
  uint8_t ev = 0; const LevelDef *d = g->ls.def; LevelState *l = &g->ls;
  g->tick++;
  for (int i = 0; i < l->nplats; i++) { MPlat *m = &l->plats[i]; int nx = m->base + plat_off(g->tick); m->dx = nx - m->x; m->x = nx; }
  uint8_t pe = player_step(&g->p, l, in);
  if (g->invuln) g->invuln--;
  if (g->regen_wait) g->regen_wait--;
  else if (g->hp > 0 && g->hp < upgrade_max_hp(g->save) && ++g->regen_tick >= 30) { g->hp++; g->regen_tick = 0; }
  if (g->energy < upgrade_max_energy(g->save) && ++g->energy_tick >= 8) { g->energy++; g->energy_tick = 0; }
  /* ---- Rune Pulse (ALPHA): activates ancient runes within 4 tiles ---- */
  bool pp = in.pulse && !g->pulse_prev; g->pulse_prev = in.pulse;
  if (g->pulse_cd) g->pulse_cd--;
  if (g->pulse_fx) g->pulse_fx--;
  if (pp && !g->pulse_cd && g->energy >= PULSE_COST) {
    bool was_open = l->gates_open; int n = 0;
    g->energy -= PULSE_COST; g->pulse_cd = PULSE_CD; g->pulse_fx = 12; ev |= EV_PULSE;
    int cx = floor_div(g->p.x + PW * SUBPX / 2, TILE_SUB), cy = floor_div(g->p.y + PH * SUBPX / 2, TILE_SUB);
    for (int ty = cy - 4; ty <= cy + 4; ty++) for (int tx = cx - 4; tx <= cx + 4; tx++) if (level_activate_rune(l, tx, ty)) n++;
    if (n) ev |= EV_RUNE;
    if (l->gates_open && !was_open) ev |= EV_GATE;
  }
  if (d->boss && l->gates_open) g->boss_done = true;
  /* ---- spikes / pits ---- */
  if ((pe & EV_SPIKE) && !g->invuln) hurt(g, SPIKE_DMG, &ev);
  if (pe & EV_PIT) g->hp = 0;
  /* ---- crumbling floor ---- */
  if (g->p.ground && !g->p.ladder) {
    int ty = floor_div(g->p.y + PH * SUBPX, TILE_SUB);
    int x0 = floor_div(g->p.x, TILE_SUB), x1 = floor_div(g->p.x + PW * SUBPX - 1, TILE_SUB);
    for (int tx = x0; tx <= x1; tx++) if (level_tile(l, tx, ty) == T_CRUMBLE) {
      int idx = ty * d->w + tx; bool have = false; int freeslot = -1;
      for (int i = 0; i < MAX_CRUMB; i++) { if (g->crumb[i].t && g->crumb[i].idx == idx) have = true; if (!g->crumb[i].t && freeslot < 0) freeslot = i; }
      if (!have && freeslot >= 0) { g->crumb[freeslot].idx = idx; g->crumb[freeslot].t = CRUMBLE_FRAMES; }
    }
  }
  for (int i = 0; i < MAX_CRUMB; i++) if (g->crumb[i].t && --g->crumb[i].t == 0) level_break(l, g->crumb[i].idx % d->w, g->crumb[i].idx / d->w);
  /* ---- crawlers: patrol, turn at walls/edges/spikes ---- */
  int ppx = g->p.x / SUBPX, ppy = g->p.y / SUBPX;
  for (int i = 0; i < g->ncr; i++) {
    Crawler *c = &g->cr[i];
    if (g->tick & 1) {
      int nx = c->x + c->dir, fx = c->dir > 0 ? nx + 13 : nx;
      bool wall = px_is(l, fx, c->y + 5, T_SOLID, T_CRUMBLE, T_GATE, T_SPIKE);
      bool floor_ = px_is(l, fx, c->y + 10, T_SOLID, T_ONEWAY, T_CRUMBLE, T_GATE);
      if (wall || !floor_) c->dir = -c->dir; else c->x = nx;
    }
    if (!g->invuln && overlap(c->x, c->y, 14, 10, ppx, ppy, PW, PH)) { g->p.face = c->x > ppx; hurt(g, CRAWLER_DMG, &ev); }
  }
  /* ---- guardian: falling bones until all runes are lit ---- */
  if (d->boss && !l->gates_open) {
    if (g->bone_timer) g->bone_timer--;
    else {
      for (int i = 0; i < MAX_BONES; i++) if (!g->bone[i].on) {
        g->bone[i] = (Bone){16 + rnd(g) % (d->w * 16 - 32), 16, 24, 1};
        break;
      }
      int per = 70 - 16 * l->runes_on; g->bone_timer = per < 30 ? 30 : per;
    }
  }
  for (int i = 0; i < MAX_BONES; i++) {
    Bone *b = &g->bone[i]; if (!b->on) continue;
    if (l->gates_open) { b->on = 0; continue; }
    if (b->t) { b->t--; continue; }
    b->y += 3;
    if (px_is(l, b->x + 3, b->y + 12, T_SOLID, T_ONEWAY, T_CRUMBLE, T_GATE)) { b->on = 0; continue; }
    if (overlap(b->x, b->y, 6, 12, ppx, ppy, PW, PH)) { b->on = 0; if (!g->invuln) hurt(g, BONE_DMG, &ev); }
  }
  /* ---- pickups ---- */
  int x0 = floor_div(g->p.x, TILE_SUB), x1 = floor_div(g->p.x + PW * SUBPX - 1, TILE_SUB);
  int y0 = floor_div(g->p.y, TILE_SUB), y1 = floor_div(g->p.y + PH * SUBPX - 1, TILE_SUB);
  for (int ty = y0; ty <= y1; ty++) for (int tx = x0; tx <= x1; tx++) {
    int v = level_take(l, tx, ty);
    if (v) {
      g->save->coins += v; if (g->save->coins > COINS_CAP) g->save->coins = COINS_CAP;
      g->run_coins += v; if (v == CHEST_VALUE) g->save->secrets++;
      g->save->treasure[g->level] = level_treasure_bits(l); ev |= EV_COIN;
    }
  }
  if (g->hp <= 0) {                      /* death: level restarts, coins + treasure records persist */
    g->save->deaths++; ev |= EV_DEAD; reset_run(g); return ev;
  }
  if (player_overlaps(&g->p, l, T_EXIT)) {
    g->save->completed |= 1UL << g->level;
    if (g->level + 1 < LEVEL_COUNT && g->save->unlocked < g->level + 1) g->save->unlocked = g->level + 1;
    if (d->boss && g->boss_done) g->save->guardians |= 1 << (d->world - 1);
    ev |= EV_WIN;
  }
  return ev;
}
