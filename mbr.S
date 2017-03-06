.code16


.globl start
start:
  cli
  mov $0x13, %ah
  mov $0x01, %al
  mov $0x00, %bh
  mov $0x4F, %bl
  mov $(end-str), %cx
  mov $str, %dx
  mov %dx, %bp
  mov $0x00, %dh
  mov $0x00, %dl
  int $0x10
loop:
  jmp loop

str:
  .ascii "Hello, world!"
end:
