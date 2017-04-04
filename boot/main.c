#include "elf.h"
#include "x86.h"

#include "lib/string.c"

#define SECTSIZE 512
#define SECTCOUNT 8

void
waitdisk(void)
{
  // wait for disk ready
  while ((inb(0x1F7) & 0xC0) != 0x40);
}

void
readsect(void *dst, uint32_t offset, uint8_t count)
{
  while (count--) {
    // wait for disk to be ready
    waitdisk();

    outb(0x1F2, 1);
    outb(0x1F3, (uint8_t) offset);    //address = offset | 0xe0000000
    outb(0x1F4, (uint8_t) (offset >> 8));
    outb(0x1F5, (uint8_t) (offset >> 16));
    outb(0x1F6, (uint8_t) ((offset >> 24) | 0xE0));
    outb(0x1F7, 0x20);  // cmd 0x20 - read sectors

    // wait for disk to be ready
    waitdisk();

    // read sectors
    insl(0x1F0, dst, SECTSIZE/4);
    dst += SECTSIZE;
    ++offset;
  }
}

// dead loops are introduced for diagnosis
void bootloader() {
  uint8_t header[SECTSIZE * SECTCOUNT];
  readsect(header, 1, SECTCOUNT);
  struct ELFHeader *elfheader = (struct ELFHeader *) header;
  if (elfheader->magic != 0x464C457FU) for (;;);  // "\x7FELF" in little endian
  struct ProgramHeader *ph = (struct ProgramHeader *) (header + elfheader->phoff);
  for (int phnum = elfheader->phnum; phnum > 0; --phnum, ++ph) {
    if (ph->off & (SECTSIZE - 1)) for (;;);  // aligned at sector
    int sectors = (ph->filesz + (SECTSIZE - 1)) / SECTSIZE;
    if (sectors >= 256) for (;;);            // things can be read in one go
    readsect((void *) ph->paddr, 1 + ph->off / SECTSIZE, (uint8_t) sectors);
    memset((void *) (ph->paddr + ph->filesz), 0, ph->memsz - ph->filesz);
  }
  ((void (*)()) (elfheader->entry & 0xFFFFFFF))();
  for (;;);
}
