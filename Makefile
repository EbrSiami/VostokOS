KERNEL := kernel.elf
ISO := VostokOS.iso

CC := x86_64-elf-gcc
LD := x86_64-elf-ld
AS := x86_64-elf-as

# note i'll turn off the AVX, SSE for now. 

CFLAGS := -O2 -g -Wall -Wextra -ffreestanding \
          -march=x86-64 \
		  -fno-tree-vectorize \
          -mcmodel=kernel \
          -mno-red-zone \
          -fno-stack-protector \
          -fno-stack-check \
          -I.

ASFLAGS := 

LDFLAGS := -nostdlib -static -T linker.ld

# Source files
KERNEL_SRC := kernel.c kernel/sched.c
DISPLAY_SRC := display/framebuffer.c display/terminal.c
FONT_SRC := font/font_data.c
LIB_SRC := lib/string.c lib/printk.c lib/memory.c lib/bitmap.c lib/panic.c lib/spinlock.c
ARCH_SRC := arch/gdt.c arch/idt.c arch/pic.c arch/apic.c
ARCH_ASM := arch/gdt_asm.s arch/idt_asm.s
DRIVERS_SRC := drivers/keyboard.c drivers/timer.c drivers/acpi.c drivers/pci.c
SHELL_SRC := shell/shell.c shell/commands.c
MM_SRC := mm/pmm.c mm/vmm.c mm/heap.c
FS_SRC := fs/vfs.c fs/tar.c

# Object files
KERNEL_OBJ := $(KERNEL_SRC:.c=.o)
DISPLAY_OBJ := $(DISPLAY_SRC:.c=.o)
FONT_OBJ := $(FONT_SRC:.c=.o)
LIB_OBJ := $(LIB_SRC:.c=.o)
ARCH_OBJ := $(ARCH_SRC:.c=.o)
ARCH_ASM_OBJ := $(ARCH_ASM:.s=.o)
DRIVERS_OBJ := $(DRIVERS_SRC:.c=.o)
SHELL_OBJ := $(SHELL_SRC:.c=.o)
MM_OBJ := $(MM_SRC:.c=.o)
FS_OBJ := $(FS_SRC:.c=.o)

ALL_OBJ := $(KERNEL_OBJ) $(DISPLAY_OBJ) $(FONT_OBJ) $(LIB_OBJ) $(ARCH_OBJ) $(ARCH_ASM_OBJ) $(DRIVERS_OBJ) $(SHELL_OBJ) $(MM_OBJ) $(FS_OBJ)

all: $(ISO)

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile assembly files
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

drivers/%.o: drivers/%.c
	$(CC) $(CFLAGS) -c $< -o $@

shell/%.o: shell/%.c
	$(CC) $(CFLAGS) -c $< -o $@

mm/%.o: mm/%.c
	$(CC) $(CFLAGS) -c $< -o $@

fs/%.o: fs/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(KERNEL): $(ALL_OBJ) linker.ld
	$(LD) $(LDFLAGS) $(ALL_OBJ) -o $(KERNEL)

# Create ISO
$(ISO): $(KERNEL) limine.conf
	cd rootfs && tar -cvf ../initrd.tar *
	mkdir -p iso_root
	cp $(KERNEL) iso_root/
	cp limine.conf iso_root/
	cp initrd.tar iso_root/
	cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO)
	./limine/limine bios-install $(ISO)

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 512M

# update clean for drivers etc 
clean:
	rm -rf *.o display/*.o font/*.o lib/*.o arch/*.o drivers/*.o shell/*.o *.elf mm/*.o kernel/*.o fs/*.o *.iso *.tar iso_root

.PHONY: all run clean