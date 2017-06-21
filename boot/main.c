#include "elf.h"
#include "x86.h"
#include "kernel/fs.h"

#define SECTSIZE 512

static inline void memset(void *dest, int c, size_t s) {
  while (s--) *(uint8_t *) dest++ = (uint8_t) c;
}

void
waitdisk(void)
{
  // wait for disk ready
  while ((inb(0x1F7) & 0xC0) != 0x40);
}

void
readsect(void *dst, uint32_t offset)
{
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
}

// dead loops are introduced for diagnosis
void bootloader() {
  INode inode;
  uint8_t header[SECTSIZE];
  int index_inode = 0;
  readsect(&inode, FSOFFSET_DATA);  // assert: dir[0].inodeOffset = 0
  readsect(header, FSOFFSET_DATA + inode.dataBlocks[0]);
  struct ELFHeader *elfheader = (struct ELFHeader *) header;
  // if (elfheader->magic != 0x464C457FU) for (;;);  // "\x7FELF" in little endian
  struct ProgramHeader *ph = (struct ProgramHeader *) (header + elfheader->phoff);
  for (int phnum = elfheader->phnum; phnum > 0; --phnum, ++ph) if (ph->memsz) {
    // if (ph->off & (SECTSIZE - 1)) for (;;);  // aligned at sector
    int i = ph->off / SECTSIZE / INODE_DATA_COUNT, count = 0;
    // if (index_inode > i) for (;;);  // strictly increasing
    while (index_inode < i) {
      readsect(&inode, FSOFFSET_DATA + inode.next);
      ++index_inode;
    }
    i = ph->off / SECTSIZE % INODE_DATA_COUNT;
    while (count < ph->filesz) {
      readsect((void *) (ph->paddr + count), FSOFFSET_DATA + inode.dataBlocks[i]);
      if (++i >= INODE_DATA_COUNT) {
        i = 0;
        readsect(&inode, inode.next);
      }
      count += SECTSIZE;
    }
    memset((void *) (ph->paddr + ph->filesz), 0, ph->memsz - ph->filesz);
  }
  ((void (*)()) elfheader->entry)();
  for (;;);
}
