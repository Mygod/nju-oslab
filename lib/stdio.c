#include <stdarg.h>

#include "syscall.h"
#include "stdio.h"

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 *
 * Source: http://www.strudel.org.uk/itoa/
 */
char* itoa(int value, char *result) {
  char* ptr = result, *ptr1 = result, tmp_char;
  int tmp_value;

  do {
    tmp_value = value;
    value /= 10;
    *ptr++ = "9876543210123456789" [9 + (tmp_value - value * 10)];
  } while ( value );

  // Apply negative sign
  if (tmp_value < 0) *ptr++ = '-';
  *ptr-- = '\0';
  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}
char* utoa(unsigned value, char* result, int base) {
  // check that the base if valid
  if (base < 2 || base > 36) { *result = '\0'; return result; }

  char* ptr = result, *ptr1 = result, tmp_char;
  unsigned tmp_value;

  do {
    tmp_value = value;
    value /= base;
    *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
  } while ( value );

  *ptr-- = '\0';
  while(ptr1 < ptr) {
    tmp_char = *ptr;
    *ptr--= *ptr1;
    *ptr1++ = tmp_char;
  }
  return result;
}

static inline void _checkedPut(char *out, size_t *counter, size_t size, char c) {
  if (*counter < size) out[*counter] = c;
  ++*counter;
}
static size_t vsnprintf(char *out, size_t size, const char *format, va_list args) {
  char buffer[16];
  size_t counter = 0;
  for (const char *i = format; *i; ++i) if (*i != '%') _checkedPut(out, &counter, size, *i); else switch (*++i) {
    case 'c':
      _checkedPut(out, &counter, size, va_arg(args, int));
      break;
    case 'd':
      itoa(va_arg(args, int), buffer);
      for (char *j = buffer; *j; ++j) _checkedPut(out, &counter, size, *j);
      break;
    case 's':
      for (char *j = va_arg(args, char *); *j; ++j) _checkedPut(out, &counter, size, *j);
      break;
    case 'x':
      utoa(va_arg(args, unsigned), buffer, 16);
      for (char *j = buffer; *j; ++j) _checkedPut(out, &counter, size, *j);
      break;
    default:
      _checkedPut(out, &counter, size, (uint8_t) *i);
      break;
  }
  if (counter < size) out[counter] = '\0';
  return counter;
}

size_t snprintf(char *out, size_t size, const char *format, ...) {
  va_list args;
  va_start(args, format);
  size_t result = vsnprintf(out, size, format, args);
  va_end(args);
  return result;
}

void vprintk(const char *format, va_list args) {
  va_list copy;
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuninitialized"
  va_copy(copy, args);                                // make a copy before reading on
  size_t size = vsnprintf(NULL, 0, format, copy) + 1; // use the copy to calculate size
  va_end(copy);
#pragma clang diagnostic pop
  char out[size];
  vsnprintf(out, size, format, args);
  sys_printk(out, size);
}

void printk(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vprintk(format, args);
  va_end(args);
}

void _warn(const char* file, int line, const char* format, ...) {
  printk("Warning (%s:%d): ", file, line);
  va_list args;
  va_start(args, format);
  vprintk(format, args);
  va_end(args);
  printk("\n");
}
void _panic(const char* file, int line, const char* format, ...) {
  __asm __volatile("cld");
  printk("Fatal (%s:%d): ", file, line);
  va_list args;
  va_start(args, format);
  vprintk(format, args);
  va_end(args);
  printk("\n");
  sys_crash();
}
