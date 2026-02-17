#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#define FONT_WIDTH 8
#define FONT_HEIGHT 8

#define FONT_CHARS 256
extern const uint8_t font_data[FONT_CHARS][8];

#endif