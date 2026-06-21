#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    int32_t x;
    int32_t y;
    bool left_click;
    bool right_click;
    bool middle_click;
} mouse_state_t;

void mouse_init(void);
void mouse_handler(void);
mouse_state_t* mouse_get_state(void);

uint64_t mouse_irq_handler(uint64_t current_rsp);

#endif