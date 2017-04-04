#ifndef OSLAB_PCB_H
#define OSLAB_PCB_H

#include "trap.h"

#define KSTACK_SIZE 4096
struct PCB {
  struct Trapframe *tf;
  uint8_t kstack[KSTACK_SIZE];
};

void pcb_exec(struct PCB *pcb);

#endif //OSLAB_PCB_H
