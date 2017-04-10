#include "assert.h"
#include "syscall.h"

static inline void test_printk() {
  printk("Printk test begin...\n");
  printk("the answer should be:\n");
  printk("#######################################################\n");
  printk("Hello, welcome to OSlab! I'm the body of the game. ");
  printk("Bootblock loads me to the memory position of 0x100000, and Makefile also tells me that I'm at the location of 0x100000. ");
  printk("~!@#$^&*()_+`1234567890-=...... ");
  printk("Now I will test your printk: ");
  printk("1 + 1 = 2, 123 * 456 = 56088\n0, -1, -2147483648, -1412505855, -32768, 102030\n0, ffffffff, 80000000, abcdef01, ffff8000, 18e8e\n");
  printk("#######################################################\n");
  printk("your answer:\n");
  printk("=======================================================\n");
  printk("%s %s%scome %co%s", "Hello,", "", "wel", 't', " ");
  printk("%c%c%c%c%c! ", 'O', 'S', 'l', 'a', 'b');
  printk("I'm the %s of %s. %s 0x%x, %s 0x%x. ", "body", "the game", "Bootblock loads me to the memory position of",
         0x100000, "and Makefile also tells me that I'm at the location of", 0x100000);
  printk("~!@#$^&*()_+`1234567890-=...... ");
  printk("Now I will test your printk: ");
  printk("%d + %d = %d, %d * %d = %d\n", 1, 1, 1 + 1, 123, 456, 123 * 456);
  printk("%d, %d, %d, %d, %d, %d\n", 0, 0xffffffff, 0x80000000, 0xabcedf01, -32768, 102030);
  printk("%x, %x, %x, %x, %x, %x\n", 0, 0xffffffff, 0x80000000, 0xabcedf01, -32768, 102030);
  printk("=======================================================\n");
  printk("Test end!!! Good luck!!!\n");
}

// This is a (pseudo) library function but I'm lazy to create another file for it
void drawLine(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint8_t color) {
  if (x0 > x1) {
    uint16_t t = x0;
    x0 = x1;
    x1 = t;
    t = y0;
    y0 = y1;
    y1 = t;
  }
  // Bresenham's line algorithm
  int16_t deltaX = x1 - x0, deltaY = y1 - y0;
  float deltaErr = (float) deltaY / deltaX;
  if (deltaErr < 0) deltaErr = -deltaErr;
  float error = deltaErr - .5f;
  for (uint16_t y = y0, x = x0; x <= x1; ++x) {
    sys_drawPoint(x, y, color);
    error += deltaErr;
    if (error >= .5f) {
      ++y;
      error -= 1;
    } else if (error <= -.5f) {
      --y;
      error += 1;
    }
  }
}

uint32_t random;
#define NEXT_RANDOM do {    \
  if (!random) random = 1;  \
  random ^= random << 13;   \
  random ^= random >> 17;   \
  random ^= random << 5;    \
} while (false)
void onKeyboard(uint8_t code) {
  printk("Keyboard 0x%x pressed!\n", code);
  for (uint16_t y = 0; y < 200; ++y) for (uint16_t x = 0; x < 320; ++x) {
    sys_drawPoint(x, y, (uint8_t) random);
    NEXT_RANDOM;
  }
}
void onClock() {
  uint32_t x0 = random % 320, y0 = (random / 320) % 200;
  NEXT_RANDOM;
  drawLine((uint16_t) x0, (uint16_t) y0, (uint16_t) (random % 320), (uint16_t) ((random / 320) % 200),
           (uint8_t) (random >> 24));
  NEXT_RANDOM;
}

int main() {
  assert(!random);
  sys_listenKeyboard(onKeyboard);
  sys_listenClock(onClock);
  printk("Current pid: %d\n", sys_getpid());
  // test_printk();
  for (;;) sys_sleep();
}
