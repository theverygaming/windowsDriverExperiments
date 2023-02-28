#include "printf.h"
#include "serial.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static void myputs(const char *str, bool debugonly) {
    while (*str) {
        serial_putc(*str);
        str++;
    }
}

static char *itoa(size_t value, char *str, size_t base) {
    char *ptr = str;

    do {
        size_t mod = value % base;
        unsigned char start = '0';
        if ((base == 16) && (mod > 9)) {
            start = 'a';
            mod -= 10;
        }
        *ptr++ = start + mod;
    } while ((value /= base) > 0);
    *ptr = '\0';

    size_t len = strlen(str);

    for (int i = 0; i < len / 2; i++) {
        char c = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = c;
    }

    return str;
}

static char *itoa_signed(ssize_t value, char *str, size_t base) {
    bool sign = false;
    if (value < 0) {
        sign = true;
        value = -value;
    }

    char *ptr = str;

    do {
        size_t mod = value % base;
        unsigned char start = '0';
        if ((base == 16) && (mod > 9)) {
            start = 'a';
            mod -= 10;
        }
        *ptr++ = start + mod;
    } while ((value /= base) > 0);
    if (sign) {
        *ptr++ = '-';
    }
    *ptr = '\0';

    size_t len = strlen(str);

    for (int i = 0; i < len / 2; i++) {
        char c = str[i];
        str[i] = str[len - i - 1];
        str[len - i - 1] = c;
    }

    return str;
}

size_t strcspn(const char *s1, const char *s2) {
    size_t n = 0;
    if (*s2 == 0) {
        return 0;
    }
    while (*s1) {
        if (strchr((char *)s2, *s1)) {
            return n;
        }
        s1++;
        n++;
    }
    return n;
}

static int printf_base(my_va_list *args, const char *fmt, char *buf, bool buf_write, size_t buf_len, bool serialonly) {
    size_t chars_written = 0;
    while (*fmt) {
        if (*fmt == '%') {
            fmt++;
            switch (*fmt) {
            case '%': {
                fmt += 1;
                if (buf_write) {
                    size_t can_write = (buf_len - 1) - chars_written;
                    if (chars_written > (buf_len - 1)) {
                        can_write = 0;
                    }
                    if (can_write >= 1) {
                        memcpy(&buf[chars_written], "%", 1);
                    } else {
                        memcpy(&buf[chars_written], "%", can_write);
                    }
                } else {
                    serial_putc('%');
                }
                chars_written += 1;
                break;
            }
            case 's': {
                fmt += 1;
                char *arg = my_va_arg(*args, char *);
                if (buf_write) {
                    size_t can_write = (buf_len - 1) - chars_written;
                    if (chars_written > (buf_len - 1)) {
                        can_write = 0;
                    }
                    if (can_write >= strlen(arg)) {
                        memcpy(&buf[chars_written], arg, strlen(arg));
                    } else {
                        memcpy(&buf[chars_written], arg, can_write);
                    }
                } else {
                    myputs(arg, serialonly);
                }
                chars_written += strlen(arg);
                break;
            }
            case 'u': {
                fmt += 1;
                size_t arg = my_va_arg(*args, size_t);
                char n_buf[11];
                char *ret = itoa(arg, n_buf, 10);
                if (buf_write) {
                    size_t can_write = (buf_len - 1) - chars_written;
                    if (chars_written > (buf_len - 1)) {
                        can_write = 0;
                    }
                    if (can_write >= strlen(ret)) {
                        memcpy(&buf[chars_written], ret, strlen(ret));
                    } else {
                        memcpy(&buf[chars_written], ret, can_write);
                    }
                } else {
                    myputs(ret, serialonly);
                }
                chars_written += strlen(ret);
                break;
            }
            case 'p': {
                fmt += 1;
                uintptr_t arg = my_va_arg(*args, uintptr_t);
                char n_buf[17];
                char *ret = itoa(arg, n_buf, 16);
                if (buf_write) {
                    size_t can_write = (buf_len - 1) - chars_written;
                    if (chars_written > (buf_len - 1)) {
                        can_write = 0;
                    }
                    if (can_write >= strlen(ret)) {
                        memcpy(&buf[chars_written], ret, strlen(ret));
                    } else {
                        memcpy(&buf[chars_written], ret, can_write);
                    }
                } else {
                    myputs(ret, serialonly);
                }
                chars_written += strlen(ret);
                break;
            }
            case 'd': {
                fmt += 1;
                size_t arg = my_va_arg(*args, size_t);
                char n_buf[12];
                char *ret = itoa_signed(arg, n_buf, 10);
                if (buf_write) {
                    size_t can_write = (buf_len - 1) - chars_written;
                    if (chars_written > (buf_len - 1)) {
                        can_write = 0;
                    }
                    if (can_write >= strlen(ret)) {
                        memcpy(&buf[chars_written], ret, strlen(ret));
                    } else {
                        memcpy(&buf[chars_written], ret, can_write);
                    }
                } else {
                    myputs(ret, serialonly);
                }
                chars_written += strlen(ret);
                break;
            }
            case 'c': {
                fmt += 1;
                char arg = my_va_arg(*args, int);
                if (buf_write) {
                    size_t can_write = (buf_len - 1) - chars_written;
                    if (chars_written > (buf_len - 1)) {
                        can_write = 0;
                    }
                    if (can_write >= 1) {
                        memcpy(&buf[chars_written], &arg, 1);
                    } else {
                        memcpy(&buf[chars_written], &arg, can_write);
                    }
                } else {
                    serial_putc(arg);
                }
                chars_written++;
                break;
            }
            default:
                printf_serial("printf: unsupported %%%c\n", *fmt);
                break;
            }
        }
        size_t count = strcspn(fmt, "%");
        if (buf_write) {
            size_t can_write = (buf_len - 1) - chars_written;
            if (chars_written > (buf_len - 1)) {
                can_write = 0;
            }
            if (can_write >= count) {
                memcpy(&buf[chars_written], fmt, count);
            } else {
                memcpy(&buf[chars_written], fmt, can_write);
            }
        } else {
            for (int i = 0; i < count; i++) {
                serial_putc(fmt[i]);
            }
        }
        chars_written += count;
        fmt += count;
    }
    if (buf_write) {
        if (chars_written < (buf_len - 1)) {
            buf[chars_written] = '\0';
        } else {
            buf[buf_len - 1] = '\0';
        }
    }

    return chars_written;
}

void printf_serial(const char *fmt, ...) {
    my_va_list args;
    my_va_start(args, fmt);
    printf_base(&args, fmt, NULL, false, 0, true);
    my_va_end(args);
}
