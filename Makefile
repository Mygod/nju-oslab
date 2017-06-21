CC      := gcc
LD      := ld
OBJCOPY := objcopy
DD      := dd
QEMU    := qemu-system-i386
GDB     := gdb

CFLAGS_SHORT := -Wall -Werror -Wfatal-errors
CFLAGS_SHORT += -Wno-unknown-pragmas -Wno-error=unknown-pragmas
CFLAGS_SHORT += -std=gnu11 -m32
CFLAGS_SHORT += -I .

CFLAGS := $(CFLAGS_SHORT)
CFLAGS += -MD
CFLAGS += -c
CFLAGS += -I include
CFLAGS += -O0
CFLAGS += -fno-builtin -fno-stack-protector
CFLAGS += -ggdb3

BIN_DIR        := bin
OBJ_DIR        := obj
LIB_DIR        := lib
BOOT_DIR       := boot
KERNEL_DIR     := kernel
USER_DIR       := user
UTIL_DIR       := util
MYFS_READ      := $(BIN_DIR)/myfs_read
MYFS_WRITE     := $(BIN_DIR)/myfs_write
OBJ_LIB_DIR    := $(OBJ_DIR)/$(LIB_DIR)
OBJ_BOOT_DIR   := $(OBJ_DIR)/$(BOOT_DIR)
OBJ_KERNEL_DIR := $(OBJ_DIR)/$(KERNEL_DIR)
OBJ_USER_DIR   := $(OBJ_DIR)/$(USER_DIR)

BOOT   := $(BIN_DIR)/boot.bin
KERNEL := $(BIN_DIR)/kernel.bin
USER   := $(BIN_DIR)/user.bin
IMAGE  := $(BIN_DIR)/disk.bin

QEMU_OPTIONS := -serial stdio
#QEMU_OPTIONS += -d int
QEMU_OPTIONS += -monitor telnet:127.0.0.1:1111,server,nowait #telnet monitor
QEMU_OPTIONS += -drive file=$(IMAGE),format=raw

QEMU_DEBUG_OPTIONS := -S
QEMU_DEBUG_OPTIONS += -s

GDB_OPTIONS := -ex "target remote 127.0.0.1:1234"
GDB_OPTIONS += -ex "symbol $(KERNEL)"
#GDB_OPTIONS += -ex "symbol $(USER)"
#GDB_OPTIONS += -ex "b *0x7c00"
#GDB_OPTIONS += -ex "b entry"
#GDB_OPTIONS += -ex "b trap"
#GDB_OPTIONS += -ex "b i386_init"
GDB_OPTIONS += -ex "b *0xf0100e74"
GDB_OPTIONS += -ex "b _panic"
#GDB_OPTIONS += -ex "c"
#GDB_OPTIONS += -ex "b kernel/trap.c:115"
GDB_OPTIONS += -ex "c"
GDB_OPTIONS += -ex "layout split"

