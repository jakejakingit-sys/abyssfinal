#include "physics.h"
int floor_div(int a, int b) { int q = a / b; return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q; }
static bool is_solid(uint8_t t) { return t == T_SOLID || t == T_CRUMBLE || t == T_GATE; }
static bool box_solid(const LevelState *l, int x, int y) {
  int x0 = floor_div(x, TILE_SUB), x1 = floor_div(x + PW * SUBPX - 1, TILE_SUB);
  int y0 = floor_div(y, TILE_SUB), y1 = floor_div(y + PH * SUBPX - 1, TILE_SUB);
  for (int ty = y0; ty <= y1; ty++) for (int tx = x0; tx <= x1; tx++) if (is_solid(level_tile(l, tx, ty))) return true;
  return false;
}
bool player_overlaps(const Player *p, const LevelState *l, uint8_t tile) {
  int x0 = floor_div(p->x, TILE_SUB), x1 = floor_div(p->x + PW * SUBPX - 1, TILE_SUB);
  int y0 = floor_div(p->y, TILE_SUB), y1 = floor_div(p->y + PH * SUBPX - 1, TILE_SUB);
  for (int ty = y0; ty <= y1; ty++) for (int tx = x0; tx <= x1; tx++) if (level_tile(l, tx, ty) == tile) return true;
  return false;
}
/* Spikes only hurt where they are drawn: a 10x9 px box at the bottom of the tile. */
static bool spike_hit(const LevelState *l, int x, int y) {
  int x0 = floor_div(x, TILE_SUB), x1 = floor_div(x + PW * SUBPX - 1, TILE_SUB);
  int y0 = floor_div(y, TILE_SUB), y1 = floor_div(y + PH * SUBPX - 1, TILE_SUB);
  for (int ty = y0; ty <= y1; ty++) for (int tx = x0; tx <= x1; tx++) {
    if (level_tile(l, tx, ty) != T_SPIKE) continue;
    int sx0 = tx * TILE_SUB + 3 * SUBPX, sx1 = tx * TILE_SUB + 13 * SUBPX;
    int sy0 = ty * TILE_SUB + 8 * SUBPX, sy1 = (ty + 1) * TILE_SUB;
    if (x < sx1 && x + PW * SUBPX > sx0 && y < sy1 && y + PH * SUBPX > sy0) return true;
  }
  return false;
}
static bool oneway_land(const LevelState *l, int x, int old_bottom, int new_bottom) {
  int r0 = floor_div(old_bottom - 1, TILE_SUB), r1 = floor_div(new_bottom - 1, TILE_SUB);
  if (r0 == r1) return false;
  int x0 = floor_div(x, TILE_SUB), x1 = floor_div(x + PW * SUBPX - 1, TILE_SUB);
  for (int tx = x0; tx <= x1; tx++) if (level_tile(l, tx, r1) == T_ONEWAY) return true;
  return false;
}
static int plat_land(const LevelState *l, int x, int old_bottom, int new_bottom) {
  for (int i = 0; i < l->nplats; i++) {
    const MPlat *m = &l->plats[i]; int top = m->y * SUBPX;
    if (old_bottom <= top && new_bottom > top && x + PW * SUBPX > m->x * SUBPX && x < (m->x + PLAT_W) * SUBPX) return i + 1;
  }
  return 0;
}
void player_spawn(Player *p, const LevelDef *d) {
  *p = (Player){0};
  p->x = d->spawn_tx * TILE_SUB + (TILE_SUB - PW * SUBPX) / 2;
  p->y = (d->spawn_ty + 1) * TILE_SUB - PH * SUBPX;
  p->face = 1;
}
uint8_t player_step(Player *p, const LevelState *l, Input in) {
  uint8_t ev = 0;
  bool jpress = in.jump && !p->jheld, jrel = !in.jump && p->jheld;
  p->jheld = in.jump;
  if (p->on_plat && p->on_plat <= l->nplats && !p->ladder) {          /* ride the moving platform */
    int dx = l->plats[p->on_plat - 1].dx * SUBPX;
    if (!box_solid(l, p->x + dx, p->y)) p->x += dx;
  }
  int cx = floor_div(p->x + PW * SUBPX / 2, TILE_SUB), cy = floor_div(p->y + PH * SUBPX / 2, TILE_SUB);
  bool on_lad = level_tile(l, cx, cy) == T_LADDER;
  if (p->drop) p->drop--;
  if (!p->ladder && (in.up || in.down) && on_lad) { p->ladder = 1; p->vx = 0; }
  if (p->ladder && !on_lad) p->ladder = 0;
  if (p->ladder) {
    p->vy = in.up ? -CLIMB_V : in.down ? CLIMB_V : 0;
    p->vx = in.left ? -CLIMB_V : in.right ? CLIMB_V : 0;
    if (in.left) p->face = 0;
    if (in.right) p->face = 1;
    if (jpress) { p->ladder = 0; p->vy = JUMPV; }
  } else {
    if (in.left && !in.right) { p->vx -= ACC; p->face = 0; }
    else if (in.right && !in.left) { p->vx += ACC; p->face = 1; }
    else if (p->vx > 0) p->vx = p->vx > FRIC ? p->vx - FRIC : 0;
    else if (p->vx < 0) p->vx = p->vx < -FRIC ? p->vx + FRIC : 0;
    if (p->vx > MAXV) p->vx = MAXV;
    if (p->vx < -MAXV) p->vx = -MAXV;
    p->vy += GRAV; if (p->vy > MAXFALL) p->vy = MAXFALL;
    if (p->ground) p->coyote = COYOTE_F; else if (p->coyote) p->coyote--;
    if (jpress) p->jbuf = JBUF_F; else if (p->jbuf) p->jbuf--;
    if (in.down && p->ground) p->drop = 6;
    if (p->jbuf && p->coyote) { p->vy = JUMPV; p->jbuf = p->coyote = 0; }
    if (jrel && p->vy < JUMPCUT) p->vy = JUMPCUT;
  }
  int s = p->vx > 0 ? 1 : -1;                                          /* 1 sub-pixel at a time: no tunnelling */
  for (int n = p->vx < 0 ? -p->vx : p->vx; n > 0; n--) {
    if (box_solid(l, p->x + s, p->y)) { p->vx = 0; break; }
    p->x += s;
  }
  p->ground = 0; p->on_plat = 0;
  s = p->vy > 0 ? 1 : -1;
  for (int n = p->vy < 0 ? -p->vy : p->vy; n > 0; n--) {
    int ny = p->y + s, ob = p->y + PH * SUBPX, nb = ny + PH * SUBPX, pl = 0;
    if (s > 0 && !p->ladder && !p->drop) pl = plat_land(l, p->x, ob, nb);
    if (box_solid(l, p->x, ny) || pl || (s > 0 && !p->ladder && !p->drop && oneway_land(l, p->x, ob, nb))) {
      if (s > 0) { p->ground = 1; p->on_plat = pl; }
      p->vy = 0; break;
    }
    p->y = ny;
  }
  if (spike_hit(l, p->x, p->y)) ev |= EV_SPIKE;
  if (p->y > l->def->h * TILE_SUB) ev |= EV_PIT;
  return ev;
}
