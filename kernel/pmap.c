#include "assert.h"
#include "error.h"
#include "memlayout.h"
#include "pmap.h"

#define PMAP_OFFSET   0x1000000 // need to avoid possibly hardware mapped region: 0xF00000 - 0xFFFFFF
#define MAX_MEM       0x8000000 // QEMU default: 128MB
#define PMAP_SIZE     (MAX_MEM - PMAP_OFFSET)
#define PGTABLE_SIZE  (PMAP_SIZE >> PDXSHIFT)
#define MMAP_COUNT    1
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
__attribute__((__aligned__(PGSIZE)))
static uint8_t mmap_memory[MMAP_COUNT][PGSIZE];

void pmap_init_process(int pid) {
  for (int i = 0; i < NPTENTRIES; ++i)
    user_pgtable[pid][i] = (pte_t) ((PMAP_OFFSET + (pid << PTSHIFT)) | (i << 12) | PTE_P | PTE_W | PTE_U);
}

void pmap_init() {
  user_pgdir[0] = &entry_pgdir;
  for (int i = 1; i < PROCESS_POOL_SIZE; ++i) user_pgdir[i] = &user_pgdir_impl[i - 1];
  for (int i = 0; i < PGTABLE_SIZE; ++i) {
    (*user_pgdir[i])[0] = entry_pgdir[0];
    (*user_pgdir[i])[0x20] = (pde_t) (((uintptr_t) &user_pgtable[i] - KERNBASE) | PTE_P | PTE_W | PTE_U);
    (*user_pgdir[i])[KERNBASE >> PDXSHIFT] = entry_pgdir[KERNBASE >> PDXSHIFT];
  }
}

void pmap_copy(int dest, int src) {
  pmap_init_process(dest);
  (*user_pgdir[src])[0x40] = (pde_t) (((uintptr_t) &user_pgtable[dest] - KERNBASE) | PTE_P | PTE_W | PTE_U);
  memcpy((void *) 0x10000000, (const void *) 0x8000000, PTSIZE);
  (*user_pgdir[src])[0x40] = (pde_t) 0; // clean up
}

int sys_mmap(void *addr, int id) {
  if ((uintptr_t) addr & (PGSIZE - 1)) return E_ALIGN; // must be aligned at PGSIZE
  if ((uintptr_t) addr >> PDXSHIFT != 0x20) return E_OUTOFMEM;
  if (id < 0 || id >= MMAP_COUNT) return E_INVID;
  user_pgtable[current_pid][((uintptr_t) addr >> PTXSHIFT) & (NPTENTRIES - 1)] =
    (pte_t) (((uintptr_t) &mmap_memory[id] - KERNBASE) | PTE_P | PTE_W | PTE_U);
  return E_SUCCESS;
}
