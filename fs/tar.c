#include "tar.h"
#include "vfs.h"
#include "../lib/string.h"
#include "../lib/printk.h"

// Standard USTAR header
typedef struct {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];    // Octal string
    char mtime[12];
    char chksum[8];
    char typeflag;    // '0' or '\0' = file, '5' = dir
    char linkname[100];
    char magic[6];    // "ustar"
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} __attribute__((packed)) tar_header_t;

// Helper to convert octal ASCII to integer
static uint64_t octal_to_int(const char* str, int size) {
    uint64_t n = 0;
    for (int i = 0; i < size; i++) {
        if (str[i] >= '0' && str[i] <= '7') {
            n = n * 8 + (str[i] - '0');
        }
    }
    return n;
}

void tar_init(void* archive_address) {
    uint8_t* current = (uint8_t*)archive_address;
    
    printk("[TAR] Parsing archive at %p...\n", archive_address);

    while (1) {
        tar_header_t* header = (tar_header_t*)current;
        
        // If the name is empty, we reached the end of the archive
        if (header->name[0] == '\0') {
            break;
        }

        // Parse file size
        uint64_t size = octal_to_int(header->size, 11);
        bool is_dir = (header->typeflag == '5');

        // File data starts immediately after the 512-byte header
        uint8_t* data = current + 512;

        // Register it in our VFS
        vfs_register_node(header->name, size, is_dir, data);

        // Move to the next file. 
        // Tar pads file data to 512 byte boundaries.
        uint64_t padded_size = (size + 511) & ~511;
        current += 512 + padded_size;
    }
    
    printk("[TAR] Parsing complete.\n");
}