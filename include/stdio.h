#ifndef STDIO_H
#define STDIO_H

char* itoa(int value, char *result);
char* utoa(unsigned value, char* result, int base);
void printk(const char *format, ...);

#endif
