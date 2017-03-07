#include "irq.h"
#include "memlayout.h"
#include "mmu.h"
#include "stdio.h"
#include "trap.h"
#include "types.h"
#include "x86.h"

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256];
struct Pseudodesc idt_pd = { sizeof(idt) - 1, (uint32_t) idt };
extern uint32_t trap_handlers[];

void trap_init() {
  for (int i = 0; i < 256 ; i++) SETGATE(idt[i], 0, GD_KT, trap_handlers[i], 0);
  // init break point
  SETGATE(idt[T_BRKPT], 0, GD_KT, trap_handlers[T_BRKPT], 3);
  // init syscall
  SETGATE(idt[T_SYSCALL], 0, GD_KT, trap_handlers[T_SYSCALL], 3);

  lidt(&idt_pd);
}

void trap(struct Trapframe *tf) {
  // The environment may have set DF and some versions
  // of GCC rely on DF being clear
  __asm __volatile("cld" ::: "cc");

  static int base = 0;
  switch (tf->tf_trapno) {
    case IRQ_OFFSET + IRQ_TIMER:
      for (int i = 0; i < 320 * 200; ++i) *((uint8_t *) 0xA0000 + i) = (uint8_t) (base + i);
      ++base;
      irq_eoi();
      break;
    case IRQ_OFFSET + IRQ_KBD:
      printk("Kernel: Keyboard 0x%x pressed!\n", inb(0x60));
      irq_eoi();
      break;
  }
}
