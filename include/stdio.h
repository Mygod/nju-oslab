#ifndef STDIO_H
#define STDIO_H

#include "types.h"

char* itoa(int value, char *result);
char* utoa(unsigned value, char* result, int base);
size_t snprintf(char *out, size_t size, const char *format, ...);
void printk(const char *format, ...);

#endif
