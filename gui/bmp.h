#ifndef BMP_H
#define BMP_H

#include <stdint.h>
#include <stddef.h>

#pragma pack(push, 1)

typedef struct {
    uint16_t signature;
    uint32_t file_size;
    uint32_t reserved;
    uint32_t data_offset; // Where the pixel array starts
} bmp_file_header_t;

typedef struct {
    uint32_t size;        // Size of this header
    int32_t  width;
    int32_t  height;
    uint16_t planes;
    uint16_t bpp;         // Bits per pixel
    uint32_t compression;
    uint32_t image_size;
    int32_t  x_pixels_per_m;
    int32_t  y_pixels_per_m;
    uint32_t colors_used;
    uint32_t colors_important;
} bmp_info_header_t;

#pragma pack(pop)

// Draw a BMP image directly to the framebuffer
// data: pointer to the raw file data
// x, y: coordinates on the screen
void bmp_draw(uint8_t* data, int screen_x, int screen_y);

#endif