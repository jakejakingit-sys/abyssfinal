/* Calculator-only glue (CEdev fileioc). Compiled only for the TI-84 CE; host tests use host/pack_host.c instead. */
#include "platform.h"
#include "assets.h"
#ifdef __TICE__
#include <fileioc.h>
#include <stdbool.h>
#define SAVE_NAME "ABYSSSV"
static const char *const PACK_NAMES[PACK_COUNT] = {"ABYSSCOR", "ABYSSLV1", "ABYSSLV2", "ABYSSLV3", "ABYSSLV4"};

/* Asset AppVars: make sure they sit in ARCHIVE (flash), then hand out a read-only pointer to their contents.
   Nothing is copied to RAM here; sprites are drawn straight from flash, only the current level is copied (level.c). */
const uint8_t *platform_pack_ptr(int id, uint32_t *len) {
  uint8_t h = ti_Open(PACK_NAMES[id], "r");
  if (!h) return NULL;
  if (!ti_IsArchived(h)) ti_SetArchiveStatus(true, h);      /* one-time flash write if the user sent it to RAM */
  ti_Close(h);
  h = ti_Open(PACK_NAMES[id], "r");                          /* re-open: the data may have moved when archived */
  if (!h) return NULL;
  const uint8_t *p = (const uint8_t *)ti_GetDataPtr(h);
  *len = ti_GetSize(h);
  ti_Close(h);
  return p;
}
void platform_save_write(const SaveData *s) {
  uint8_t buf[SAVE_BYTES]; save_serialize(s, buf);
  uint8_t h = ti_Open(SAVE_NAME, "w");                       /* fresh RAM variable */
  if (h) { ti_Write(buf, SAVE_BYTES, 1, h); ti_SetArchiveStatus(true, h); ti_Close(h); }   /* one archive write */
  assets_refresh();                                          /* archive writes can move data: re-acquire every pointer */
}
SaveStatus platform_save_load(SaveData *s) {
  uint8_t buf[SAVE_BYTES]; uint8_t h = ti_Open(SAVE_NAME, "r");
  if (!h) { save_default(s); return SAVE_BAD_LEN; }
  size_t n = ti_Read(buf, 1, SAVE_BYTES, h);
  ti_Close(h);
  return save_load_or_default(s, buf, (int)n);
}
#endif
