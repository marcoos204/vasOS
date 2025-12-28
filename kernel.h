#pragma once
#include "common.h"
//here, PANIC is called as a macro so that it can correctly display the source file and the line number. 
//if it was a function, it would display the file name and the number where PANIC is defined and not where it's called
//other thing to note is that in C the compiler will look at the different strings arguments and will concatenate all of them into only one fmt argument input.
// ##__VA_ARGS__ works as it automatically passes all varidic arguments to the function, and if left empty it deletes the preceding coma
#define PANIC(fmt, ...)                                                                     \
    do{                                                                                     \
        printf("PANIC: %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);                \
        while(1) {}                                                                         \
    } while (0)


struct sbiret {
    long error;
    long value;
};