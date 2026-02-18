#include "bitmap.h"

void bitmap_set(uint8_t *bitmap, uint64_t index) {
    bitmap[index / 8] |= (1 << (index % 8));
}

void bitmap_clear(uint8_t *bitmap, uint64_t index) {
    bitmap[index / 8] &= ~(1 << (index % 8));
}

bool bitmap_test(uint8_t *bitmap, uint64_t index) {
    return (bitmap[index / 8] & (1 << (index % 8))) > 0;
}