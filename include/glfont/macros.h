#ifndef GLFONT_MACROS_H
#define GLFONT_MACROS_H
#include <stdlib.h>
#include <string.h>
#define NEW(T) (T *)calloc(1, sizeof(T))
#define OR(a, b) (a ? a : b)
#endif
