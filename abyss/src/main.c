/* ABYSS - CE front end. All art comes from the archived ABYSSCOR AppVar (read in place); levels from ABYSSLV1..4.
   Uses only graphx/keypadc/fileioc/timers calls. Never run on hardware by the author. */
#include <graphx.h>
#include <keypadc.h>
#include <tice.h>
#include <sys/timers.h>
#include <stdint.h>
#include "game.h"
#include "upgrades.h"
#include "abilities.h"
#include "animation.h"
#include "assets.h"
#include "platform.h"

#define FRAME_TICKS 546   /* 32768 Hz / 60 fps cap */
enum { C_BG1 = 1, C_BG2, C_BG3, C_STONE_D, C_STONE, C_STONE_L, C_STONE_H, C_PURPLE, C_PURPLE_D, C_MOSS, C_MOSS_D, C_SKIN, C_SKIN_D,
       C_CLOAK, C_CLOAK_L, C_CLOAK_D, C_RUNE, C_RUNE_D, C_RED, C_RED_D, C_GOLD, C_GOLD_D, C_WHITE, C_BROWN, C_BROWN_D, C_BONE, C_BONE_D,
       C_OUT, C_FIRE, C_FIRE_L, C_GREEN };
static const char *const WORLD_NAMES[4] = {"FORGOTTEN CRYPT", "SUNKEN MINES", "INFERNAL DEPTHS", "THE VOID"};
static const char *const LEVEL_NAMES[LEVEL_COUNT] = {
  "The Entrance", "Forgotten Halls", "Burial Grounds", "Bone Chambers", "Bone King's Throne",
  "Mine Entrance", "Crystal Caverns", "Collapsing Tunnels", "Deep Shafts", "Broodmother's Nest",
  "Ashen Path", "Burning Halls", "Magma Bridge", "Furnace Chambers", "Inferno's Arena",
  "Forgotten Gate", "Shattered Reality", "Lost Kingdom", "Abyssal Core", "The Forgotten One"};

static int camx, shake;
static uint16_t anim_tick, fx_rng = 0xBEEF;
static uint32_t frame_elapsed;

/* ================= basics ================= */
static uint16_t rnd(void) { fx_rng ^= fx_rng << 7; fx_rng ^= fx_rng >> 9; fx_rng ^= fx_rng << 8; return fx_rng; }
static void frame_wait(void) {
  while (timer_Get(1) < FRAME_TICKS) { }
  frame_elapsed = timer_Get(1);
  timer_Set(1, 0);
}
static void present(void) { frame_wait(); gfx_SwapDraw(); anim_tick++; }
static bool edge(bool now, bool *prev) { bool r = now && !*prev; *prev = now; return r; }
static Input read_input(void) {
  kb_Scan();
  Input in = { (kb_Data[7] & kb_Left) != 0, (kb_Data[7] & kb_Right) != 0, (kb_Data[7] & kb_Up) != 0, (kb_Data[7] & kb_Down) != 0,
               (kb_Data[1] & kb_2nd) != 0, (kb_Data[2] & kb_Alpha) != 0 };
  return in;
}
static void tsp(int id, int x, int y) { const uint8_t *p = spr(id); if (p) gfx_TransparentSprite((gfx_sprite_t *)p, x, y); }
static void osp(int id, int x, int y) { const uint8_t *p = spr(id); if (p) gfx_Sprite((gfx_sprite_t *)p, x, y); }
static void text(const char *s, int x, int y, uint8_t c) { gfx_SetTextFGColor(c); gfx_PrintStringXY(s, x, y); }
static int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }
static void text_c(const char *s, int cx, int y, uint8_t c) { text(s, cx - slen(s) * 4, y, c); }
static void num(int v, int x, int y, uint8_t c) { gfx_SetTextFGColor(c); gfx_SetTextXY(x, y); gfx_PrintUInt(v, 1); }
static void panel(int x, int y, int w, int h) {
  gfx_SetColor(C_OUT); gfx_FillRectangle(x, y, w, h); gfx_SetColor(C_STONE_L); gfx_Rectangle(x, y, w, h);
  gfx_SetColor(C_BG2); gfx_FillRectangle(x + 2, y + 2, w - 4, h - 4); gfx_SetColor(C_STONE_D); gfx_Rectangle(x + 1, y + 1, w - 2, h - 2);
}
static int popcount32(uint32_t v) { int n = 0; while (v) { n += v & 1; v >>= 1; } return n; }

