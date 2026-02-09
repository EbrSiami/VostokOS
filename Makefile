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

# Object files
KERNEL_OBJ := $(KERNEL_SRC:.c=.o)
DISPLAY_OBJ := $(DISPLAY_SRC:.c=.o)
FONT_OBJ := $(FONT_SRC:.c=.o)

ALL_OBJ := $(KERNEL_OBJ) $(DISPLAY_OBJ) $(FONT_OBJ)

all: $(ISO)

# Compile kernel
kernel.o: kernel.c
	$(CC) $(CFLAGS) -c kernel.c -o kernel.o

# Compile display modules
display/framebuffer.o: display/framebuffer.c display/framebuffer.h
	$(CC) $(CFLAGS) -c display/framebuffer.c -o display/framebuffer.o

display/terminal.o: display/terminal.c display/terminal.h display/framebuffer.h font/font.h
	$(CC) $(CFLAGS) -c display/terminal.c -o display/terminal.o

# Compile font
font/font_data.o: font/font_data.c font/font.h
	$(CC) $(CFLAGS) -c font/font_data.c -o font/font_data.o

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
	rm -rf *.o display/*.o font/*.o *.elf *.iso iso_root

.PHONY: all run clean