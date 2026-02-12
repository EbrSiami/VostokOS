#include "string.h"

size_t strlen(const char *str) {
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) {
        return 0;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

char *strcpy(char *dest, const char *src) {
    char *original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    char *original_dest = dest;
    while (n && (*dest++ = *src++)) {
        n--;
    }
    while (n--) {
        *dest++ = '\0';
    }
    return original_dest;
}

char *strcat(char *dest, const char *src) {
    char *original_dest = dest;
    while (*dest) {
        dest++;
    }
    while ((*dest++ = *src++));
    return original_dest;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1;
    const unsigned char *p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

// Convert signed integer to string
void itoa(int64_t value, char *str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }
    
    char *ptr = str;
    int64_t tmp_value;
    int negative = 0;
    
    // Handle negative numbers for base 10
    if (value < 0 && base == 10) {
        negative = 1;
        value = -value;
    }
    
    // Process digits in reverse
    tmp_value = value;
    do {
        int remainder = tmp_value % base;
        *ptr++ = (remainder < 10) ? remainder + '0' : remainder - 10 + 'a';
        tmp_value /= base;
    } while (tmp_value);
    
    // Add negative sign
    if (negative) {
        *ptr++ = '-';
    }
    
    *ptr-- = '\0';
    
    // Reverse the string
    char *start = str;
    while (start < ptr) {
        char temp = *start;
        *start++ = *ptr;
        *ptr-- = temp;
    }
}

// Convert unsigned integer to string
void utoa(uint64_t value, char *str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return;
    }
    
    char *ptr = str;
    uint64_t tmp_value;
    
    // Process digits in reverse
    tmp_value = value;
    do {
        int remainder = tmp_value % base;
        *ptr++ = (remainder < 10) ? remainder + '0' : remainder - 10 + 'a';
        tmp_value /= base;
    } while (tmp_value);
    
    *ptr-- = '\0';
    
    // Reverse the string
    char *start = str;
    while (start < ptr) {
        char temp = *start;
        *start++ = *ptr;
        *ptr-- = temp;
    }
}

// Hex conversion helper (your approach is better for this actually!)
void itoa_hex(uint64_t value, char *str, int uppercase) {
    const char *hex_digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    char *ptr = str;
    
    // Handle zero case
    if (value == 0) {
        *ptr++ = '0';
        *ptr = '\0';
        return;
    }
    
    // Find first non-zero digit
    int started = 0;
    for (int i = 15; i >= 0; i--) {
        uint8_t digit = (value >> (i * 4)) & 0xF;
        if (digit != 0 || started) {
            *ptr++ = hex_digits[digit];
            started = 1;
        }
    }
    
    *ptr = '\0';
}