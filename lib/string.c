#include "string.h"

void memcpy(void *dest, const void *src, size_t size) {
  while (size--) *(uint8_t *) dest++ = *(uint8_t *) src++;
}

void memset(void *dest, int c, size_t s) {
  while (s--) *(uint8_t *) dest++ = (uint8_t) c;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (int x; n; --n) if ((x = *s1 - *s2) || !(*s1 && *s2)) return x;
  return 0;
}

char *strncpy(char *dest, const char *src, size_t n) {
  while (n--) if ((*dest++ = *src)) ++src;
  return dest;
}