LIB_C := $(wildcard $(LIB_DIR)/*.c)
LIB_O := $(LIB_C:%.c=$(OBJ_DIR)/%.o)

BOOT_S := $(wildcard $(BOOT_DIR)/*.S)
BOOT_C := $(wildcard $(BOOT_DIR)/*.c)
BOOT_O := $(BOOT_S:%.S=$(OBJ_DIR)/%.o)
BOOT_O += $(BOOT_C:%.c=$(OBJ_DIR)/%.o)

KERNEL_LD := $(shell find $(KERNEL_DIR) -name "*.ld")
KERNEL_C := $(shell find $(KERNEL_DIR) -name "*.c")
KERNEL_S := $(wildcard $(KERNEL_DIR)/*.S)
KERNEL_O := $(KERNEL_C:%.c=$(OBJ_DIR)/%.o)
KERNEL_O += $(KERNEL_S:%.S=$(OBJ_DIR)/%.o)

USER_C := $(shell find $(USER_DIR) -name "*.c")
USER_O := $(USER_C:%.c=$(OBJ_DIR)/%.o)

all: $(IMAGE) $(MYFS_READ)

$(IMAGE): $(BOOT) $(KERNEL) $(USER) $(MYFS_WRITE)
	@mkdir -p $(BIN_DIR)
	@$(DD) if=/dev/zero of=$(IMAGE) count=4099          > /dev/null 2> /dev/null
	@$(DD) if=$(BOOT) of=$(IMAGE) conv=notrunc          > /dev/null 2> /dev/null
	@$(MYFS_WRITE) $(IMAGE) kernel.bin                  < $(KERNEL)
	@$(MYFS_WRITE) $(IMAGE) user.bin                    < $(USER)

$(BOOT): $(BOOT_O)
	@mkdir -p $(BIN_DIR)
	$(LD) -e start -Ttext=0x7C00 -m elf_i386 -nostdlib -o $@.out $^
	$(OBJCOPY) --strip-all --only-section=.text --output-target=binary $@.out $@
	@rm $@.out
	@util/make-mbr $@

$(OBJ_BOOT_DIR)/%.o: $(BOOT_DIR)/%.S
	@mkdir -p $(OBJ_BOOT_DIR)
	$(CC) $(CFLAGS) -Os $< -o $@

$(OBJ_BOOT_DIR)/%.o: $(BOOT_DIR)/%.c
	@mkdir -p $(OBJ_BOOT_DIR)
	$(CC) $(CFLAGS) -Os $< -o $@

$(KERNEL): $(KERNEL_LD)
$(KERNEL): $(KERNEL_O) $(LIB_O)
	@mkdir -p $(BIN_DIR)
	$(LD) -m elf_i386 -T $(KERNEL_LD) -nostdlib -o $@ $^ $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

$(USER): $(USER_O) $(LIB_O)
	@mkdir -p $(BIN_DIR)
	$(LD) -m elf_i386 -emain -nostdlib -o $@ $^ $(shell $(CC) $(CFLAGS) -print-libgcc-file-name)

$(OBJ_LIB_DIR)/%.o : $(LIB_DIR)/%.c
	@mkdir -p $(OBJ_LIB_DIR)
	$(CC) $(CFLAGS) $< -o $@

$(OBJ_KERNEL_DIR)/%.o: $(KERNEL_DIR)/%.[cS]
	mkdir -p $(OBJ_DIR)/$(dir $<)
	$(CC) $(CFLAGS) -Wno-error=main -Wno-main $< -o $@

$(OBJ_USER_DIR)/%.o: $(USER_DIR)/%.c
	mkdir -p $(OBJ_DIR)/$(dir $<)
	$(CC) $(CFLAGS) $< -o $@

$(BIN_DIR)/myfs_%: $(UTIL_DIR)/myfs_%.c $(UTIL_DIR)/common.c $(OBJ_KERNEL_DIR)/fs.o
	mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS_SHORT) $^ -o $@

DEPS := $(shell find -name "*.d")
-include $(DEPS)

.PHONY: qemu debug gdb clean

qemu: $(IMAGE)
	@$(QEMU) $(QEMU_OPTIONS)

# Faster, but not suitable for debugging
qemu-kvm: $(IMAGE)
	@$(QEMU) $(QEMU_OPTIONS) --enable-kvm

debug: $(IMAGE)
	@$(QEMU) $(QEMU_DEBUG_OPTIONS) $(QEMU_OPTIONS)

gdb:
	$(GDB) $(GDB_OPTIONS)

fs-test: $(IMAGE) $(MYFS_READ)
	$(MYFS_READ) $(IMAGE) kernel.bin > /tmp/test.bin && cmp -b $(KERNEL) /tmp/test.bin
	$(MYFS_READ) $(IMAGE) user.bin   > /tmp/test.bin && cmp -b $(USER) /tmp/test.bin

clean:
	@rm -rf $(BIN_DIR) 2> /dev/null
	@rm -rf $(OBJ_DIR) 2> /dev/null
	@rm -rf $(BOOT)    2> /dev/null
	@rm -rf $(KERNEL)  2> /dev/null
	@rm -rf $(IMAGE)   2> /dev/null

STUNO?=archive
submit:
	@git ls-files | tar zcf $(BIN_DIR)/$(STUNO).tar.gz .git -T -
