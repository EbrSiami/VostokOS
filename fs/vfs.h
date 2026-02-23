#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MAX_FILENAME 128

// Represents a file in our system
typedef struct vfs_node {
    char name[MAX_FILENAME];
    uint64_t size;
    bool is_dir;
    uint8_t* data; // Pointer to the file's data in RAM
    struct vfs_node* next;
} vfs_node_t;

// Initialize the VFS
void vfs_init(void);

// Add a file to the VFS registry
void vfs_register_node(const char* name, uint64_t size, bool is_dir, uint8_t* data);

// Find a file
vfs_node_t* vfs_open(const char* filename);

// Read from a file
size_t vfs_read(vfs_node_t* node, void* buffer, size_t size, size_t offset);

// Get the root of the file list (for 'ls' command)
vfs_node_t* vfs_get_root(void);

#endif