#ifndef BITMAP_H
#define BITMAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void bitmap_set(uint8_t *bitmap, uint64_t index);
void bitmap_clear(uint8_t *bitmap, uint64_t index);
bool bitmap_test(uint8_t *bitmap, uint64_t index);

#endif