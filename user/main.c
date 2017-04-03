#include "syscall.h"

const char testMessage[] = "Hello, world from user space!\n";

void sys_printk(const char *out, size_t size) {}  // TODO: implement me

int main() {
  sys_printk(testMessage, sizeof(testMessage) - 1);
  return 0;
  // TODO: exit gracefully
}
