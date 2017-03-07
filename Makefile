BOOT   := boot.bin
KERNEL := kernel.bin
IMAGE  := disk.bin

CC      := gcc
LD      := ld
OBJCOPY := objcopy
DD      := dd
QEMU    := qemu-system-i386
GDB     := gdb

CFLAGS := -Wall -Werror -Wfatal-errors
CFLAGS += -MD
CFLAGS += -std=gnu11 -m32 -c
CFLAGS += -I . -I include
CFLAGS += -O0
CFLAGS += -fno-builtin -fno-stack-protector
CFLAGS += -ggdb3

QEMU_OPTIONS := -serial stdio
#QEMU_OPTIONS += -d int
QEMU_OPTIONS += -monitor telnet:127.0.0.1:1111,server,nowait #telnet monitor

QEMU_DEBUG_OPTIONS := -S
QEMU_DEBUG_OPTIONS += -s

GDB_OPTIONS := -ex "target remote 127.0.0.1:1234"
GDB_OPTIONS += -ex "symbol $(KERNEL)"
#GDB_OPTIONS += -ex "b *0x7c00"
GDB_OPTIONS += -ex "b main"
GDB_OPTIONS += -ex "b trap"
GDB_OPTIONS += -ex "c"

OBJ_DIR        := obj
LIB_DIR        := lib
BOOT_DIR       := boot
KERNEL_DIR     := kernel
OBJ_LIB_DIR    := $(OBJ_DIR)/$(LIB_DIR)
OBJ_BOOT_DIR   := $(OBJ_DIR)/$(BOOT_DIR)
OBJ_KERNEL_DIR := $(OBJ_DIR)/$(KERNEL_DIR)

LD_SCRIPT := $(shell find $(KERNEL_DIR) -name "*.ld")

LIB_C := $(wildcard $(LIB_DIR)/*.c)
LIB_O := $(LIB_C:%.c=$(OBJ_DIR)/%.o)

BOOT_S := $(wildcard $(BOOT_DIR)/*.S)
BOOT_C := $(wildcard $(BOOT_DIR)/*.c)
BOOT_O := $(BOOT_S:%.S=$(OBJ_DIR)/%.o)
BOOT_O += $(BOOT_C:%.c=$(OBJ_DIR)/%.o)

KERNEL_C := $(shell find $(KERNEL_DIR) -name "*.c")
KERNEL_S := $(wildcard $(KERNEL_DIR)/*.S)
KERNEL_O := $(KERNEL_C:%.c=$(OBJ_DIR)/%.o)
KERNEL_O += $(KERNEL_S:%.S=$(OBJ_DIR)/%.o)

$(IMAGE): $(BOOT) $(KERNEL)
	@$(DD) if=/dev/zero of=$(IMAGE) count=10240         > /dev/null 2> /dev/null
	@$(DD) if=$(BOOT) of=$(IMAGE) conv=notrunc          > /dev/null 2> /dev/null
	@$(DD) if=$(KERNEL) of=$(IMAGE) seek=1 conv=notrunc > /dev/null 2> /dev/null

$(BOOT): $(BOOT_O)
	$(LD) -e start -Ttext=0x7C00 -m elf_i386 -nostdlib -o $@.out $^
	$(OBJCOPY) --strip-all --only-section=.text --output-target=binary $@.out $@
	@rm $@.out
	./make-mbr $@

$(OBJ_BOOT_DIR)/%.o: $(BOOT_DIR)/%.S
	@mkdir -p $(OBJ_BOOT_DIR)
	$(CC) $(CFLAGS) -Os $< -o $@

$(OBJ_BOOT_DIR)/%.o: $(BOOT_DIR)/%.c
	@mkdir -p $(OBJ_BOOT_DIR)
	$(CC) $(CFLAGS) -Os $< -o $@

$(KERNEL): $(LD_SCRIPT)
$(KERNEL): $(KERNEL_O) $(LIB_O)
	$(LD) -m elf_i386 -T $(LD_SCRIPT) -nostdlib -o $@ $^ $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

$(OBJ_LIB_DIR)/%.o : $(LIB_DIR)/%.c
	@mkdir -p $(OBJ_LIB_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(OBJ_KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.[cS]
	mkdir -p $(OBJ_DIR)/$(dir $<)
	$(CC) $(CFLAGS) -Wno-error=main -Wno-main $< -o $@

DEPS := $(shell find -name "*.d")
-include $(DEPS)

.PHONY: qemu debug gdb clean

qemu: $(IMAGE)
	$(QEMU) $(QEMU_OPTIONS) $(IMAGE)

# Faster, but not suitable for debugging
qemu-kvm: $(IMAGE)
	$(QEMU) $(QEMU_OPTIONS) --enable-kvm $(IMAGE)

debug: $(IMAGE)
	$(QEMU) $(QEMU_DEBUG_OPTIONS) $(QEMU_OPTIONS) $(IMAGE)

gdb:
	$(GDB) $(GDB_OPTIONS)

clean:
	@rm -rf $(OBJ_DIR) 2> /dev/null
	@rm -rf $(BOOT)    2> /dev/null
	@rm -rf $(KERNEL)  2> /dev/null
	@rm -rf $(IMAGE)   2> /dev/null
