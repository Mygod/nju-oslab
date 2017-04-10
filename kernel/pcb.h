#ifndef OSLAB_PCB_H
#define OSLAB_PCB_H

#include "string.h"
#include "syscall.h"
#include "trap.h"

#define KSTACK_SIZE 4096
#define PROCESS_POOL_SIZE 28

struct PCB {
  bool used;
  uint64_t wakeTime;
  ClockListener clockListener;
  KeyboardListener keyboardListener;
  uint8_t kstack[KSTACK_SIZE];
  struct Trapframe tf;
};
_Static_assert(offsetof(struct PCB, kstack) + KSTACK_SIZE == offsetof(struct PCB, tf),
               "tf must follows kstack in struct PCB!");
_Static_assert(offsetof(struct PCB, tf) + sizeof(struct Trapframe) == sizeof(struct PCB),
               "tf must be the last member of struct PCB!");

extern struct PCB pcb_pool[PROCESS_POOL_SIZE];
extern int current_pid;
extern uint64_t sys_time;

void pcb_init(struct PCB *pcb, uintptr_t esp, uintptr_t eip, uint32_t eflags);
static inline void pcb_copy(struct PCB *dest, const struct PCB *src) {
  memcpy(dest, src, sizeof(struct PCB));
}
void pcb_exec(int pid, struct PCB *pcb);
static inline void pcb_free(struct PCB *pcb) {
  pcb->used = false;
}

#endif //OSLAB_PCB_H
