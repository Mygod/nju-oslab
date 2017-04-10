#include "assert.h"
#include "memlayout.h"
#include "string.h"
#include "x86.h"
#include "kernel/pcb.h"
#include "kernel/pmap.h"

struct PCB pcb_pool[PROCESS_POOL_SIZE];

void pcb_init(struct PCB *pcb, uintptr_t esp, uintptr_t eip, uint32_t eflags) {
  pcb->used = true;
  memset(&pcb->tf, 0, sizeof(pcb->tf));
  pcb->tf.tf_ds = pcb->tf.tf_es = pcb->tf.tf_ss = GD_UD | 3;
  pcb->tf.tf_esp = esp;
  pcb->tf.tf_cs = GD_UT | 3;
  pcb->tf.tf_eip = eip;
  pcb->tf.tf_eflags = eflags;
}

void pcb_exec(int pid, struct PCB *pcb) {
  lcr3((uint32_t) user_pgdir[pid] - KERNBASE);
  __asm __volatile("movl %0,%%esp\n"
      "\tpopal\n"
      "\tpopl %%es\n"
      "\tpopl %%ds\n"
      "\taddl $0x8,%%esp\n" /* skip tf_trapno and tf_errcode */
      "\tiret"
  : : "g" (&pcb->tf) : "memory");
  panic("iret failed");  /* mostly to placate the compiler */
}
