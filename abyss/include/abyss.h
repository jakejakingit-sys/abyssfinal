#ifndef ABYSS_H
#define ABYSS_H
#include <stdint.h>
#include <stdbool.h>
#define TILE_PX 16
#define SUBPX 16                    /* positions are in 1/16 pixel */
#define TILE_SUB (TILE_PX * SUBPX)
#define LEVEL_COUNT 20
#define MAX_LVL_W 96
#define MAX_LVL_H 15
#define MAX_TREASURE 32
#define COIN_VALUE 5
#define CHEST_VALUE 40
#define PLAT_W 32
#define MAX_PLATS 4
enum Tile { T_AIR = 0, T_SOLID, T_ONEWAY, T_SPIKE, T_LADDER, T_COIN, T_CHEST, T_EXIT, T_CRUMBLE, T_RUNE, T_GATE, T_TORCH,
            T_RUNE_ON /* runtime only */, T_SKULL, T_TILE_COUNT };
enum { ENT_CRAWLER = 0, ENT_PLATFORM = 1 };
#endif
