## Vostok Operating System.

## I dedicate the line-by-line code I wrote to develop this operating system to Sergei Korolev :)

This OS is my first attempt at developing a 64-bit operating system completely from scratch. 

After 4 months of struggling with writing my own custom bootloader, I decided to surrender myself to assembly and architecture constraints and adopted the **Limine Bootloader** to boot the kernel.

### How to get started ? 
### for compiling VostokOS you need a Linux environment (Ubuntu/Debian preferred) with a **Toolchain (Cross-Compiler)** for `x86_64-elf`, along with emulation and ISO creation tools.

#### step 1: Install System Dependencies
```bash
sudo apt update
```

```bash
sudo apt install build-essential bison flex libgmp3-dev libmpfr-dev libmpc-dev texinfo libgmp-dev xorriso qemu-system-x86
```

#### step 2: Define the environment variables
```bash
export PREFIX="$HOME/opt/cross"
```
```bash
export TARGET=x86_64-elf
```
```bash
export PATH="$PREFIX/bin:$PATH"
```

#### step 3: Download and compile the Binutils
**NOTE:** don't forget to check for the lastest versions before downloading.

```bash
# download and extract (version 2.46 for now)
wget https://ftp.gnu.org/gnu/binutils/binutils-2.46.tar.gz
tar -xf binutils-2.46.tar.gz
mkdir build-binutils
cd build-binutils

# Configuration for OS-independent architecture
../binutils-2.46/configure --target=$TARGET --prefix="$PREFIX" --with-sysroot --disable-nls --disable-werror

# compile and install
make -j$(nproc)
make install
cd ..
```

#### step 4: Download and compile the GCC
**NOTE:** don't forget to check for the lastest versions before downloading.

```bash
# download and extract (version 16.1 for now)
wget https://ftp.gnu.org/gnu/gcc/gcc-16.1.0/gcc-16.1.0.tar.gz
tar -xf gcc-16.1.0.tar.gz
mkdir build-gcc
cd build-gcc

# Standalone compiler configuration
../gcc-16.1.0/configure --target=$TARGET --prefix="$PREFIX" --disable-nls --enable-languages=c,c++ --without-headers

# Compile the main part of the compiler and its supporting library (libgcc)
make -j$(nproc) all-gcc
make -j$(nproc) all-target-libgcc

# final installing
make install-gcc
make install-target-libgcc
cd ..
```
### **the end:**
#### after finishing these steps a folder in ```$HOME/opt/cross/bin``` will be created that includes all necessary tools for our makefile. all you need is adding this folder address to your PATH.

**NOTE:** ***compiling GCC will take a lot of time (around 10-20min on a I7 12700 or around 55 min on an old Core 2 T7300) so have a break and just don't look at your pc :)***

#### step 5 (final!): now all you need is a bootloader.

**VostokOS is currently heavily dependent on the Limine bootloader (memory tables, acpi tables, framebuffer, etc.). Therefore, it is recommended that you use only this bootloader.**

- 1- Visit the official Limine page on GitHub at ```github.com/Limine-Bootloader/``` and download the latest binary version in the Release section. (example ```limine-binary.tar.gz```)

- 2- Create a folder called ```limine``` in the root of the project and extract the contents of the downloaded limine bootloader file into this folder.

#### now you can compile project by ```make``` and run it in qemu by ```make run``` or even boot VostokOS on your real hardware/vmware/virtualBox using Ventoy and a USB device!
**NOTE:** ***an iso file called VostokOS.iso will be created in project root folder after compiling***


the TODO list for now, i dont have much good memory :(

    1- ctrl+c
    2- Implement AML parser
    3- Tab completion.
    4- Command history (up/down arrows).
    5- modular shell and commands. (i did it somehow but not fully)
    6- USB
    7- supporting file systems
    8- using panic instead of simple hlt

Good luck!