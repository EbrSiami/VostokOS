KERNEL := kernel.elf
ISO := image.iso

CC := x86_64-elf-gcc
LD := x86_64-elf-ld

CFLAGS := -O2 -g -Wall -Wextra -ffreestanding \
          -mcmodel=kernel \
          -mno-red-zone \
          -fno-stack-protector \
          -fno-stack-check \
          -I.

LDFLAGS := -nostdlib -static -T linker.ld

# Source files
KERNEL_SRC := kernel.c
DISPLAY_SRC := display/framebuffer.c display/terminal.c
FONT_SRC := font/font_data.c
LIB_SRC := lib/string.c lib/printk.c

# Object files
KERNEL_OBJ := $(KERNEL_SRC:.c=.o)
DISPLAY_OBJ := $(DISPLAY_SRC:.c=.o)
FONT_OBJ := $(FONT_SRC:.c=.o)
LIB_OBJ := $(LIB_SRC:.c=.o)

ALL_OBJ := $(KERNEL_OBJ) $(DISPLAY_OBJ) $(FONT_OBJ) $(LIB_OBJ)

all: $(ISO)

# Compile kernel
kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

# Compile display modules
display/%.o: display/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile font
font/%.o: font/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile lib
lib/%.o: lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(KERNEL): $(ALL_OBJ) linker.ld
	$(LD) $(LDFLAGS) $(ALL_OBJ) -o $(KERNEL)

# Create ISO
$(ISO): $(KERNEL) limine.conf
	mkdir -p iso_root
	cp $(KERNEL) iso_root/
	cp limine.conf iso_root/
	cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin iso_root/
	xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(ISO)
	./limine/limine bios-install $(ISO)

run: $(ISO)
	qemu-system-x86_64 -cdrom $(ISO) -m 512M

clean:
	rm -rf *.o display/*.o font/*.o lib/*.o *.elf *.iso iso_root

.PHONY: all run clean