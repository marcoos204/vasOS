#pragma once
#include "common.h"
//here, PANIC is called as a macro so that it can correctly display the source file and the line number. 
//if it was a function, it would display the file name and the number where PANIC is defined and not where it's called. 
//other thing to note is that in C the compiler will look at the different strings arguments and will concatenate all of them into only one fmt argument input.
// ##__VA_ARGS__ works as it automatically passes all varidic arguments to the function, and if left empty it deletes the preceding coma.
#define ART    \
"       c~~p ,---------. \n"   \
"  ,---'oo  )           \\ \n"   \
" ( O O                  )/\n"    \
"  `=^='                 /\n"     \
"       \\    ,     .   / \n"    \
"        \\\\  |-----'|  /  \n"      \
"        ||__|    |_|__|\n" \

//its fun toggling with the ascii art. cool application of escapping with backslashes (which basically escape or 'eat' the \n symbol present at the end of each line)
#define PANIC(fmt, ...)                                                                     \
    do{                                                                                    \
        printf("PANIC: %s:%d " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__);                \
        printf(ART);\
        printf("sorry for that buddy. Let me know if there's anything I can help with"); \
        while(1) {}                                                                         \
    } while (0)


struct sbiret {
    long error;
    long value;
};

struct trap_frame {
    uint32_t ra;
    uint32_t gp;
    uint32_t tp;
    uint32_t t0;
    uint32_t t1;
    uint32_t t2;
    uint32_t t3;
    uint32_t t4;
    uint32_t t5;
    uint32_t t6;
    uint32_t a0;
    uint32_t a1;
    uint32_t a2;
    uint32_t a3;
    uint32_t a4;
    uint32_t a5;
    uint32_t a6;
    uint32_t a7;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t s8;
    uint32_t s9;
    uint32_t s10;
    uint32_t s11;
    uint32_t sp;

} __attribute__((packed)); //don't add padding nor optimizations

//next macro utilizes a GNU compiler extension: Statement expression
//it allows to define a C code block ({}) and the last line is the return value
//used in read_csr because it needs to return a value

//here its passing the read value from the csr into the variable __tmp ("=r"(tmp))
#define READ_CSR(reg)                                       \
({                                                          \
    unsigned long __tmp;                                    \
    __asm__ __volatile__("csrr %0, " #reg : "=r"(__tmp));   \
    __tmp;                                                  \
})                                                         

//here its writing the value (tmp) into the csr
#define WRITE_CSR(reg, value)                               \
  do{                                                       \
    uint32_t __tmp = (value);                               \
    __asm__ __volatile__("csrw " #reg ", %0" ::"r"(__tmp)); \
}while(0)

