#ifndef ABILITIES_H
#define ABILITIES_H
#include "save.h"
#define ABILITY_COUNT 7
extern const char *const ABILITY_NAMES[ABILITY_COUNT];
extern const uint8_t ABILITY_LEVEL[ABILITY_COUNT];   /* 1-based level number */
uint8_t abilities_for_level(int level_index);
#endif
