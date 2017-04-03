#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

enum {
  SYS_printk = 0
};

// The following methods are implemented differently in kernel (direct calls) and user space (syscalls)
void sys_printk(const char *out, size_t size);

#endif
