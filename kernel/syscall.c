#include "assert.h"
#include "error.h"
#include "memlayout.h"
#include "serial.h"
#include "syscall.h"
#include "trap.h"
#include "kernel/pcb.h"
#include "kernel/pmap.h"

extern void sched() __attribute__((noreturn));

__attribute__((noreturn)) void sys_exit(int code) {
  printk("[Kernel] Process #%d exited with code %d.\n", current_pid + 1, code);
  pcb_free(&pcb_pool[current_pid]);
  sched();
}

void sys_printk(const char *out, size_t size) {
  for (size_t i = 0; i < size; ++i) serial_putchar((uint8_t) out[i]);
}

void sys_sleep(int ticks) {
  pcb_pool[current_pid].wakeTime = sys_time + ticks;
  sched();
}

KeyboardListener sys_listenKeyboard(KeyboardListener handler) {
  KeyboardListener old = pcb_pool[current_pid].keyboardListener;
  pcb_pool[current_pid].keyboardListener = handler;
  return old;
}

ClockListener sys_listenClock(ClockListener handler) {
  ClockListener old = pcb_pool[current_pid].clockListener;
  pcb_pool[current_pid].clockListener = handler;
  return old;
}

uint8_t sys_drawPoint(uint16_t x, uint16_t y, uint8_t color) {
  uint8_t *p = (uint8_t *) KERNBASE + IOPHYSMEM + y * 320 + x, old = *p;
  *p = color;
  return old;
}

__attribute__((noreturn)) void sys_crash() {
  for (;;) __asm __volatile("cli; hlt");
}

int sys_getpid() {
  return current_pid + 1;
}

int sys_fork() {
  int new_pid = 0;
  while (new_pid < PROCESS_POOL_SIZE && pcb_pool[new_pid].used) ++new_pid;
  assert(new_pid < PROCESS_POOL_SIZE);  // no more processes available? NOOOOOOooooooo....
  pcb_copy(&pcb_pool[new_pid], &pcb_pool[current_pid]);
  pcb_pool[new_pid].tf.tf_regs.reg_eax = (uint32_t) (new_pid + 1);
  pmap_copy(new_pid, current_pid);
  return E_SUCCESS;
}

int32_t syscall_dispatch(struct Trapframe *tf) {
#define arg1 tf->tf_regs.reg_edx
#define arg2 tf->tf_regs.reg_ecx
#define arg3 tf->tf_regs.reg_ebx
#define arg4 tf->tf_regs.reg_edi
#define arg5 tf->tf_regs.reg_esi
  switch (tf->tf_regs.reg_eax) {  // syscall number
    case SYS_exit: sys_exit(arg1);
    case SYS_printk:
      sys_printk((const char *) arg1, arg2);
      return E_SUCCESS;
    case SYS_sleep: sys_sleep(arg1);  // sys_sleep doesn't return
    case SYS_listenKeyboard: return (int32_t) sys_listenKeyboard((KeyboardListener) arg1);
    case SYS_listenClock: return (int32_t) sys_listenClock((ClockListener) arg1);
    case SYS_drawPoint: return sys_drawPoint((uint16_t) arg1, (uint16_t) arg2, (uint8_t) arg3);
    case SYS_crash: sys_crash();
    case SYS_getpid: return sys_getpid();
    case SYS_fork: return sys_fork();
    case SYS_sem_open: return sys_sem_open(arg1);
    case SYS_sem_close: return sys_sem_close(arg1);
    case SYS_sem_wait: return sys_sem_wait(arg1);
    case SYS_sem_post: return sys_sem_post(arg1);
    case SYS_mmap: return sys_mmap((void *) arg1, arg2);
    default: return E_SYSCALL_NOT_FOUND;
  }
}
