#ifndef SAVE_H
#define SAVE_H
#include "abyss.h"
#define SAVE_VERSION 1
#define SAVE_BYTES 107
#define COINS_CAP 999999UL
typedef struct {
  uint8_t unlocked, abilities, upg[3], guardians;
  uint16_t relics, secrets, deaths;
  uint32_t completed, coins;
  uint32_t treasure[LEVEL_COUNT];
} SaveData;
typedef enum { SAVE_OK = 0, SAVE_BAD_LEN, SAVE_BAD_MAGIC, SAVE_BAD_VERSION, SAVE_BAD_CRC, SAVE_BAD_RANGE } SaveStatus;
void save_default(SaveData *s);
void save_serialize(const SaveData *s, uint8_t out[SAVE_BYTES]);
SaveStatus save_parse(SaveData *s, const uint8_t *buf, int len);
SaveStatus save_load_or_default(SaveData *s, const uint8_t *buf, int len); /* never leaves s invalid */
uint16_t crc16(const uint8_t *d, int n);
#endif
