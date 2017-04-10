#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

enum {
  SYS_printk = 0,
  SYS_sleep,
  SYS_listenKeyboard,
  SYS_listenClock,
  SYS_drawPoint,
  SYS_crash,
  SYS_getpid,
};

typedef void (*KeyboardListener)(uint8_t), (*ClockListener)();

// The following methods are implemented differently in kernel (direct calls) and user space (syscalls)
void sys_printk(const char *out, size_t size);
void sys_sleep(int ticks);
// Please do not write malicious code in these handlers as they are executed in kernel space
// TODO: implement proper signal handling which requires long jumps :/
KeyboardListener sys_listenKeyboard(KeyboardListener handler);
ClockListener sys_listenClock(ClockListener handler);
uint8_t sys_drawPoint(uint16_t x, uint16_t y, uint8_t color);
void sys_crash() __attribute__((noreturn));
int sys_getpid();

#endif
