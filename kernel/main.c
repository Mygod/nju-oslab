#include "common.h"
#include "serial.h"
#include "tests.h"

int main() {
  serial_init();
  test_printk();
  terminate;
  return 0;
}
