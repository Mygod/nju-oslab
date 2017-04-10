#ifndef OSLAB_PMAP_H
#define OSLAB_PMAP_H

#include "pcb.h"

extern pde_t (*user_pgdir[PROCESS_POOL_SIZE])[NPDENTRIES];
void pmap_init();

#endif //OSLAB_PMAP_H
