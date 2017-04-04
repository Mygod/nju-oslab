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

void sys_sleep() {
  assert(do_syscall0(SYS_sleep) == E_SUCCESS);
}
