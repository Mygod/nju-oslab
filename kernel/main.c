#include <stdarg.h>

#include "include/serial.h"

#define terminate for (;;)

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 *
 * Source: http://www.strudel.org.uk/itoa/
 */
//char* itoa(int value, char* result) {
//  char* ptr = result, *ptr1 = result, tmp_char;
//  int tmp_value;
//
//  do {
//    tmp_value = value;
//    value /= 10;
//    *ptr++ = "9876543210123456789" [9 + (tmp_value - value * 10)];
//  } while ( value );
//
//  // Apply negative sign
//  if (tmp_value < 0) *ptr++ = '-';
//  *ptr-- = '\0';
//  while(ptr1 < ptr) {
//    tmp_char = *ptr;
//    *ptr--= *ptr1;
//    *ptr1++ = tmp_char;
//  }
//  return result;
//}

void printk(const char *format, ...) {
  va_list args;
  va_start(args, format);
//  char buffer[16];
  for (const char *i = format; *i; ++i) if (*i != '%') serial_putchar((uint8_t) *i); else switch (*++i) {
        case 'c':
          serial_putchar(va_arg(args, int));
          break;
        case 'd':
          //itoa(va_arg(args, int), buffer, 10);
        case 's':
        case 'x':
        default:
          break;
  }
  va_end(args);
}

int main() {
  serial_init();
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
  terminate;
  return 0;
}
