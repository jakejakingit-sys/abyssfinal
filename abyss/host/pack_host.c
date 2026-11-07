/* HOST-ONLY: serves the generated packs the way the calculator serves archived AppVars. Not compiled into the calculator program. */
#include "platform.h"
extern const uint8_t PACK_CORE[];
extern const unsigned PACK_CORE_LEN;
const uint8_t *host_level_pack(int w, uint32_t *len);
const uint8_t *platform_pack_ptr(int id, uint32_t *len) {
  if (id == 0) { *len = PACK_CORE_LEN; return PACK_CORE; }
  return host_level_pack(id, len);
}
