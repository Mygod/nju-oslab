#include "memlayout.h"
#include "pmap.h"

#define PMAP_OFFSET   0x1000000 // need to avoid possibly hardware mapped region: 0xF00000 - 0xFFFFFF
#define MAX_MEM       0x8000000 // QEMU default: 128MB
#define PMAP_SIZE     (MAX_MEM - PMAP_OFFSET)
#define PGTABLE_SIZE  (PMAP_SIZE >> PDXSHIFT)
_Static_assert(PROCESS_POOL_SIZE == PGTABLE_SIZE,
               "We need to have a process pool with same size as that of page table just for convenience.");

/*
 * IMPORTANT: Answer to question in lab2 handouts
 *
 * Q：这样做为什么可以，会不会带来什么问题？
 *
 * Because it's perfectly doable. The problem is that every process will have access to kernel space. This could be
 * utilized by malicious programs.
 */

extern pde_t entry_pgdir[NPDENTRIES];
pde_t (*user_pgdir[PROCESS_POOL_SIZE])[NPDENTRIES];
__attribute__((__aligned__(PGSIZE)))
static pde_t user_pgdir_impl[PROCESS_POOL_SIZE - 1][NPDENTRIES];
__attribute__((__aligned__(PGSIZE)))
static pte_t user_pgtable[PGTABLE_SIZE][NPTENTRIES];

void pmap_init() {
  user_pgdir[0] = &entry_pgdir;
  for (int i = 1; i < PROCESS_POOL_SIZE; ++i) user_pgdir[i] = &user_pgdir_impl[i - 1];
  for (int i = 0; i < PGTABLE_SIZE; ++i) {
    for (int j = 0; j < NPTENTRIES; ++j)
      user_pgtable[i][j] = (pte_t) ((PMAP_OFFSET + (i << PTSHIFT)) | (j << 12) | PTE_P | PTE_W | PTE_U);
    (*user_pgdir[i])[0] = entry_pgdir[0];
    (*user_pgdir[i])[0x20] = (pde_t) (((uintptr_t) &user_pgtable[i] - KERNBASE) | PTE_P | PTE_W | PTE_U);
    (*user_pgdir[i])[KERNBASE >> PDXSHIFT] = entry_pgdir[KERNBASE >> PDXSHIFT];
  }
}

void pmap_copy(int dest, int src) {
  (*user_pgdir[src])[0x40] = (pde_t) (((uintptr_t) &user_pgtable[dest] - KERNBASE) | PTE_P | PTE_W | PTE_U);
  memcpy((void *) 0x10000000, (const void *) 0x8000000, PTSIZE);
  (*user_pgdir[src])[0x40] = (pde_t) 0; // clean up
}
