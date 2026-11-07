#ifndef UPGRADES_H
#define UPGRADES_H
#include "save.h"
typedef enum { UPG_ARMOR = 0, UPG_HEALTH, UPG_ENERGY, UPG_COUNT } UpgCat;
typedef enum { BUY_OK = 0, BUY_MAXED, BUY_NO_COINS } BuyResult;
#define UPG_MAX_LEVEL 5
int upgrade_cost(UpgCat c, int level);            /* -1 when maxed */
BuyResult upgrade_buy(SaveData *s, UpgCat c);
int upgrade_max_hp(const SaveData *s);
int upgrade_max_energy(const SaveData *s);
int upgrade_armor(const SaveData *s);             /* flat damage reduction */
#endif
