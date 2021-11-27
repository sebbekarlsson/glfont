#ifndef GLFONT_STRING_UTILS_H
#define GLFONT_STRING_UTILS_H
#include <stdint.h>
char** glfont_str_chunk(const char* src, uint32_t chunk_length, uint32_t* len);
uint32_t glfont_str_count_alpha(const char* src);
uint32_t glfont_str_next_break(const char* src, uint32_t start);
#endif
