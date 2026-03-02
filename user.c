#include "user.h"

extern char __stack_top[];

//we don't need to zero clear the bss with xeros. The kernel already guarantees that it's already filled in the alloc_pages function
__attribute__((section(".text.start")))
__attribute__((naked))
void start(void) {
    __asm__ __volatile__(
        "mv sp, %[stack_top] \n"
        "call main           \n"
        "call exit           \n"
        :: [stack_top] "r" (__stack_top) //variable stack top es un registro con el valor de la direccion de stack top
    );
}

int syscall(int sysno, int arg0, int arg1, int arg2) {
    register int a0 __asm__("a0") = arg0;
    register int a1 __asm__("a1") = arg1;
    register int a2 __asm__("a2") = arg2;
    register int a3 __asm__("a3") = sysno; //syscall number

    __asm__ __volatile__("ecall" //when ecall is executed, the exception handler is called and control is given to the kernel
                         : "=r"(a0) //return value from the kernel set here
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3)
                         : "memory");
    return a0;
}

void putchar(char ch) {
    syscall(SYS_PUTCHAR, ch, 0, 0);
}

int getchar(void) {
    return syscall(SYS_GETCHAR, 0, 0, 0);
}

__attribute__((noreturn)) void exit(void) {
    syscall(SYS_EXIT, 0, 0, 0);
    for(;;);
}