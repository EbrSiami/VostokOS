#include "keyboard.h"
#include "../lib/printk.h"
#include "../display/terminal.h"
#include "../shell/shell.h"

// Helper to read from I/O port
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    __asm__ volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

// Helper to write to I/O port
static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Keyboard state
static keyboard_state_t kb_state = {
    .shift_pressed = false,
    .ctrl_pressed = false,
    .alt_pressed = false,
    .caps_lock = false,
    .num_lock = false,
    .scroll_lock = false
};

// US QWERTY scancode to ASCII mapping (set 1)
static const char scancode_to_ascii[] = {
    0,    0,   '1',  '2',  '3',  '4',  '5',  '6',   // 0x00-0x07
    '7',  '8', '9',  '0',  '-',  '=',  '\b', '\t',  // 0x08-0x0F
    'q',  'w', 'e',  'r',  't',  'y',  'u',  'i',   // 0x10-0x17
    'o',  'p', '[',  ']',  '\n', 0,    'a',  's',   // 0x18-0x1F
    'd',  'f', 'g',  'h',  'j',  'k',  'l',  ';',   // 0x20-0x27
    '\'', '`', 0,    '\\', 'z',  'x',  'c',  'v',   // 0x28-0x2F
    'b',  'n', 'm',  ',',  '.',  '/',  0,    '*',   // 0x30-0x37
    0,    ' ', 0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,   0,    0,    0,    0,    0,    '7',   // 0x40-0x47
    '8',  '9', '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3', '0',  '.',  0,    0,    0,    0,     // 0x50-0x57
    0,    0                                          // 0x58-0x59
};

// Shifted characters
static const char scancode_to_ascii_shift[] = {
    0,    0,   '!',  '@',  '#',  '$',  '%',  '^',   // 0x00-0x07
    '&',  '*', '(',  ')',  '_',  '+',  '\b', '\t',  // 0x08-0x0F
    'Q',  'W', 'E',  'R',  'T',  'Y',  'U',  'I',   // 0x10-0x17
    'O',  'P', '{',  '}',  '\n', 0,    'A',  'S',   // 0x18-0x1F
    'D',  'F', 'G',  'H',  'J',  'K',  'L',  ':',   // 0x20-0x27
    '"',  '~', 0,    '|',  'Z',  'X',  'C',  'V',   // 0x28-0x2F
    'B',  'N', 'M',  '<',  '>',  '?',  0,    '*',   // 0x30-0x37
    0,    ' ', 0,    0,    0,    0,    0,    0,     // 0x38-0x3F
    0,    0,   0,    0,    0,    0,    0,    '7',   // 0x40-0x47
    '8',  '9', '-',  '4',  '5',  '6',  '+',  '1',   // 0x48-0x4F
    '2',  '3', '0',  '.',  0,    0,    0,    0,     // 0x50-0x57
    0,    0                                          // 0x58-0x59
};

void keyboard_init(void) {
    // Clear keyboard state
    kb_state.shift_pressed = false;
    kb_state.ctrl_pressed = false;
    kb_state.alt_pressed = false;
    kb_state.caps_lock = false;
    kb_state.num_lock = false;
    kb_state.scroll_lock = false;
    
    printk("[KEYBOARD] PS/2 keyboard driver initialized\n");
}

keyboard_state_t* keyboard_get_state(void) {
    return &kb_state;
}

// Set keyboard LEDs
void keyboard_set_leds(void) {
    uint8_t led_state = 0;
    
    if (kb_state.scroll_lock) led_state |= 0x01;
    if (kb_state.num_lock)    led_state |= 0x02;
    if (kb_state.caps_lock)   led_state |= 0x04;
    
    // Send LED command
    outb(KEYBOARD_DATA_PORT, 0xED);
    
    // Wait for acknowledgment
    for (volatile int i = 0; i < 10000; i++);
    
    // Send LED state
    outb(KEYBOARD_DATA_PORT, led_state);
    
    // Wait for acknowledgment
    for (volatile int i = 0; i < 10000; i++);
}

void keyboard_handler(void) {
    // Check if data is actually available
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if ((status & 0x01) == 0) {
        return;  // No data available
    }
    
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Check if this is a key release (break code)
    bool key_released = (scancode & 0x80) != 0;
    scancode &= 0x7F;  // Remove the release bit
    
    // Handle modifier keys
    if (scancode == KEY_LSHIFT || scancode == KEY_RSHIFT) {
        kb_state.shift_pressed = !key_released;
        return;
    }
    
    if (scancode == KEY_CTRL) {
        kb_state.ctrl_pressed = !key_released;
        return;
    }
    
    if (scancode == KEY_ALT) {
        kb_state.alt_pressed = !key_released;
        return;
    }
    
    // Handle toggle keys (only on press, not release)
    if (!key_released) {
        if (scancode == KEY_CAPSLOCK) {
            kb_state.caps_lock = !kb_state.caps_lock;
            keyboard_set_leds();  // Update LEDs
            return;
        }
        
        if (scancode == KEY_NUMLOCK) {
            kb_state.num_lock = !kb_state.num_lock;
            keyboard_set_leds();  // Update LEDs
            return;
        }
        
        if (scancode == KEY_SCROLLLOCK) {
            kb_state.scroll_lock = !kb_state.scroll_lock;
            keyboard_set_leds();  // Update LEDs
            return;
        }
    }
    
    // Only process key presses (not releases) for printable characters
    if (key_released) {
        return;
    }
    
    // Convert scancode to ASCII
    char ascii = 0;
    
    if (scancode < sizeof(scancode_to_ascii)) {
        if (kb_state.shift_pressed) {
            ascii = scancode_to_ascii_shift[scancode];
        } else {
            ascii = scancode_to_ascii[scancode];
        }
        
        // Handle caps lock for letters
        if (kb_state.caps_lock && ascii >= 'a' && ascii <= 'z') {
            ascii -= 32;  // Convert to uppercase
        } else if (kb_state.caps_lock && ascii >= 'A' && ascii <= 'Z' && kb_state.shift_pressed) {
            ascii += 32;  // Convert to lowercase (shift inverts caps lock)
        }
    }
    
    // Print the character and pass to shell
    if (ascii == '\b') {
        shell_process_char(ascii);
    } else if (ascii == '\n') {
        shell_process_char(ascii);
    } else if (ascii == '\t') {
        // Handle tab separately - shell doesn't need it for now
        terminal_write("    ");
    } else if (ascii != 0) {
        shell_process_char(ascii);
    }
}