#ifndef OSLAB_STRING_H
#define OSLAB_STRING_H

#include "types.h"

void memcpy(void *dest, const void *src, size_t size);
void memset(void *dest, int c, size_t s);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dest, const char *src, size_t n);

#endif //OSLAB_STRING_H
