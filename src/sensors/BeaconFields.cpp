#include "BeaconFields.h"

#include <string.h>

void appendBeaconField(char* buf, size_t buf_len, const char* field) {
  size_t used = strlen(buf);
  if (used == 0) {
    strlcpy(buf, field, buf_len);
    return;
  }
  if (used + 1 < buf_len) {
    strlcat(buf, " ", buf_len);
    strlcat(buf, field, buf_len);
  }
}
