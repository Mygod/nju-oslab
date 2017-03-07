#ifndef SERIAL_H
#define SERIAL_H

#include "x86.h"

#define PORT_COM1 0x3f8

static inline void serial_init() {
  outb(PORT_COM1 + 1, 0x00);
  outb(PORT_COM1 + 3, 0x80);
  outb(PORT_COM1 + 0, 0x03);
  outb(PORT_COM1 + 1, 0x00);
  outb(PORT_COM1 + 3, 0x03);
  outb(PORT_COM1 + 2, 0xC7);
  outb(PORT_COM1 + 4, 0x0B);
}

static inline int serial_idle() {
  return inb(PORT_COM1 + 5) & 0x20;
}

static inline void serial_putchar(uint8_t ch) {
  while (!serial_idle()); // spin wait
  outb(PORT_COM1, ch);
}

#endif