/* ================= palette: per-world themes + fades ================= */
static uint8_t cur_pal[32][3];
static void apply_palette(int f) {
  static uint16_t p16[32];
  for (int i = 0; i < 32; i++) p16[i] = gfx_RGBTo1555(cur_pal[i][0] * f / 16, cur_pal[i][1] * f / 16, cur_pal[i][2] * f / 16);
  gfx_SetPalette(p16, sizeof p16, 0);
}
static void set_theme(int world, int bright) {
  const uint8_t *bp = base_palette(), *th = theme_colors(world);
  for (int i = 0; i < 32; i++) for (int k = 0; k < 3; k++) cur_pal[i][k] = bp[i * 3 + k];
  for (int i = 0; i < 11; i++) for (int k = 0; k < 3; k++) cur_pal[i + 1][k] = th[i * 3 + k];
  apply_palette(bright);
}
static void fade_in(void) { for (int f = 0; f <= 16; f += 2) { apply_palette(f); frame_wait(); } }
static void fade_out(void) { for (int f = 16; f >= 0; f -= 2) { apply_palette(f); frame_wait(); } }

/* ================= particles ================= */
typedef struct { int16_t x, y, vx, vy; uint8_t life, col, sz, grav; } Part;
static Part parts[56];
static void spawn(int px, int py, int vx4, int vy4, int life, int col, int sz, int grav) {
  for (int i = 0; i < 56; i++) if (!parts[i].life) { parts[i] = (Part){px * 4, py * 4, vx4, vy4, life, col, sz, grav}; return; }
}
static void burst(int px, int py, int n, int col, int spd, int life, int grav, int sz) {
  for (int i = 0; i < n; i++) spawn(px, py, (int)(rnd() % (2 * spd + 1)) - spd, (int)(rnd() % (2 * spd + 1)) - spd - spd / 2, life / 2 + rnd() % (life / 2 + 1), col, sz, grav);
}
static void parts_draw(int cam) {
  for (int i = 0; i < 56; i++) {
    Part *p = &parts[i]; if (!p->life) continue;
    p->x += p->vx; p->y += p->vy; if (p->grav && (anim_tick & 1)) p->vy += p->grav; p->life--;
    int sx = p->x / 4 - cam, sy = p->y / 4;
    if (sx >= 0 && sx < 320 && sy >= 0 && sy < 240) { gfx_SetColor(p->col); gfx_FillRectangle(sx, sy, p->sz, p->sz); }
  }
}
static void parts_clear(void) { for (int i = 0; i < 56; i++) parts[i].life = 0; }

/* ================= backgrounds ================= */
static void draw_background(int cam) {
  gfx_FillScreen(C_BG1);
  gfx_SetColor(C_BG2); gfx_FillRectangle(0, 60, 320, 180);
  for (int i = -1; i < 4; i++) tsp(SPR_BG_ARCH, i * 130 - (cam / 6) % 130, 128);
  for (int i = -1; i < 5; i++) {
    int x = i * 96 - (cam / 3) % 96; tsp(SPR_BG_PILLAR, x, 112);
    if (((i + cam / 288) & 1) == 0) tsp(SPR_BG_CHAIN, x + 54, 14);
  }
  gfx_SetColor(C_BG3); for (int y = 196; y < 240; y += 2) gfx_HorizLine(0, y, 320);
  gfx_SetColor(C_BG1); gfx_FillRectangle(0, 0, 320, 12);
}

/* ================= world rendering ================= */
static bool is_wall(uint8_t t) { return t == T_SOLID || t == T_GATE || t == T_CRUMBLE; }
static void draw_tiles(const Game *g) {
  const LevelState *l = &g->ls; const LevelDef *d = l->def; int t0 = camx / TILE_PX;
  for (int ty = 0; ty < d->h; ty++)
    for (int tx = t0; tx <= t0 + 20 && tx < d->w; tx++)
      if (level_tile(l, tx, ty) == T_TORCH) {
        tsp(SPR_GLOW, tx * TILE_PX + 8 - 16 - camx, ty * TILE_PX + 4 - 16);
        if (((anim_tick + tx * 7) & 15) == 0) spawn(tx * TILE_PX + 7 + (int)(rnd() % 3), ty * TILE_PX + 2, (int)(rnd() % 3) - 1, -3, 30, C_FIRE_L, 1, 0);
      }
  for (int ty = 0; ty < d->h; ty++)
    for (int tx = t0; tx <= t0 + 20 && tx < d->w; tx++) {
      uint8_t t = level_tile(l, tx, ty); if (!t) continue;
      int x = tx * TILE_PX - camx, y = ty * TILE_PX, id = -1; bool opaque = false;
      switch (t) {
        case T_SOLID: {
          bool up = is_wall(level_tile(l, tx, ty - 1)), dn = ty + 1 >= d->h || is_wall(level_tile(l, tx, ty + 1));
          if (!up) id = ((tx + ty) & 1) ? SPR_SOLID_TOP_B : SPR_SOLID_TOP_A;
          else if (!dn) id = SPR_SOLID_UNDER;
          else if (ty >= 2 && is_wall(level_tile(l, tx, ty - 2)) && ((tx * 7 + ty * 3) % 3)) id = SPR_SOLID_DEEP;
          else id = ((tx + ty) & 1) ? SPR_SOLID_B : SPR_SOLID_A;
          opaque = true; } break;
        case T_CRUMBLE: id = SPR_CRUMBLE; opaque = true; break;
        case T_ONEWAY: id = SPR_ONEWAY; break;
        case T_SPIKE: id = SPR_SPIKE; break;
        case T_LADDER: id = SPR_LADDER; break;
        case T_COIN: id = SPR_COIN0 + ((anim_tick / 6 + tx) & 3); break;
        case T_CHEST: id = SPR_CHEST; break;
        case T_EXIT: id = SPR_EXIT0 + ((anim_tick / 14) & 1); break;
        case T_RUNE: id = SPR_RUNE_OFF; break;
        case T_RUNE_ON: id = SPR_RUNE_ON; if (((anim_tick + tx * 5) & 31) == 0) spawn(tx * TILE_PX + 8, ty * TILE_PX + 4, 0, -2, 24, C_RUNE, 1, 0); break;
        case T_GATE: id = SPR_GATE; break;
        case T_TORCH: id = SPR_TORCH0 + ((anim_tick / 7 + tx) & 1); break;
        case T_SKULL: id = SPR_SKULL; break;
        default: break;
      }
      if (id < 0) continue;
      if (opaque) osp(id, x, y); else tsp(id, x, y);
    }
}
static int boss_t;
static void draw_boss(const Game *g) {
  const LevelDef *d = g->ls.def; if (!d->boss) return;
  int base = d->boss == 1 ? SPR_KING0 : SPR_BROOD0, w = spr(base)[0];
  if (!g->boss_done) {
    bool attack = false; for (int i = 0; i < MAX_BONES; i++) if (g->bone[i].on && g->bone[i].t) attack = true;
    int f = attack ? 2 : ((anim_tick / 26) & 1);
    tsp(base + f, d->w * 8 - w / 2 - camx, 18 + ((anim_tick / 13) & 1) * 2);
  } else if (boss_t > 0) {
    if ((boss_t / 3) & 1 || boss_t > 50) tsp(base + 2, d->w * 8 - w / 2 - camx + (int)(rnd() % 5) - 2, 18 + (90 - boss_t) / 3);
    if ((anim_tick & 3) == 0) burst(d->w * 8 + (int)(rnd() % 40) - 20, 30 + (90 - boss_t) / 3, 3, d->boss == 1 ? C_BONE : C_BROWN, 6, 40, 1, 2);
    boss_t--;
  }
}
static void draw_entities(const Game *g, int pframe) {
  const LevelState *l = &g->ls; const LevelDef *d = l->def;
  draw_boss(g);
  for (int i = 0; i < l->nplats; i++) {
    int x = l->plats[i].x - camx, y = l->plats[i].y;
    gfx_SetColor(C_OUT); gfx_FillRectangle(x - 1, y - 1, PLAT_W + 2, 9);
    gfx_SetColor(C_PURPLE_D); gfx_FillRectangle(x, y, PLAT_W, 6);
    gfx_SetColor(C_PURPLE); gfx_HorizLine_NoClip(x, y, PLAT_W); gfx_SetColor(C_STONE_H);
    gfx_FillRectangle(x + 3, y + 2, 2, 2); gfx_FillRectangle(x + 15, y + 2, 2, 2); gfx_FillRectangle(x + 27, y + 2, 2, 2);
    gfx_SetColor(C_RUNE_D); gfx_FillRectangle(x + 12, y + 6, 8, 2);
  }
  for (int i = 0; i < g->ncr; i++) { const Crawler *c = &g->cr[i]; tsp(SPR_CRAWLER(c->dir > 0 ? 0 : 1, (anim_tick / 8) & 1), c->x - camx, c->y); }
  for (int i = 0; i < MAX_BONES; i++) {
    const Bone *b = &g->bone[i]; if (!b->on) continue;
    if (b->t) {
      if ((b->t / 3) & 1) { gfx_SetColor(C_RED); gfx_FillRectangle(b->x - camx - 1, 14, 8, 4); gfx_SetColor(C_FIRE_L); gfx_FillRectangle(b->x - camx + 1, 15, 4, 2); }
    } else tsp(d->boss == 2 ? SPR_SHARD : SPR_BONE, b->x - camx, b->y);
  }
  if (!(g->invuln & 4) || g->invuln < 30) tsp(SPR_PLAYER(g->p.face ? 0 : 1, pframe), g->p.x / SUBPX - 1 - camx, g->p.y / SUBPX - 2);
  if (g->pulse_fx) {
    int r = (12 - g->pulse_fx) * 6 + 6, cx = g->p.x / SUBPX + PW / 2 - camx, cy = g->p.y / SUBPX + PH / 2;
    gfx_SetColor(C_RUNE); gfx_Rectangle(cx - r, cy - r, r * 2, r * 2); gfx_SetColor(C_WHITE); gfx_Rectangle(cx - r + 2, cy - r + 2, r * 2 - 4, r * 2 - 4);
  }
}
static void draw_hud(const Game *g, const SaveData *s) {
  const LevelDef *d = g->ls.def; int mh = upgrade_max_hp(s), me = upgrade_max_energy(s);
  tsp(SPR_HEART, 5, 3);
  gfx_SetColor(C_OUT); gfx_FillRectangle(16, 3, 84, 10);
  gfx_SetColor(C_RED_D); gfx_FillRectangle(17, 4, 82, 8); gfx_SetColor(g->hp * 4 < mh && (anim_tick & 8) ? C_FIRE : C_RED); gfx_FillRectangle(17, 4, g->hp * 82 / mh, 8);
  gfx_SetColor(C_WHITE); gfx_HorizLine_NoClip(17, 4, g->hp * 82 / mh);
  tsp(SPR_BOLT, 6, 14);
  gfx_SetColor(C_OUT); gfx_FillRectangle(16, 15, 84, 7);
  gfx_SetColor(C_CLOAK_D); gfx_FillRectangle(17, 16, 82, 5); gfx_SetColor(C_RUNE); gfx_FillRectangle(17, 16, g->energy * 82 / me, 5);
  for (int k = 1; k < me / 20; k++) { gfx_SetColor(C_OUT); gfx_VertLine_NoClip(17 + k * 20 * 82 / me, 16, 5); }
  tsp(SPR_COIN0 + ((anim_tick / 6) & 3), 258, 0); num(s->coins, 276, 4, C_GOLD);
  bool ready = g->energy >= PULSE_COST && !g->pulse_cd;
  tsp(ready ? SPR_PULSE_ON : SPR_PULSE_OFF, 4, 220);
  if (g->pulse_cd) { gfx_SetColor(C_BG1); gfx_FillRectangle(5, 221, 16, g->pulse_cd * 16 / PULSE_CD); }
  if (g->ls.runes_total) {
    for (int i = 0; i < g->ls.runes_total; i++) { tsp(i < g->ls.runes_on ? SPR_RUNE_ON : SPR_RUNE_OFF, 124 + i * 14 - 4, -3); }
    if (g->ls.gates_open) text("GATE OPEN", 190, 4, C_GREEN);
  }
  (void)d;
}
static void render(const Game *g, const SaveData *s, int pframe) {
  const LevelDef *d = g->ls.def; int target = g->p.x / SUBPX + PW / 2 - 160 + (g->p.face ? 24 : -24), maxcam = d->w * TILE_PX - 320;
  if (target > maxcam) target = maxcam;
  if (target < 0) target = 0;
  if (g->tick <= 1) camx = target; else camx += (target - camx) / 4;
  int sh = 0; if (shake) { sh = (anim_tick & 1) ? 2 + shake / 6 : -2 - shake / 6; shake--; }
  int saved = camx; camx += sh; if (camx < 0) camx = 0;
  draw_background(camx); draw_tiles(g); draw_entities(g, pframe); parts_draw(camx); draw_hud(g, s);
  camx = saved;
  if (g->tick < 120) {
    panel(50, 176, 220, 44); text_c(WORLD_NAMES[d->world - 1], 160, 181, C_STONE_H);
    text_c(d->name, 160, 194, C_GOLD); text_c(d->boss ? "GUARDIAN - light every rune!" : "Reach the exit", 160, 207, d->boss ? C_RED : C_STONE_L);
  }
}

