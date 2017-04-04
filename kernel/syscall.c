#include "error.h"
#include "serial.h"
#include "syscall.h"
#include "trap.h"
#include "types.h"

void sys_printk(const char *out, size_t size) {
  for (size_t i = 0; i < size; ++i) serial_putchar((uint8_t) out[i]);
}

void sys_sleep() {
  __asm __volatile("sti; hlt; cli");
}

KeyboardListener keyboardListener;
KeyboardListener sys_listenKeyboard(KeyboardListener handler) {
  KeyboardListener old = keyboardListener;
  keyboardListener = handler;
  return old;
}

ClockListener clockListener;
ClockListener sys_listenClock(ClockListener handler) {
  ClockListener old = clockListener;
  clockListener = handler;
  return old;
}

int32_t syscall_dispatch(struct Trapframe *tf) {
#define arg1 tf->tf_regs.reg_edx
#define arg2 tf->tf_regs.reg_ecx
#define arg3 tf->tf_regs.reg_ebx
#define arg4 tf->tf_regs.reg_edi
#define arg5 tf->tf_regs.reg_esi
  switch (tf->tf_regs.reg_eax) {  // syscall number
    case SYS_printk:
      sys_printk((const char *) arg1, arg2);
      return E_SUCCESS;
    case SYS_sleep:
      sys_sleep();
      return E_SUCCESS;
    case SYS_listenKeyboard: return (int32_t) sys_listenKeyboard((KeyboardListener) arg1);
    case SYS_listenClock: return (int32_t) sys_listenClock((ClockListener) arg1);
    default: return E_SYSCALL_NOT_FOUND;
  }
}
