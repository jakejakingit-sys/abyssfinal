#include "assets.h"
#include "level.h"
#include "platform.h"
typedef struct { const uint8_t *base; uint32_t len; } Pack;
static Pack packs[PACK_COUNT];
static AssetStatus status = ASSETS_NO_CORE;
static uint16_t rd16(const uint8_t *p) { return p[0] | p[1] << 8; }
static uint32_t rd32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
/* pack header: "ABYP" kind(1) version(1) count(2) crc16(2) total_len(4); then count x u32 offsets */
static bool header_ok(const uint8_t *b, uint32_t len, int kind, bool full) {
  if (len < 14 || b[0] != 'A' || b[1] != 'B' || b[2] != 'Y' || b[3] != 'P' || b[4] != kind || b[5] != 2) return false;
  if (rd32(b + 10) != len || 14 + 4 * (uint32_t)rd16(b + 6) > len) return false;
  return !full || crc16(b + 14, (int)(len - 14)) == rd16(b + 8);
}
static const uint8_t *item(const Pack *p, int i) {
  if (!p->base || i < 0 || i >= rd16(p->base + 6)) return 0;
  uint32_t off = rd32(p->base + 14 + 4 * i);
  return off < p->len ? p->base + off : 0;
}
const uint8_t *spr(int id) { return id >= 0 && id < SPR_COUNT ? item(&packs[0], id) : 0; }
const uint8_t *theme_colors(int world) { const uint8_t *t = item(&packs[0], ITEM_THEMES); return t ? t + (world & 3) * 33 : 0; }
const uint8_t *base_palette(void) { return item(&packs[0], ITEM_PALETTE); }
static bool parse_level(LevelDef *d, const uint8_t *b, uint32_t avail) {
  if (avail < 8) return false;
  uint8_t nl = b[7]; uint32_t need = 8u + nl + 3u * b[6] + (uint32_t)b[1] * b[2];
  if (b[1] < 8 || b[1] > MAX_LVL_W || b[2] != MAX_LVL_H || need > avail || nl == 0 || b[8 + nl - 1] != 0) return false;
  d->world = b[0]; d->w = b[1]; d->h = b[2]; d->spawn_tx = b[3]; d->spawn_ty = b[4]; d->boss = b[5]; d->nent = b[6];
  d->name = (const char *)(b + 8); d->ents = (const EntDef *)(b + 8 + nl); d->tiles = b + 8 + nl + 3 * b[6];
  return true;
}
static void acquire(bool full) {
  bool core_seen = false;
  for (int i = 0; i < LEVEL_COUNT; i++) LEVELS[i].tiles = 0;
  for (int id = 0; id < PACK_COUNT; id++) {
    uint32_t len = 0; const uint8_t *p = platform_pack_ptr(id, &len);
    packs[id].base = 0; packs[id].len = 0;
    if (id == 0 && p) core_seen = true;
    if (!p || !header_ok(p, len, id == 0 ? 0 : 1, full)) continue;
    packs[id].base = p; packs[id].len = len;
    if (id == 0) continue;
    int n = rd16(p + 6);
    for (int k = 0; k < n && k < 5; k++) {
      uint32_t off = rd32(p + 14 + 4 * k);
      if (off < len) parse_level(&LEVELS[(id - 1) * 5 + k], p + off, len - off);
    }
  }
  status = packs[0].base ? ASSETS_OK : core_seen ? ASSETS_BAD_CORE : ASSETS_NO_CORE;
}
bool assets_init(void) { acquire(true); return status == ASSETS_OK; }
void assets_refresh(void) { acquire(false); }
AssetStatus assets_status(void) { return status; }
