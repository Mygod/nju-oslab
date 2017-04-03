#include "irq.h"
#include "serial.h"
#include "tests.h"

#define IRQ_OFFSET 0x20
#define IRQ_SLAVE 2

static inline void irq_init(uint16_t mask) {
  // modify interrupt masks
  outb(IO_PIC1 + 1, (uint8_t) mask);
  outb(IO_PIC2 + 1, (uint8_t) (mask >> 8));

  // Set up master (8259A-1)

  // ICW1:  0001g0hi
  //    g:  0 = edge triggering, 1 = level triggering
  //    h:  0 = cascaded PICs, 1 = master only
  //    i:  0 = no ICW4, 1 = ICW4 required
  outb(IO_PIC1, 0x11);

  // ICW2:  Vector offset
  outb(IO_PIC1+1, IRQ_OFFSET);

  // ICW3:  bit mask of IR lines connected to slave PICs (master PIC),
  //        3-bit No of IR line at which slave connects to master(slave PIC).
  outb(IO_PIC1+1, 1 << IRQ_SLAVE);

  // ICW4:  000nbmap
  //    n:  1 = special fully nested mode
  //    b:  1 = buffered mode
  //    m:  0 = slave PIC, 1 = master PIC
  //    (ignored when b is 0, as the master/slave role
  //    can be hardwired).
  //    a:  1 = Automatic EOI mode
  //    p:  0 = MCS-80/85 mode, 1 = intel x86 mode
  outb(IO_PIC1+1, 0x1);

  // Set up slave (8259A-2)
  outb(IO_PIC2, 0x11);              // ICW1
  outb(IO_PIC2 + 1, IRQ_OFFSET + 8);// ICW2
  outb(IO_PIC2 + 1, IRQ_SLAVE);     // ICW3
  // NB Automatic EOI mode doesn't tend to work on the slave.
  // Linux source code says it's "to be investigated".
  outb(IO_PIC2 + 1, 0x01);          // ICW4

  // OCW3:  0ef01prs
  //   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
  //    p:  0 = no polling, 1 = polling mode
  //   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
  outb(IO_PIC1, 0x68);             /* clear specific mask */
  outb(IO_PIC1, 0x0a);             /* read IRR by default */

  outb(IO_PIC2, 0x68);             /* OCW3 */
  outb(IO_PIC2, 0x0a);             /* OCW3 */
}

#define PORT_CH_0 0x40
#define PORT_CMD 0x43
#define PIT_FREQUENCE 1193182

union CmdByte {
  struct {
    uint8_t present_mode : 1;
    uint8_t operate_mode : 3;
    uint8_t access_mode  : 2;
    uint8_t channel      : 2;
  };
  uint8_t val;
};

static inline void pit_init(int hz) {
  union CmdByte mode = {
      .present_mode = 0,  // 16-bit binary
      .operate_mode = 2,  // rate generator, for more accuracy
      .access_mode  = 3,  // low byte / high byte, see below
      .channel      = 0,  // use channel 0
  };

  int counter = PIT_FREQUENCE / hz;

  outb(PORT_CMD, mode.val);
  outb(PORT_CH_0, (uint8_t) (counter & 0xFF));         // access low byte
  outb(PORT_CH_0, (uint8_t) ((counter >> 8) & 0xFF));  // access high byte
}

extern void env_init();

int main() {
  serial_init();
  pit_init(100);
  // Interrupt 0: Timer
  // Interrupt 1: Keyboard
  // Interrupt 2: Slave 8259A
  irq_init(0xFFF8);
  env_init();

  //test_printk();
  __asm __volatile("sti");  // test interrupt gates and trap gates
  for (;;) __asm __volatile("hlt");
  return 0;
}
