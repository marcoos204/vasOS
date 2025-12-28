#include "common.h"

void putchar(char ch);

void printf(const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);

    while(*fmt) {
        if (*fmt == '%'){
            fmt++; //skips %
            switch(*fmt) {
                case '\0': //% at the end of the format string
                    putchar('%');
                    goto end;
                case '%': //print '%'
                    putchar('%');
                    break;
                case 's': { //prints NULL-terminated string
                    const char *s = va_arg(vargs, const char *);
                    while (*s) {
                        putchar(*s);
                        s++;
                    }
                    break;
                }
                case 'd': { //prints integer in decimal
                    int value = va_arg(vargs, int);
                    unsigned magnitude = value;
                    if (value < 0) {
                        putchar('-');
                        magnitude = -magnitude; //absolute value
                    }

                    unsigned divisor = 1;
                    while (magnitude / divisor > 9)
                        divisor *= 10;

                    while (divisor > 0) {
                        putchar('0' + magnitude / divisor);
                        magnitude %= divisor;
                        divisor /= 10;
                    }
                    break;
                }

                case 'x': {
                    unsigned value = va_arg(vargs, unsigned);
                    for (int i = 7; i >= 0; i--) {
                        unsigned nibble = (value >> (i * 4)) & 0xf; //shifts 4 bits to the lsb and applies mask
                        putchar("0123456789abcdef"[nibble]); //converts the value to hex. 
                    }
                    break;
                }

            }
        } else {
            putchar(*fmt); //just puts the actual value of the current string (one char)
        }

        fmt++;


    }
end:
    va_end(vargs);
}

//memory operations
void *memset(void *buf, char c, size_t n) {
   uint8_t *p = (uint8_t *) buf; //casts buf addr to 8bit, needed to define the data size to write in buf
   while (n--)
       *p++ = c; //assigns byte of mem to c value, its doing first the dereference and then the p advancment
   return buf;
}

void *memcpy(void *dst, const void *src, size_t n){
    uint8_t *d = (uint8_t *) dst;
    const uint8_t *s = (const uint8_t *) src;
    while (n--)
        *d++ = *s++;
    return dst;
}

//string operations

char *strcpy(char *dst, const char *src) { //WARNING: strcpy continues copying even if size of src is larger than the memory area of dst. Recommended to use an alternative function like strcopy_s
    char *d = dst;
    while (*src)
        *d++ = *src++;
    *d = '\0';
    return dst;
}

int strcmp(const char *s1, const char *s2){
    
    while ((*s2) && (*s1)){
        if (*s1 != *s2){
            break;
        }
        s1++;
        s2++;
    }

    return *(unsigned char *)s1 - *(unsigned char *)s2; //POSIX specification

}