/* ================= screens ================= */
static bool pause_menu(void) {
  bool pe = false, pc = true;
  for (;;) {
    kb_Scan();
    if (edge(kb_Data[6] & kb_Clear, &pc)) return false;
    if (edge(kb_Data[6] & kb_Enter, &pe)) return true;
    panel(70, 88, 180, 64); text_c("PAUSED", 160, 96, C_GOLD);
    text_c("CLEAR  resume", 160, 116, C_WHITE); text_c("ENTER  leave level", 160, 130, C_WHITE);
    present();
  }
}
static bool screen_complete(const Game *g, const SaveData *s, bool has_next) {
  bool pe, pc; const LevelDef *d = g->ls.def; int lv = g->level, found = popcount32(s->treasure[lv]), total = level_treasure_count(d);
  kb_Scan(); pe = kb_Data[6] & kb_Enter; pc = kb_Data[6] & kb_Clear;
  for (int t = 0;; t++) {
    kb_Scan();
    if (edge(kb_Data[6] & kb_Enter, &pe) && t > 25) return has_next;
    if (edge(kb_Data[6] & kb_Clear, &pc) && t > 25) return false;
    draw_background(camx); draw_tiles(g); draw_hud(g, s); parts_draw(camx);
    int ph = t < 20 ? 150 - t * 5 : 50; panel(40, ph, 240, 150 - (ph - 50) / 2);
    if (t >= 10) {
      text_c(d->boss ? "GUARDIAN DEFEATED!" : "LEVEL COMPLETE", 160, 60, d->boss ? C_RUNE : C_GREEN);
      text_c(d->name, 160, 78, C_GOLD);
      text("Coins gained", 62, 102, C_WHITE); num(g->run_coins, 214, 102, C_GOLD);
      text("Treasure", 62, 118, C_WHITE);
      for (int i = 0; i < total && i < 16; i++) { gfx_SetColor(C_OUT); gfx_FillRectangle(150 + i * 8, 118, 7, 7); gfx_SetColor(i < found ? C_GOLD : C_STONE_D); gfx_FillRectangle(151 + i * 8, 119, 5, 5); }
      text("Deaths so far", 62, 134, C_WHITE); num(s->deaths, 214, 134, C_STONE_H);
      if (d->boss) text_c("WORLD CLEARED", 160, 152, C_RUNE);
      text_c(has_next ? "ENTER next level    CLEAR map" : "ENTER / CLEAR   back to map", 160, 172, C_STONE_H);
    }
    present();
  }
}

