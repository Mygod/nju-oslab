#include "assert.h"
#include "memlayout.h"
#include "pmap.h"
#include "x86.h"

#define PMAP_OFFSET 0x1000000 // need to avoid possibly hardware mapped region: 0xF00000 - 0xFFFFFF
#define MAX_MEM     0x8000000 // QEMU default: 128MB
#define PMAP_SIZE   (MAX_MEM - PMAP_OFFSET)

extern pde_t entry_pgdir[NPDENTRIES];
static uint32_t allocated;
__attribute__((__aligned__(PGSIZE)))
static pte_t user_pgtable[PMAP_SIZE >> PDXSHIFT][NPTENTRIES];

void pmap_alloc(uint32_t addr, bool user) {
  assert(allocated << PTSHIFT < PMAP_SIZE);
  addr >>= PTSHIFT;
  uint32_t flags = PTE_P | PTE_W;
  if (user) flags |= PTE_U;
  for (int i = 0; i < NPTENTRIES; ++i)
    user_pgtable[allocated][i] = (pte_t) ((PMAP_OFFSET + (allocated << PTSHIFT)) | (i << 12) | flags);
  entry_pgdir[addr] = (pde_t) (((uintptr_t) &user_pgtable[allocated] - KERNBASE) | flags);
  printk("pmap: Allocated 4MB at 0x%x to 0x%x, used %d/%d\n", addr << PTSHIFT, PMAP_OFFSET + (allocated << PTSHIFT),
         allocated + 1, PMAP_SIZE >> PTSHIFT);
  ++allocated;
}
