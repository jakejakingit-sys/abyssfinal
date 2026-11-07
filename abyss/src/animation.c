#include "animation.h"
int anim_player_frame(const Player *p, uint16_t tick) {
  if (p->ladder) return FR_CLIMB0 + ((p->y / (6 * SUBPX)) & 1);
  if (!p->ground) return p->vy < 0 ? FR_JUMP : FR_FALL;
  int v = p->vx < 0 ? -p->vx : p->vx;
  if (v > 4) return FR_RUN0 + ((tick / 4) & 3);
  return FR_IDLE0 + ((tick / 28) & 1);
}
