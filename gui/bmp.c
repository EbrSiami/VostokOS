#include "bmp.h"
#include "../display/framebuffer.h"
#include "../lib/printk.h"

void bmp_draw(uint8_t* data, int screen_x, int screen_y) {
    bmp_file_header_t* file_header = (bmp_file_header_t*)data;
    
    // Check signature 'B' (0x42) and 'M' (0x4D)
    if (file_header->signature != 0x4D42) {
        printk("[GUI] Error: Not a valid BMP file.\n");
        return;
    }

    bmp_info_header_t* info = (bmp_info_header_t*)(data + sizeof(bmp_file_header_t));

    if (info->compression != 0) {
        printk("[GUI] Error: Compressed BMPs not supported.\n");
        return;
    }

    if (info->bpp != 24 && info->bpp != 32) {
        printk("[GUI] Error: Only 24-bit and 32-bit BMPs are supported.\n");
        return;
    }

    int width = info->width;
    int height = info->height;
    int is_bottom_up = 1;
    if (height < 0) {
        is_bottom_up = 0;
        height = -height;
    }

    framebuffer_t *fb = fb_get();
    uint32_t *screen_buffer = (uint32_t *)fb->address;
    int pitch_words = fb->pitch / 4;

    uint8_t* pixel_array = data + file_header->data_offset;
    int bytes_per_pixel = info->bpp / 8;
    int row_stride = (width * bytes_per_pixel + 3) & ~3;

    for (int y = 0; y < height; y++) {
        int file_y = is_bottom_up ? (height - 1 - y) : y;
        uint8_t* row_ptr = pixel_array + (file_y * row_stride);

        for (int x = 0; x < width; x++) {
            int pixel_offset = x * bytes_per_pixel;
            
            uint8_t b = row_ptr[pixel_offset];
            uint8_t g = row_ptr[pixel_offset + 1];
            uint8_t r = row_ptr[pixel_offset + 2];
            uint32_t color = (r << 16) | (g << 8) | b;
            
            int draw_x = screen_x + x;
            int draw_y = screen_y + y;

            if (draw_x >= 0 && (size_t)draw_x < fb->width && draw_y >= 0 && (size_t)draw_y < fb->height) {
                screen_buffer[(draw_y * pitch_words) + draw_x] = color;
            }
        }
    }
}