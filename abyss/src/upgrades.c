#include "upgrades.h"
static const uint16_t COST[UPG_COUNT][UPG_MAX_LEVEL] = {
  {60, 120, 240, 480, 900}, {50, 100, 200, 400, 800}, {50, 100, 200, 400, 800}};
int upgrade_cost(UpgCat c, int level) { return (c >= UPG_COUNT || level < 0 || level >= UPG_MAX_LEVEL) ? -1 : COST[c][level]; }
BuyResult upgrade_buy(SaveData *s, UpgCat c) {
  int cost = upgrade_cost(c, s->upg[c]);
  if (cost < 0) return BUY_MAXED;
  if (s->coins < (uint32_t)cost) return BUY_NO_COINS;
  s->coins -= cost; s->upg[c]++;
  return BUY_OK;
}
int upgrade_max_hp(const SaveData *s) { return 100 + 20 * s->upg[UPG_HEALTH]; }
int upgrade_max_energy(const SaveData *s) { return 100 + 20 * s->upg[UPG_ENERGY]; }
int upgrade_armor(const SaveData *s) { return 2 * s->upg[UPG_ARMOR]; }
