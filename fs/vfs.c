#include "vfs.h"
#include "../mm/heap.h"
#include "../lib/string.h"
#include "../lib/printk.h"

static vfs_node_t* vfs_root = NULL;

void vfs_init(void) {
    vfs_root = NULL;
    printk("[VFS] Initialized.\n");
}

void vfs_register_node(const char* name, uint64_t size, bool is_dir, uint8_t* data) {
    vfs_node_t* node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    
    strncpy(node->name, name, MAX_FILENAME - 1);
    node->name[MAX_FILENAME - 1] = '\0';
    
    node->size = size;
    node->is_dir = is_dir;
    node->data = data;
    
    // Add to front of linked list
    node->next = vfs_root;
    vfs_root = node;
    
    printk("[VFS] Registered: /%s (%llu bytes)\n", node->name, node->size);
}

vfs_node_t* vfs_open(const char* filename) {
    vfs_node_t* current = vfs_root;
    while (current != NULL) {
        if (strcmp(current->name, filename) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL; // File not found
}

size_t vfs_read(vfs_node_t* node, void* buffer, size_t size, size_t offset) {
    if (!node || node->is_dir || offset >= node->size) return 0;
    
    size_t available = node->size - offset;
    size_t to_read = (size < available) ? size : available;
    
    memcpy(buffer, node->data + offset, to_read);
    
    return to_read;
}

vfs_node_t* vfs_get_root(void) {
    return vfs_root;
}