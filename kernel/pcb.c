#include "assert.h"
#include "kernel/pcb.h"

void pcb_exec(struct PCB *pcb) {
  // TODO: lcr3
  __asm __volatile("movl %0,%%esp\n"
      "\tpopal\n"
      "\tpopl %%es\n"
      "\tpopl %%ds\n"
      "\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
      "\tiret"
  : : "g" (pcb->tf) : "memory");
  panic("iret failed");  /* mostly to placate the compiler */
}
