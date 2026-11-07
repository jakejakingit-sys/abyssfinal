#include "level.h"
LevelDef LEVELS[LEVEL_COUNT];
bool level_available(int i) { return i >= 0 && i < LEVEL_COUNT && LEVELS[i].tiles != 0; }
int level_count(void) { int n = 0; for (int i = 0; i < LEVEL_COUNT; i++) n += LEVELS[i].tiles != 0; return n; }
static bool bit(const uint8_t *b, int i) { return (b[i >> 3] >> (i & 7)) & 1; }
static void setbit(uint8_t *b, int i) { b[i >> 3] |= 1 << (i & 7); }
uint8_t level_raw(const LevelDef *d, int tx, int ty) {
  if (tx < 0 || tx >= d->w || ty < 0) return T_SOLID;   /* walls + ceiling */
  if (ty >= d->h) return T_AIR;                          /* bottomless pit */
  return d->tiles[ty * d->w + tx];
}
static bool is_treasure(uint8_t t) { return t == T_COIN || t == T_CHEST; }
static uint8_t ram_raw(const LevelState *l, int tx, int ty) {
  if (tx < 0 || tx >= l->def->w || ty < 0) return T_SOLID;
  if (ty >= l->def->h) return T_AIR;
  return l->tiles[ty * l->def->w + tx];
}
uint8_t level_tile(const LevelState *l, int tx, int ty) {
  uint8_t t = ram_raw(l, tx, ty);
  int i = ty * l->def->w + tx;
  switch (t) {
    case T_COIN: case T_CHEST: if (bit(l->taken, i)) return T_AIR; break;
    case T_CRUMBLE: if (bit(l->rt, i)) return T_AIR; break;
    case T_RUNE: if (bit(l->rt, i)) return T_RUNE_ON; break;
    case T_GATE: if (l->gates_open) return T_AIR; break;
    default: break;
  }
  return t;
}
int level_treasure_count(const LevelDef *d) {
  int n = 0;
  for (int i = 0; i < d->w * d->h; i++) if (is_treasure(d->tiles[i])) n++;
  return n;
}
void level_reset_runtime(LevelState *l) {
  for (int i = 0; i < BITS_BYTES; i++) l->rt[i] = 0;
  l->runes_on = l->runes_total = 0; l->gates_open = false;
  for (int i = 0; i < l->def->w * l->def->h; i++) if (l->tiles[i] == T_RUNE) l->runes_total++;
}
void level_load(LevelState *l, int idx, uint32_t bits) {
  l->def = &LEVELS[idx]; l->nplats = 0;
  int n = l->def->w * l->def->h;
  for (int i = 0; i < n; i++) l->tiles[i] = l->def->tiles[i];     /* the one archive->RAM copy */
  for (int i = 0; i < BITS_BYTES; i++) l->taken[i] = 0;
  int k = 0;
  for (int i = 0; i < n; i++)
    if (is_treasure(l->tiles[i])) { if (k < MAX_TREASURE && (bits >> k) & 1) setbit(l->taken, i); k++; }
  level_reset_runtime(l);
}
uint32_t level_treasure_bits(const LevelState *l) {
  uint32_t bits = 0; int k = 0;
  for (int i = 0; i < l->def->w * l->def->h; i++)
    if (is_treasure(l->tiles[i])) { if (k < MAX_TREASURE && bit(l->taken, i)) bits |= 1UL << k; k++; }
  return bits;
}
int level_take(LevelState *l, int tx, int ty) {
  uint8_t t = level_tile(l, tx, ty);
  if (!is_treasure(t)) return 0;
  setbit(l->taken, ty * l->def->w + tx);
  return t == T_CHEST ? CHEST_VALUE : COIN_VALUE;
}
void level_break(LevelState *l, int tx, int ty) { setbit(l->rt, ty * l->def->w + tx); }
bool level_activate_rune(LevelState *l, int tx, int ty) {
  if (ram_raw(l, tx, ty) != T_RUNE) return false;
  int i = ty * l->def->w + tx;
  if (bit(l->rt, i)) return false;
  setbit(l->rt, i); l->runes_on++;
  if (l->runes_on == l->runes_total) l->gates_open = true;
  return true;
}
