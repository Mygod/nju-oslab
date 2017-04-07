#include "assert.h"
#include "memlayout.h"
#include "pmap.h"

#define PMAP_OFFSET   0x1000000 // need to avoid possibly hardware mapped region: 0xF00000 - 0xFFFFFF
#define MAX_MEM       0x8000000 // QEMU default: 128MB
#define PMAP_SIZE     (MAX_MEM - PMAP_OFFSET)
#define PGTABLE_SIZE  (PMAP_SIZE >> PDXSHIFT)

/*
 * IMPORTANT: Answer to question in lab2 handouts
 *
 * Q：这样做为什么可以，会不会带来什么问题？
 *
 * Because it's perfectly doable. The problem is that every process will have access to kernel space. This could be
 * utilized by malicious programs.
 */

extern pde_t entry_pgdir[NPDENTRIES];
static uint32_t allocated;
__attribute__((__aligned__(PGSIZE)))
static pte_t user_pgtable[PGTABLE_SIZE][NPTENTRIES];

void pmap_alloc(uint32_t addr, bool user) {
  assert(allocated < PGTABLE_SIZE);
  addr >>= PTSHIFT;
  uint32_t flags = PTE_P | PTE_W;
  if (user) flags |= PTE_U;
  for (int i = 0; i < NPTENTRIES; ++i)
    user_pgtable[allocated][i] = (pte_t) ((PMAP_OFFSET + (allocated << PTSHIFT)) | (i << 12) | flags);
  entry_pgdir[addr] = (pde_t) (((uintptr_t) &user_pgtable[allocated] - KERNBASE) | flags);
  printk("pmap: Allocated 4MB at 0x%x to 0x%x, used %d/%d\n", addr << PTSHIFT, PMAP_OFFSET + (allocated << PTSHIFT),
         allocated + 1, PGTABLE_SIZE);
  ++allocated;
}
