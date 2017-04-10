#include "assert.h"
#include "irq.h"
#include "memlayout.h"
#include "pcb.h"
#include "pmap.h"
#include "syscall.h"

struct Segdesc gdt[6] = {
    // 0x0 - unused (always faults -- for trapping NULL far pointers)
    SEG_NULL,

    // 0x8 - kernel code segment
    [GD_KT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 0),

    // 0x10 - kernel data segment
    [GD_KD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 0),

    // 0x18 - user code segment
    [GD_UT >> 3] = SEG(STA_X | STA_R, 0x0, 0xffffffff, 3),

    // 0x20 - user data segment
    [GD_UD >> 3] = SEG(STA_W, 0x0, 0xffffffff, 3),

    // TSS descriptor
    [GD_TSS0 >> 3] = SEG_NULL
};
struct Pseudodesc gdt_pd = { sizeof(gdt) - 1, (uint32_t) gdt };

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256];
struct Pseudodesc idt_pd = { sizeof(idt) - 1, (uint32_t) idt };
extern uint32_t trap_handlers[];

struct Taskstate pts;

void env_init() {
  lgdt(&gdt_pd);
  // The kernel never uses GS or FS, so we leave those set to
  // the user data segment.
  asm volatile("movw %%ax,%%gs" :: "a" (GD_UD|3));
  asm volatile("movw %%ax,%%fs" :: "a" (GD_UD|3));
  // The kernel does use ES, DS, and SS.  We'll change between
  // the kernel and user data segments as needed.
  asm volatile("movw %%ax,%%es" :: "a" (GD_KD));
  asm volatile("movw %%ax,%%ds" :: "a" (GD_KD));
  asm volatile("movw %%ax,%%ss" :: "a" (GD_KD));
  // Load the kernel text segment into CS.
  asm volatile("ljmp %0,$1f\n 1:\n" :: "i" (GD_KT));
  // For good measure, clear the local descriptor table (LDT),
  // since we don't use it.
  lldt(0);

  for (int i = 0; i < 256 ; i++) SETGATE(idt[i], 0, GD_KT, trap_handlers[i], 0);
  // the following traps are available for user processes
  // init break point
  SETGATE(idt[T_BRKPT], 0, GD_KT, trap_handlers[T_BRKPT], 3);
  // init syscall
  SETGATE(idt[T_SYSCALL], 0, GD_KT, trap_handlers[T_SYSCALL], 3);

  pts.ts_esp0 = 0xF0400000;
  pts.ts_ss0 = GD_KD;

  // Initialize the TSS slot of the gdt.
  gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&pts), sizeof(struct Taskstate), 0);
  gdt[GD_TSS0 >> 3].sd_s = 0;

  // Load the TSS selector (like other segment selectors, the
  // bottom three bits are special; we leave them 0)
  ltr(GD_TSS0);

  lidt(&idt_pd);
}

extern uint32_t syscall_dispatch(struct Trapframe *tf);
void trap(struct Trapframe *tf) {
  // The environment may have set DF and some versions
  // of GCC rely on DF being clear
  __asm __volatile("cld" ::: "cc");

  switch (tf->tf_trapno) {
    case T_GPFLT: panic("General protection fault at 0x%x.", tf->tf_eip);
    case T_PGFLT: panic("Page fault at 0x%x, va=0x%x", tf->tf_eip, rcr2());
    case T_SYSCALL:
      tf->tf_regs.reg_eax = syscall_dispatch(tf);
      break;
    case IRQ_OFFSET + IRQ_TIMER: {
      static int64_t timer;
      ++timer;
      extern ClockListener clockListener;
      if (!(timer % PROCESS_POOL_SIZE) && clockListener != NULL) clockListener(); // ~47.666 Hz
      irq_eoi();
      break;
    }
    case IRQ_OFFSET + IRQ_KBD: {
      uint8_t code = inb(0x60);
      extern KeyboardListener keyboardListener;
      if (keyboardListener != NULL) keyboardListener(code);
      irq_eoi();
      break;
    }
    case IRQ_OFFSET + IRQ_IDE:
      // ignore, triggered when iret to user
      break;
    default:
      warn("Unhandled interrupt %d occurred at 0x%x.", tf->tf_trapno, tf->tf_eip);
      break;
  }
}
