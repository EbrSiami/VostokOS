#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

// String functions
size_t strlen(const char *str);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char *strcat(char *dest, const char *src);

// Memory functions
void *memset(void *s, int c, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

// Number to string conversions
void itoa(int64_t value, char *str, int base);
void utoa(uint64_t value, char *str, int base);
void itoa_hex(uint64_t value, char *str, int uppercase);

#endif