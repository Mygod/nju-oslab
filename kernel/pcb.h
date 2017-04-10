#ifndef OSLAB_PCB_H
#define OSLAB_PCB_H

#include "trap.h"

#define PROCESS_POOL_SIZE 28

struct PCB {
  bool used;
  struct Trapframe tf;
};

extern struct PCB pcb_pool[PROCESS_POOL_SIZE];

void pcb_init(struct PCB *pcb, uintptr_t esp, uintptr_t eip, uint32_t eflags);
void pcb_exec(int pid, struct PCB *pcb);
static inline void pcb_free(struct PCB *pcb) {
  pcb->used = false;
}

#endif //OSLAB_PCB_H
