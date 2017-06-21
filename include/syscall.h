#ifndef SYSCALL_H
#define SYSCALL_H

#include "types.h"

enum {
  SYS_exit = 1,
  SYS_printk,
  SYS_sleep,
  SYS_listenKeyboard,
  SYS_listenClock,
  SYS_drawPoint,
  SYS_crash,
  SYS_getpid,
  SYS_fork,
  SYS_sem_open,
  SYS_sem_close,
  SYS_sem_wait,
  SYS_sem_post,
  SYS_mmap,
  SYS_fs_open,
  SYS_fs_read,
  SYS_fs_write,
  SYS_fs_lseek,
  SYS_fs_close,
};

typedef void (*KeyboardListener)(uint8_t), (*ClockListener)();

// The following methods are implemented differently in kernel (direct calls) and user space (syscalls)
void sys_exit(int code) __attribute__((noreturn));
void sys_printk(const char *out, size_t size);
void sys_sleep(int ticks);
// Please do not write malicious code in these handlers as they are executed in kernel space
// TODO: implement proper signal handling which requires long jumps :/
KeyboardListener sys_listenKeyboard(KeyboardListener handler);
ClockListener sys_listenClock(ClockListener handler);
uint8_t sys_drawPoint(uint16_t x, uint16_t y, uint8_t color);
void sys_crash() __attribute__((noreturn));
int sys_getpid();
// It returns child pid for parent and -1 for children
int sys_fork();
int sys_sem_open(int sem);
int sys_sem_close(int sem);
int sys_sem_wait(int sem);
int sys_sem_post(int sem);
int sys_mmap(void *addr, int id);
int fs_open(const char *pathname, int flags);
int fs_read(int fd, void *buf, int len);
int fs_write(int fd, const void *buf, int len);
int fs_lseek(int fd, int offset, int whence);
int fs_close(int fd);

#define O_RDONLY	     00
#define O_RDWR		     02
#define O_CREAT	       0100
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif
