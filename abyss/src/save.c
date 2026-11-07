#include "save.h"
#include "upgrades.h"
uint16_t crc16(const uint8_t *d, int n) {          /* CRC-16/CCITT-FALSE, nibble table (fast enough to verify 40 KB at startup) */
  static const uint16_t T[16] = {0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF};
  uint16_t c = 0xFFFF;
  while (n--) { uint8_t b = *d++; c = (uint16_t)((c << 4) ^ T[(c >> 12) ^ (b >> 4)]); c = (uint16_t)((c << 4) ^ T[(c >> 12) ^ (b & 15)]); }
  return c;
}
void save_default(SaveData *s) { *s = (SaveData){0}; s->abilities = 1; }
static uint8_t *w32(uint8_t *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; return p + 4; }
static uint8_t *w16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; return p + 2; }
static uint32_t r32(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t r16(const uint8_t *p) { return p[0] | p[1] << 8; }
void save_serialize(const SaveData *s, uint8_t out[SAVE_BYTES]) {
  uint8_t *p = out;
  *p++ = 'A'; *p++ = 'B'; *p++ = 'Y'; *p++ = 'S'; *p++ = SAVE_VERSION;
  *p++ = s->unlocked; p = w32(p, s->completed); p = w32(p, s->coins);
  for (int i = 0; i < 3; i++) *p++ = s->upg[i];
  *p++ = s->abilities; p = w16(p, s->relics); p = w16(p, s->secrets); *p++ = s->guardians; p = w16(p, s->deaths);
  for (int i = 0; i < LEVEL_COUNT; i++) p = w32(p, s->treasure[i]);
  w16(p, crc16(out, SAVE_BYTES - 2));
}
SaveStatus save_parse(SaveData *s, const uint8_t *b, int len) {
  if (len != SAVE_BYTES) return SAVE_BAD_LEN;
  if (b[0] != 'A' || b[1] != 'B' || b[2] != 'Y' || b[3] != 'S') return SAVE_BAD_MAGIC;
  if (b[4] != SAVE_VERSION) return SAVE_BAD_VERSION;
  if (r16(b + SAVE_BYTES - 2) != crc16(b, SAVE_BYTES - 2)) return SAVE_BAD_CRC;
  SaveData t; const uint8_t *p = b + 5;
  t.unlocked = *p++; t.completed = r32(p); p += 4; t.coins = r32(p); p += 4;
  for (int i = 0; i < 3; i++) t.upg[i] = *p++;
  t.abilities = *p++; t.relics = r16(p); p += 2; t.secrets = r16(p); p += 2; t.guardians = *p++; t.deaths = r16(p); p += 2;
  for (int i = 0; i < LEVEL_COUNT; i++) { t.treasure[i] = r32(p); p += 4; }
  if (t.unlocked >= LEVEL_COUNT || t.completed >= (1UL << LEVEL_COUNT) || t.coins > COINS_CAP ||
      t.abilities >= 128 || t.guardians >= 16 || t.upg[0] > UPG_MAX_LEVEL || t.upg[1] > UPG_MAX_LEVEL || t.upg[2] > UPG_MAX_LEVEL)
    return SAVE_BAD_RANGE;
  *s = t; return SAVE_OK;
}
SaveStatus save_load_or_default(SaveData *s, const uint8_t *b, int len) {
  SaveStatus st = b ? save_parse(s, b, len) : SAVE_BAD_LEN;
  if (st != SAVE_OK) save_default(s);
  return st;
}
