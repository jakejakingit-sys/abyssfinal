#ifndef ASSETS_H
#define ASSETS_H
#include <stdint.h>
#include <stdbool.h>
#include "sprite_ids.h"
/* Sprite = [width, height, indexed pixels...] (graphx gfx_sprite_t layout), index 0 transparent. Lives in the archived ABYSSCOR AppVar. */
typedef enum { ASSETS_OK = 0, ASSETS_NO_CORE, ASSETS_BAD_CORE } AssetStatus;
bool assets_init(void);          /* validate every pack (magic, size, CRC-16) and acquire archive pointers; false => core missing/corrupt */
void assets_refresh(void);       /* re-acquire pointers after an archive write (cheap, no CRC) */
AssetStatus assets_status(void);
const uint8_t *spr(int id);                  /* pointer into archive; NULL if the id is invalid */
const uint8_t *theme_colors(int world);      /* 11 RGB triples for palette indices 1..11 */
const uint8_t *base_palette(void);           /* 32 RGB triples */
#endif