/* returns true if the level was completed and the player wants the next one */
static bool play_level(SaveData *s, int idx) {
  static Game g; game_start_level(&g, s, idx);
  set_theme(g.ls.def->world - 1, 0); parts_clear(); shake = 0; boss_t = 0;
  bool pc = true, prev_ground = true, prev_boss = false; int prev_vy = 0, land_t = 0, ppx = 0, ppy = 0; bool prev_bone[MAX_BONES] = {0};
  timer_Set(1, 0);
  render(&g, s, FR_IDLE0); gfx_SwapDraw(); fade_in();
  uint32_t sim_accum = 0;
  frame_elapsed = 0;
  timer_Set(1, 0);
  for (;;) {
    Input in = read_input();
    if (edge(kb_Data[6] & kb_Clear, &pc)) {
      if (pause_menu()) { fade_out(); platform_save_write(s); return false; }
      sim_accum = 0; frame_elapsed = 0; timer_Set(1, 0);
    }

    sim_accum += frame_elapsed;
    frame_elapsed = 0;
    uint8_t ev = 0;
    while (sim_accum >= FRAME_TICKS) {
      ppx = g.p.x / SUBPX;
      ppy = g.p.y / SUBPX;
      ev |= game_update(&g, in);
      sim_accum -= FRAME_TICKS;
    }

    int fx = g.p.x / SUBPX + PW / 2, fy = g.p.y / SUBPX + PH;
    if (g.p.ground && (g.p.vx > 30 || g.p.vx < -30) && (g.tick % 7) == 0) spawn(fx - (g.p.vx > 0 ? 4 : -4), fy - 1, (g.p.vx > 0 ? -2 : 2), -1, 14, C_STONE_L, 2, 0);
    if (prev_ground && !g.p.ground && g.p.vy < 0) { burst(fx, fy, 5, C_STONE_L, 4, 16, 0, 2); }
    if (!prev_ground && g.p.ground && prev_vy >= 45) { land_t = 6; burst(fx, fy, 8, C_STONE_L, 6, 18, 0, 2); }
    prev_ground = g.p.ground; prev_vy = g.p.vy; if (land_t) land_t--;
    if (ev & EV_COIN) burst(fx, fy - 8, 7, C_GOLD, 5, 22, 1, 1);
    if (ev & EV_HURT) { shake = 10; burst(fx, fy - 7, 10, C_RED, 8, 24, 1, 2); }
    if (ev & EV_PULSE) burst(fx, fy - 7, 12, C_RUNE, 9, 20, 0, 1);
    if (ev & EV_RUNE) { shake = 6; burst(fx, fy - 7, 16, C_WHITE, 10, 26, 0, 2); }
    if (ev & EV_GATE) shake = 14;
    if (ev & EV_DEAD) { shake = 16; burst(ppx + 5, ppy + 7, 30, C_CLOAK_L, 12, 40, 1, 2); }
    if (g.boss_done && !prev_boss) { boss_t = 90; shake = 26; }
    prev_boss = g.boss_done;
    for (int i = 0; i < MAX_BONES; i++) {
      if (prev_bone[i] && !g.bone[i].on && !g.boss_done && !(ev & EV_HURT)) burst(g.bone[i].x + 3, g.bone[i].y + 12, 5, g.ls.def->boss == 2 ? C_RUNE : C_BONE, 6, 16, 1, 2);
      prev_bone[i] = g.bone[i].on && !g.bone[i].t;
    }
    for (int i = 0; i < MAX_CRUMB; i++) if (g.crumb[i].t > 0 && g.crumb[i].t < CRUMBLE_FRAMES && (g.crumb[i].t & 3) == 0)
      spawn((g.crumb[i].idx % g.ls.def->w) * 16 + 4 + (int)(rnd() % 8), (g.crumb[i].idx / g.ls.def->w) * 16 + 14, 0, 2, 12, C_STONE_L, 2, 1);
    int pf = anim_player_frame(&g.p, g.tick);
    if (g.invuln > 36) pf = FR_HURT; else if (land_t && g.p.ground) pf = FR_LAND;
    if (ev & EV_WIN) {
      platform_save_write(&*s); render(&g, s, FR_IDLE0);
      burst(g.p.x / SUBPX + 5, g.p.y / SUBPX + 5, 24, C_GREEN, 10, 40, 1, 2);
      bool next = screen_complete(&g, s, level_available(idx + 1)); fade_out(); return next;
    }
    render(&g, s, pf); present();
  }
}
static void screen_levels(SaveData *s) {
  int sel = s->unlocked; if (!level_available(sel)) sel = 0;
  bool pl = true, pr = true, pu = true, pd = true, pe = true, pc = true; int w_prev = -1;
  for (;;) {
    kb_Scan();
    if (edge(kb_Data[6] & kb_Clear, &pc)) return;
    if (edge(kb_Data[7] & kb_Left, &pl) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Right, &pr) && sel < LEVEL_COUNT - 1) sel++;
    if (edge(kb_Data[7] & kb_Up, &pu) && sel >= 5) sel -= 5;
    if (edge(kb_Data[7] & kb_Down, &pd) && sel < LEVEL_COUNT - 5) sel += 5;
    if (sel / 5 != w_prev) { w_prev = sel / 5; set_theme(w_prev, 16); }
    bool avail = level_available(sel), open = avail && sel <= s->unlocked;
    if (edge(kb_Data[6] & kb_Enter, &pe) && open) {
      fade_out(); int cur = sel;
      while (play_level(s, cur)) { cur++; if (!level_available(cur) || cur > s->unlocked) break; }
      sel = level_available(cur) && cur <= s->unlocked ? cur : (level_available(cur - 1) ? cur - 1 : sel); if (sel < 0) sel = 0;
      pe = true; pc = true; w_prev = -1; continue;
    }
    draw_background(anim_tick); text_c("SELECT LEVEL", 160, 5, C_GOLD);
    for (int w = 0; w < 4; w++) {
      int y = 20 + w * 50; bool cur = w == sel / 5;
      gfx_SetColor(cur ? C_STONE_L : C_STONE_D); gfx_HorizLine(8, y, 304);
      text(WORLD_NAMES[w], 10, y + 3, cur ? C_GOLD : C_STONE_L);
      for (int k = 0; k < 5; k++) {
        int i = w * 5 + k, x = 12 + k * 61, by = y + 16; bool done = (s->completed >> i) & 1, ok = level_available(i) && i <= s->unlocked;
        gfx_SetColor(C_OUT); gfx_FillRectangle(x, by, 54, 26);
        gfx_SetColor(done ? C_GREEN : ok ? C_STONE_L : C_STONE_D); gfx_FillRectangle(x + 1, by + 1, 52, 24);
        gfx_SetColor(done ? C_MOSS_D : ok ? C_STONE : C_BG3); gfx_FillRectangle(x + 3, by + 3, 48, 20);
        if (i == sel) { gfx_SetColor(C_GOLD); gfx_Rectangle(x - 2, by - 2, 58, 30); gfx_Rectangle(x - 1, by - 1, 56, 28); }
        gfx_SetTextFGColor(ok ? C_WHITE : C_STONE); gfx_SetTextXY(x + (i + 1 < 10 ? 22 : 18), by + 9); gfx_PrintUInt(i + 1, 1);
        if (k == 4) tsp(SPR_HEART, x + 40, by + 4);
        if (done) text("OK", x + 4, by + 4, C_GOLD);
        else if (!ok) text(level_available(i) ? "LOCK" : "SOON", x + 4, by + 4, C_STONE_L);
      }
    }
    panel(8, 214, 304, 22); text(level_available(sel) ? LEVELS[sel].name : LEVEL_NAMES[sel], 14, 220, C_GOLD);
    if (!avail) text("Coming soon", 214, 220, C_STONE_L); else if (!open) text("Locked", 230, 220, C_RED); else text("ENTER play", 214, 220, C_GREEN);
    present();
  }
}
static void screen_upgrades(SaveData *s) {
  static const char *N[3] = {"ARMOR", "MAX HEALTH", "ENERGY"}; static const char *D[3] = {"-2 damage per level", "+20 max HP per level", "+20 energy per level"};
  int sel = 0, flash = 0; bool pu = true, pd = true, pe = true, pc = true;
  set_theme(0, 16);
  for (;;) {
    kb_Scan();
    if (edge(kb_Data[6] & kb_Clear, &pc)) { platform_save_write(s); return; }
    if (edge(kb_Data[7] & kb_Up, &pu) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Down, &pd) && sel < 2) sel++;
    if (edge(kb_Data[6] & kb_Enter, &pe)) flash = upgrade_buy(s, (UpgCat)sel) == BUY_OK ? 14 : -14;
    draw_background(anim_tick); text_c("UPGRADES", 160, 8, C_GOLD);
    tsp(SPR_COIN0 + ((anim_tick / 6) & 3), 112, 24); num(s->coins, 132, 29, C_GOLD); gfx_PrintString(" coins");
    for (int i = 0; i < 3; i++) {
      int y = 54 + i * 52; panel(20, y, 280, 44);
      if (i == sel) { gfx_SetColor(C_GOLD); gfx_Rectangle(18, y - 2, 284, 48); }
      text(N[i], 30, y + 6, i == sel ? C_GOLD : C_WHITE); text(D[i], 30, y + 24, C_STONE_L);
      for (int p = 0; p < UPG_MAX_LEVEL; p++) { gfx_SetColor(C_OUT); gfx_FillRectangle(170 + p * 16, y + 8, 13, 10); gfx_SetColor(p < s->upg[i] ? C_RUNE : C_STONE_D); gfx_FillRectangle(171 + p * 16, y + 9, 11, 8); }
      int c = upgrade_cost((UpgCat)i, s->upg[i]);
      if (c < 0) text("MAXED", 190, y + 26, C_GREEN); else { text("Cost", 190, y + 26, C_WHITE); num(c, 230, y + 26, s->coins >= (uint32_t)c ? C_GOLD : C_RED); }
    }
    if (flash > 0) { text_c("UPGRADED!", 160, 222, C_GREEN); flash--; } else if (flash < 0) { text_c("Not available", 160, 222, C_RED); flash++; }
    else text_c("UP/DOWN select   ENTER buy   CLEAR back", 160, 222, C_STONE_L);
    present();
  }
}
static void screen_info(const SaveData *s, bool collection) {
  bool pc = true; int done = 0, found = 0, total = 0, built = 0;
  for (int i = 0; i < LEVEL_COUNT; i++) { done += (s->completed >> i) & 1; if (level_available(i)) { built++; found += popcount32(s->treasure[i]); total += level_treasure_count(&LEVELS[i]); } }
  set_theme(0, 16);
  for (;;) {
    kb_Scan(); if (edge(kb_Data[6] & kb_Clear, &pc)) return;
    draw_background(anim_tick); text_c(collection ? "COLLECTION" : "STATS", 160, 8, C_GOLD);
    panel(20, 26, 280, 192);
    if (!collection) {
      text("Levels completed", 34, 42, C_WHITE); num(done, 230, 42, C_GOLD); gfx_PrintString(" / 20");
      text("Guardians defeated", 34, 62, C_WHITE); num(popcount32(s->guardians), 230, 62, C_GOLD); gfx_PrintString(" / 4");
      text("Total deaths", 34, 82, C_WHITE); num(s->deaths, 230, 82, C_RED);
      text("Treasures found", 34, 102, C_WHITE); num(found, 230, 102, C_GOLD); gfx_PrintString(" / "); gfx_PrintUInt(total, 1);
      text("Secret chests", 34, 122, C_WHITE); num(s->secrets, 230, 122, C_GOLD);
      text("Highest level", 34, 142, C_WHITE); num(s->unlocked + 1, 230, 142, C_GOLD);
    } else {
      for (int i = 0; i < ABILITY_COUNT; i++) {
        bool have = (s->abilities >> i) & 1; int y = 36 + i * 14;
        text(have ? ABILITY_NAMES[i] : "??????", 34, y, have ? C_RUNE : C_STONE);
        if (have) text("unlocked", 200, y, C_GREEN); else { text("level", 200, y, C_STONE); gfx_PrintString(" "); gfx_PrintUInt(ABILITY_LEVEL[i], 1); }
      }
      int pct = built ? (done + found) * 100 / (built + total) : 0;
      text("Treasure records", 34, 142, C_WHITE); num(found, 230, 142, C_GOLD); gfx_PrintString("/"); gfx_PrintUInt(total, 1);
      text("Completion (built levels)", 34, 160, C_WHITE); num(pct, 250, 160, C_GOLD); gfx_PrintString("%");
      text("Relics: not implemented yet", 34, 190, C_STONE_L);
    }
    present();
  }
}
static void missing_assets(void) {
  gfx_FillScreen(255); gfx_SetTextFGColor(0);
  gfx_PrintStringXY("ABYSS: game data not found", 20, 30);
  gfx_PrintStringXY(assets_status() == ASSETS_BAD_CORE ? "ABYSSCOR is damaged. Send it again." : "ABYSSCOR.8xv is missing.", 20, 50);
  gfx_PrintStringXY("Send these files to the calculator:", 20, 80);
  gfx_PrintStringXY("ABYSSCOR.8xv  ABYSSLV1.8xv  (and any", 20, 96);
  gfx_PrintStringXY("other ABYSSLV*.8xv), then run ABYSS.", 20, 108);
  gfx_PrintStringXY("Press any key.", 20, 150);
  gfx_SwapDraw();
  do kb_Scan(); while (!kb_AnyKey());
  do kb_Scan(); while (kb_AnyKey());
}
int main(void) {
  static SaveData save;
  gfx_Begin(); gfx_SetDrawBuffer(); gfx_SetTextScale(1, 1);
  if (!assets_init()) { missing_assets(); gfx_End(); return 1; }
  platform_save_load(&save);
  gfx_SetTransparentColor(0);
  timer_Enable(1, TIMER_32K, TIMER_NOINT, TIMER_UP); timer_Set(1, 0);
  static const char *M[4] = {"PLAY", "UPGRADES", "COLLECTION", "STATS"};
  int sel = 0; bool pu = true, pd = true, pe = true;
  set_theme(0, 0); parts_clear();
  for (int f = 0; f < 3; f++) { draw_background(0); gfx_SwapDraw(); }
  fade_in();
  for (;;) {
    kb_Scan(); if (kb_Data[6] & kb_Clear) break;
    if (edge(kb_Data[7] & kb_Up, &pu) && sel > 0) sel--;
    if (edge(kb_Data[7] & kb_Down, &pd) && sel < 3) sel++;
    if (edge(kb_Data[6] & kb_Enter, &pe)) {
      if (sel == 0) screen_levels(&save); else if (sel == 1) screen_upgrades(&save); else screen_info(&save, sel == 2);
      set_theme(0, 16); timer_Set(1, 0); pe = true;
    }
    int cam = anim_tick * 2;
    draw_background(cam);
    if ((anim_tick & 3) == 0) spawn(cam + (int)(rnd() % 320), 238, (int)(rnd() % 3) - 1, -3 - (int)(rnd() % 3), 90, (rnd() & 1) ? C_FIRE : C_RUNE_D, 1, 0);
    for (int x = 0; x < 20; x++) osp(SPR_SOLID_TOP_A + (x & 1), x * 16, 224);
    tsp(SPR_GLOW, 24, 190); tsp(SPR_GLOW, 264, 190);
    tsp(SPR_TORCH0 + ((anim_tick / 7) & 1), 40, 208); tsp(SPR_TORCH0 + ((anim_tick / 7 + 1) & 1), 264, 208);
    tsp(SPR_PLAYER(0, FR_IDLE0 + ((anim_tick / 28) & 1)), 154, 208);
    parts_draw(cam);
    tsp(SPR_LOGO, (320 - spr(SPR_LOGO)[0]) / 2, 14 + (((anim_tick / 20) & 1)));
    text_c("THE WAYSTATION", 160, 64, C_STONE_H);
    panel(100, 80, 120, 108);
    for (int i = 0; i < 4; i++) {
      int y = 90 + i * 24;
      if (i == sel) { gfx_SetColor(C_PURPLE_D); gfx_FillRectangle(104, y - 4, 112, 18); gfx_SetColor(C_GOLD); gfx_Rectangle(104, y - 4, 112, 18); }
      text_c(M[i], 160, y, i == sel ? C_GOLD : C_WHITE);
    }
    text("CLEAR quit", 6, 230, C_STONE_L);
    present();
  }
  fade_out(); platform_save_write(&save);
  gfx_End(); return 0;
}
