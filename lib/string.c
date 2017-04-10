#include "string.h"

void memcpy(void *dest, const void *src, size_t size) {
  while (size--) *(uint8_t *) dest++ = *(uint8_t *) src++;
}

void memset(void *dest, int c, size_t s) {
  while (s--) *(uint8_t *) dest++ = (uint8_t) c;
}
