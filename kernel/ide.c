#include "assert.h"
#include "ide.h"
#include "x86.h"

#define IDE_BSY		0x80
#define IDE_DRDY	0x40
#define IDE_DF		0x20
#define IDE_ERR		0x01

static int
ide_wait_ready(bool check_error)
{
  int r;

  while (((r = inb(0x1F7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
    /* do nothing */;

  if (check_error && (r & (IDE_DF|IDE_ERR)) != 0)
    return -1;
  return 0;
}

_Static_assert(!(SECTSIZE & (SECTSIZE - 1)), "SECTSIZE must be a power of 2!");
int ide_read(void *dst, uint32_t offset, uint32_t count) {
  assert(!(offset & 3));  // assert: aligned at 4 byte
  assert(!(count & 3));
  for (uint32_t i = offset & -SECTSIZE, end = offset + count; i < end;) {
    int r;
    ide_wait_ready(false);
    uint32_t secno = i / SECTSIZE;
    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t) secno);
    outb(0x1F4, (uint8_t) (secno >> 8));
    outb(0x1F5, (uint8_t) (secno >> 16));
    outb(0x1F6, (uint8_t) (0xE0 | (secno >> 24)));
    outb(0x1F7, 0x20);	// CMD 0x20 means read sector
    if ((r = ide_wait_ready(1)) < 0) return r;
    int32_t n = offset - i;
    if (n > 0) {
      for (uint32_t j = 0; j < n; j += 4) inl(0x1F0); // seek
      i += n;
      n = SECTSIZE - n;
    } else n = SECTSIZE;
    if (n > end - i) n = end - i;
    insl(0x1F0, dst, n / 4);
    i += n;
    dst += n;
  }
  return 0;
}
