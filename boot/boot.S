#include "mmu.h"

.code16
.globl start
start:
  cli

# Enable A20
seta20.1:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.1

  movb    $0xd1,%al               # 0xd1 -> port 0x64
  outb    %al,$0x64

seta20.2:
  inb     $0x64,%al               # Wait for not busy
  testb   $0x2,%al
  jnz     seta20.2

  movb    $0xdf,%al               # 0xdf -> port 0x60
  outb    %al,$0x60

# Mode 13h
  movw  $0x13, %ax
  int   $0x10

# VBE mode
# movw  $0x4f02, %ax
# movw  $0x0118, %bx    # 1280×1024 24-bit mode
# int   $0x10

  lgdt gdtdesc
  movl %cr0, %eax
  orl  $0x1, %eax       # Protected mode enable flag
  movl %eax, %cr0
  ljmp $0x8, $next

.code32
next:
  movw  $0x10, %ax      # Data segment selector
  movw  %ax, %ds
  movw  %ax, %es
  movw  %ax, %fs
  movw  %ax, %gs
  movw  %ax, %ss
  movl  $0xf00000, %esp
  call  bootloader

.p2align 2
gdt:
  SEG_NULL                          # null segment
  SEG(STA_X|STA_R, 0x0, 0xffffffff) # code segment
  SEG(STA_W, 0x0, 0xffffffff)       # data segment

gdtdesc:
  .word 0x17    # sizeof(gdt) - 1
  .long gdt     # address
