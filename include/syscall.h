#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

enum {
  SYS_printk = 0,
  SYS_sleep,
  SYS_listenKeyboard,
  SYS_listenClock,
};

typedef void (*KeyboardListener)(uint8_t), (*ClockListener)();

// The following methods are implemented differently in kernel (direct calls) and user space (syscalls)
void sys_printk(const char *out, size_t size);
void sys_sleep();
// Please do not write malicious code in these handlers as they are executed in kernel space
// TODO: implement proper signal handling which requires long jumps :/
void sys_listenKeyboard(KeyboardListener handler);
void sys_listenClock(ClockListener handler);

#endif
