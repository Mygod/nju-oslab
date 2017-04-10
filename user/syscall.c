#include "assert.h"
#include "error.h"
#include "syscall.h"
#include "trap.h"

static inline int32_t do_syscall0(int syscallno) {
  int32_t ret;
  asm volatile("int %1\n"
    : "=a" (ret)
    : "i" (T_SYSCALL),
      "a" (syscallno)
    : "cc", "memory");
  return ret;
}
static inline int32_t do_syscall1(int syscallno, uint32_t a1) {
  int32_t ret;
  asm volatile("int %1\n"
    : "=a" (ret)
    : "i" (T_SYSCALL),
      "a" (syscallno),
      "d" (a1)
    : "cc", "memory");
  return ret;
}
static inline int32_t do_syscall2(int syscallno, uint32_t a1, uint32_t a2) {
  int32_t ret;
  asm volatile("int %1\n"
    : "=a" (ret)
    : "i" (T_SYSCALL),
      "a" (syscallno),
      "d" (a1),
      "c" (a2)
    : "cc", "memory");
  return ret;
}
static inline int32_t do_syscall3(int syscallno, uint32_t a1, uint32_t a2, uint32_t a3) {
  int32_t ret;
  asm volatile("int %1\n"
    : "=a" (ret)
    : "i" (T_SYSCALL),
      "a" (syscallno),
      "d" (a1),
      "c" (a2),
      "b" (a3)
    : "cc", "memory");
  return ret;
}
static inline int32_t do_syscall4(int syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4) {
  int32_t ret;
  asm volatile("int %1\n"
    : "=a" (ret)
    : "i" (T_SYSCALL),
      "a" (syscallno),
      "d" (a1),
      "c" (a2),
      "b" (a3),
      "D" (a4)
    : "cc", "memory");
  return ret;
}
static inline int32_t do_syscall5(int syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5) {
  int32_t ret;
  asm volatile("int %1\n"
    : "=a" (ret)
    : "i" (T_SYSCALL),
      "a" (syscallno),
      "d" (a1),
      "c" (a2),
      "b" (a3),
      "D" (a4),
      "S" (a5)
    : "cc", "memory");
  return ret;
}


void sys_printk(const char *out, size_t size) {
  assert(do_syscall2(SYS_printk, (uint32_t) out, (uint32_t) size) == E_SUCCESS);
}

void sys_sleep(int ticks) {
  do_syscall1(SYS_sleep, (uint32_t) ticks); // this call won't return success
}

KeyboardListener keyboardListener;
void _keyboardListener(uint8_t code) {
  if (keyboardListener != NULL) keyboardListener(code);
}
KeyboardListener sys_listenKeyboard(KeyboardListener handler) {
  static bool registered = false;
  if (!registered) {
    do_syscall1(SYS_listenKeyboard, (uint32_t) &_keyboardListener);
    registered = true;
  }
  KeyboardListener old = keyboardListener;
  keyboardListener = handler;
  return old;
}

ClockListener clockListener;
void _clockListener() {
  if (clockListener != NULL) clockListener();
}
ClockListener sys_listenClock(ClockListener handler) {
  static bool registered = false;
  if (!registered) {
    do_syscall1(SYS_listenClock, (uint32_t) &_clockListener);
    registered = true;
  }
  ClockListener old = clockListener;
  clockListener = handler;
  return old;
}

uint8_t sys_drawPoint(uint16_t x, uint16_t y, uint8_t color) {
  int32_t result = do_syscall3(SYS_drawPoint, x, y, color);
  assert(result >= E_SUCCESS);
  return (uint8_t) result;
}

__attribute__((noreturn)) void sys_crash() {
  for (;;) do_syscall0(SYS_crash);
}

int sys_getpid() {
  return do_syscall0(SYS_getpid);
}
