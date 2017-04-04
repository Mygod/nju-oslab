#include "string.h"

void memset(void *dest, int c, size_t s) {
  while (s--) *(uint8_t *) dest++ = (uint8_t) c;
}
