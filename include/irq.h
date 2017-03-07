#ifndef IRQ_H
#define IRQ_H

#include "x86.h"

#define IO_PIC1 0x20
#define IO_PIC2 0xA0

static inline void irq_eoi() {
  outb(IO_PIC1, 0x20);
  outb(IO_PIC2, 0x20);
}

#endif
