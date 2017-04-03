#include "irq.h"
#include "memlayout.h"
#include "stdio.h"
#include "trap.h"

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256];
struct Pseudodesc idt_pd = { sizeof(idt) - 1, (uint32_t) idt };
extern uint32_t trap_handlers[];

void trap_init() {
  for (int i = 0; i < 256 ; i++) SETGATE(idt[i], 0, GD_KT, trap_handlers[i], 0);
  // the following traps are available for user processes
  // init break point
  SETGATE(idt[T_BRKPT], 0, GD_KT, trap_handlers[T_BRKPT], 3);
  // init syscall
  SETGATE(idt[T_SYSCALL], 0, GD_KT, trap_handlers[T_SYSCALL], 3);

  lidt(&idt_pd);
}

extern uint32_t syscall_dispatch(struct Trapframe *tf);
void trap(struct Trapframe *tf) {
  // The environment may have set DF and some versions
  // of GCC rely on DF being clear
  __asm __volatile("cld" ::: "cc");

  static int base = 1;
  uint8_t code;
  switch (tf->tf_trapno) {
    case T_SYSCALL:
      tf->tf_regs.reg_eax = syscall_dispatch(tf);
      break;
    case IRQ_OFFSET + IRQ_TIMER:
      for (int i = 0; i < 320 * 200; ++i) *((uint8_t *) 0xA0000 + i) = (uint8_t) (base + i + i);
      base += 2;
      irq_eoi();
      break;
    case IRQ_OFFSET + IRQ_KBD:
      code = inb(0x60);
      printk("Kernel: Keyboard 0x%x pressed!\n", code);
      base = (base & ~1) | (code >> 7);
      irq_eoi();
      break;
  }
}
