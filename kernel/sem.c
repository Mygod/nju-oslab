#include "error.h"
#include "pcb.h"

#define SEM_POOL_SIZE 3

int sem_pool[SEM_POOL_SIZE];
extern void sched() __attribute__((noreturn));

int sys_sem_open(int sem) {
  return sem < 0 || sem >= SEM_POOL_SIZE ? E_INVID : sem;
}
int sys_sem_close(int sem) {
  return sem < 0 || sem >= SEM_POOL_SIZE ? E_INVID : E_SUCCESS; // wait that doesn't seem to do anything
}
int sys_sem_wait(int sem) {
  if (!sem_pool[sem]) {
    pcb_pool[current_pid].waitSem = sem;
    sched();  // return value = SYS_sem_wait > 0
  }
  --sem_pool[sem];
  return E_SUCCESS;
}
int sys_sem_post(int sem) {
  ++sem_pool[sem];
  return E_SUCCESS;
}
