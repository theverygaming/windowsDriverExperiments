#pragma once
#include <stdint.h>

void printf_serial(const char *fmt, ...);

typedef __builtin_va_list my_va_list;
#define my_va_start(v, l)    __builtin_va_start(v, l)
#define my_va_end(v)         __builtin_va_end(v)
#define my_va_arg(v, l)      __builtin_va_arg(v, l)
#define my_va_copy(dst, src) __builtin_va_copy(dst, src)
