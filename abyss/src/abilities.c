#include "abilities.h"
const char *const ABILITY_NAMES[ABILITY_COUNT] = {"Rune Pulse", "Heavy Landing", "Phase Dash", "Protective Parry", "Ground Wave", "Whirlwind", "Abyssal Resonance"};
const uint8_t ABILITY_LEVEL[ABILITY_COUNT] = {1, 3, 6, 9, 12, 16, 20};
uint8_t abilities_for_level(int idx) {
  uint8_t m = 0;
  for (int i = 0; i < ABILITY_COUNT; i++) if (ABILITY_LEVEL[i] <= idx + 1) m |= 1 << i;
  return m;
}
