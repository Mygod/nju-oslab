#include "assert.h"
#include "mmu.h"
#include "string.h"
#include "syscall.h"

static size_t test_printk(char *out, size_t size) {
  size_t count = 0;
  count += snprintf(out + count, size > count ? size - count : 0,
                    "Printk test begin...\n");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "the answer should be:\n");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "#######################################################\n");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "Hello, welcome to OSlab! I'm the body of the game. ");
  count += snprintf(out + count,
                    size > count ? size - count : 0,
                    "Bootblock loads me to the memory position of 0x100000, and Makefile also tells me that I'm at the location of 0x100000. ");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "~!@#$^&*()_+`1234567890-=...... ");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "Now I will test your printk: ");
  count += snprintf(out + count,
                    size > count ? size - count : 0,
                    "1 + 1 = 2, 123 * 456 = 56088\n0, -1, -2147483648, -1412505855, -32768, 102030\n0, ffffffff, 80000000, abcdef01, ffff8000, 18e8e\n");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "#######################################################\n");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "your answer:\n");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "=======================================================\n");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "%s %s%scome %co%s", "Hello,", "", "wel", 't', " ");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "%c%c%c%c%c! ", 'O', 'S', 'l', 'a', 'b');
  count += snprintf(out + count, size > count ? size - count : 0,
                    "I'm the %s of %s. %s 0x%x, %s 0x%x. ", "body",
                    "the game", "Bootblock loads me to the memory position of",
                    0x100000, "and Makefile also tells me that I'm at the location of", 0x100000);
  count += snprintf(out + count, size > count ? size - count : 0,
                    "~!@#$^&*()_+`1234567890-=...... ");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "Now I will test your printk: ");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "%d + %d = %d, %d * %d = %d\n", 1, 1, 1 + 1, 123, 456, 123 * 456);
  count += snprintf(out + count, size > count ? size - count : 0,
                    "%d, %d, %d, %d, %d, %d\n", 0, 0xffffffff, 0x80000000, 0xabcedf01, -32768, 102030);
  count += snprintf(out + count, size > count ? size - count : 0,
                    "%x, %x, %x, %x, %x, %x\n", 0, 0xffffffff, 0x80000000, 0xabcedf01, -32768, 102030);
  count += snprintf(out + count, size > count ? size - count : 0,
                    "=======================================================\n");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "Test end!!! Good luck!!!\n");
  for (int i = 0; i < 400; ++i) count += snprintf(out + count, size > count ? size - count : 0, "PLACEHOLDER");
  count += snprintf(out + count, size > count ? size - count : 0,
                    "\nTest size excluding this line: %d\n", count);
  return count;
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

__attribute__((__aligned__(PGSIZE)))
static uint8_t sharedMem[PGSIZE];

void process0() {
  sys_mmap(sharedMem, 0);
  int bufferMutex = sys_sem_open(0), fillCount = sys_sem_open(1), emptyCount = sys_sem_open(2);
  sys_sem_post(emptyCount); // ready to take input
  for (;;) {
    printk("\n(parent process will now block and wait for input)\n");
    sys_sem_wait(fillCount);
    sys_sem_wait(bufferMutex);
    if (!*sharedMem) break;
    printk("%s", sharedMem);
    sys_sem_post(bufferMutex);
    sys_sem_post(emptyCount);
  }
  printk("\n(parent process has finished reading, starting game)\n");
  sys_sem_post(bufferMutex);
  sys_sem_post(emptyCount);
  sys_sem_close(bufferMutex);
  sys_sem_close(fillCount);
  sys_sem_close(emptyCount);

  sys_listenKeyboard(onKeyboard);
  sys_listenClock(onClock);
  for (;;) {
    sys_sleep(0x7fffffff);
    warn("sys_sleep(forever) returned. You really have run this program for too long.");
  }
}

void process1() {
  printk("Child process is generating buffer to output.\n");
  size_t count = test_printk(NULL, 0) + 1;
  char buffer[count];
  test_printk(buffer, count);
  sys_mmap(sharedMem, 0);
  int bufferMutex = sys_sem_open(0), fillCount = sys_sem_open(1), emptyCount = sys_sem_open(2);
  for (size_t i = 0; i < count; i += PGSIZE - 1) {
    sys_sem_wait(emptyCount);
    sys_sem_wait(bufferMutex);
    memcpy(sharedMem, buffer + i, PGSIZE - 1 + i > count ? PGSIZE - 1 + i - count : PGSIZE - 1);
    sharedMem[PGSIZE - 1] = 0;
    sys_sem_post(bufferMutex);
    sys_sem_post(fillCount);
  }
  sys_sem_wait(emptyCount);
  sys_sem_wait(bufferMutex);
  sharedMem[0] = 0;
  sys_sem_post(bufferMutex);
  sys_sem_post(fillCount);
  sys_sem_close(bufferMutex);
  sys_sem_close(fillCount);
  sys_sem_close(emptyCount);
  sys_exit(0);
}

int main() {
  assert(!random);
  int bufferMutex = sys_sem_open(0);
  sys_sem_post(bufferMutex);  // initialize *bufferMutex = 1
  sys_sem_close(bufferMutex);
  if (sys_fork() > 0) process1(); else process0();
}
