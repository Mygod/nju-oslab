#include "assert.h"
#include "error.h"
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

  pts.ts_esp0 = KSTACKTOP;
  pts.ts_ss0 = GD_KD;

  // Initialize the TSS slot of the gdt.
  gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&pts), sizeof(struct Taskstate), 0);
  gdt[GD_TSS0 >> 3].sd_s = 0;

  // Load the TSS selector (like other segment selectors, the
  // bottom three bits are special; we leave them 0)
  ltr(GD_TSS0);

  lidt(&idt_pd);
}

static bool halted;
extern int sem_pool[];
__attribute__((noreturn)) void sched() {
  for (int i = 1; i <= PROCESS_POOL_SIZE; ++i) {
    int pid = (current_pid + i) % PROCESS_POOL_SIZE;
    if (pcb_pool[pid].used && pcb_pool[pid].wakeTime <= sys_time) {
      if (pcb_pool[pid].waitSem >= 0) {
        if (sem_pool[pcb_pool[pid].waitSem]) {
          --sem_pool[pcb_pool[pid].waitSem];
          pcb_pool[pid].waitSem = -1;
        } else continue;
      }
      pcb_exec(pid, &pcb_pool[pid]);
    }
  }
  // Nothing else to schedule, wait for something to happen
  // We're now within kernel space so we need to move our pointer
  halted = true;
  __asm __volatile(
    "movl %0, %%ebp\n"
    "movl %0, %%esp\n"
    "sti\n"
    "1:\n"
    "hlt\n"
    "jmp 1b\n"
    : : "i" (KSTACKTOP));
  panic("shut up compiler you don't know what you're doing");
}

extern uint32_t syscall_dispatch(struct Trapframe *tf);
void trap(struct Trapframe *tf) {
  // The environment may have set DF and some versions
  // of GCC rely on DF being clear
  __asm __volatile("cld" ::: "cc");

  bool wasHalted = halted;
  halted = false;
  // printk("[DEBUG] tf: 0x%x, pcb.tf: 0x%x\n", tf, &pcb_pool[current_pid].tf);
  switch (tf->tf_trapno) {
    case T_ILLOP: panic("Illegal opcode at 0x%x, pid=%d: 0x%x", tf->tf_eip, current_pid, *(uint32_t *) tf->tf_eip);
    case T_GPFLT: panic("General protection fault at 0x%x, pid=%d", tf->tf_eip, current_pid);
    case T_PGFLT: panic("Page fault at 0x%x, va=0x%x, pid=%d", tf->tf_eip, rcr2(), current_pid);
    case T_SYSCALL:
      tf->tf_regs.reg_eax = syscall_dispatch(tf);
      break;
    case IRQ_OFFSET + IRQ_TIMER: {
      ++sys_time;
      if (!(sys_time % PROCESS_POOL_SIZE)) {  // ~47.666 Hz
        for (int i = 0; i < PROCESS_POOL_SIZE; ++i) if (pcb_pool[i].used && pcb_pool[i].clockListener) {
          pmap_load(i);
          pcb_pool[i].clockListener();
        }
      }
      irq_eoi();
      sched();  // current process has used up its time
    }
    case IRQ_OFFSET + IRQ_KBD: {
      uint8_t code = inb(0x60);
      int now = current_pid;
      for (int i = 0; i < PROCESS_POOL_SIZE; ++i) if (pcb_pool[i].used && pcb_pool[i].keyboardListener) {
        if (now != i) pmap_load(i);
        pcb_pool[now = i].keyboardListener(code);
      }
      if (!wasHalted && now != current_pid) pmap_load(current_pid);
      irq_eoi();
      break;
    }
    case IRQ_OFFSET + IRQ_IDE:
      // ignore, triggered when iret to user
      irq_eoi();
      break;
    default:
      warn("Unhandled interrupt %d occurred at 0x%x.", tf->tf_trapno, tf->tf_eip);
      break;
  }
  if (wasHalted) sched();
}
