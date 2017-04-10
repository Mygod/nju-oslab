#ifndef OSLAB_PMAP_H
#define OSLAB_PMAP_H

#include "pcb.h"
#include "x86.h"

extern pde_t (*user_pgdir[PROCESS_POOL_SIZE])[NPDENTRIES];
void pmap_init();
void pmap_copy(int dest, int src);
static inline void pmap_load(int pid) {
  return lcr3((uint32_t) user_pgdir[pid] - KERNBASE);
}

#endif //OSLAB_PMAP_H
