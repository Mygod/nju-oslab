#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include "kernel/fs.h"

#define TRY(x) do { if ((r = (x)) < 0) return r; } while (false)

void _panic(const char* file, int line, const char* format, ...) {
  __asm __volatile("cld");
  printf("Fatal (%s:%d): ", file, line);
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  printf("\n");
  assert(0);
}

int imageFd;
int ide_read(uint32_t secno, void *dst, size_t nsecs) {
  int r;
  TRY(lseek(imageFd, secno * SECTSIZE, SEEK_SET));
  return (int) read(imageFd, dst, nsecs * SECTSIZE);
}
int ide_write(uint32_t secno, const void *src, size_t nsecs) {
  int r;
  TRY(lseek(imageFd, secno * SECTSIZE, SEEK_SET));
  return (int) write(imageFd, src, nsecs * SECTSIZE);
}
