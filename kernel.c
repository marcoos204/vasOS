#include "kernel.h"
#include "common.h"

extern char __bss[], __bss_end[], __stack_top[]; //we need to resolve as [] so that the variables turn into pointers
                                                  //same as using extern char __bss... and then passing &__bss to memset.
                                                  //otherwise it could treat __bss's value as the pointer, and if it's 0 then it could wipe out 0x00000000, crashing



struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3, long arg4, long arg5, long fid, long eid){
   
    //register keyword is used to specify compiler "this variable can be put in a register", with __asm__("register") indicating the register
    register long a0 __asm__("a0") = arg0; 
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid; //SBI function ID for any given eid in a7
    register long a7 __asm__("a7") = eid; //SBI extension ID
    
    //we manually execute the ecall with inline assembly, setting a0 and a1 as output registers
    __asm__ __volatile("ecall"
                        : "=r"(a0), "=r"(a1)
                        : "r"(a0),"r"(a1), "r"(a2), "r"(a3), "r"(a4), 
                          "r"(a5), "r"(a6), "r"(a7)
                        : "memory");

    return (struct sbiret){.error = a0, .value = a1};
}


//sbi_console_putchar is the extension id 0x01. with the next ecall, we invoke it

void putchar(char ch){ //basically telling the SBI runtime service to put this character in the console (uart) for us
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1 /* Console Putchar*/);
}


//some bootloaders may zero-clear bss; we do it just in case
void kernel_main(void) {
    
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss); //initializes bss region to 0

    printf("\n\n Hello %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    for (;;){

        __asm__ __volatile__("wfi");
    }
}
__attribute__((section(".text.boot"))) //place boot funct exactly at 0x80200000
__attribute__((naked)) //do not insert code before and after the function body. Otherwise, it could potentially modify sp
void boot(void) {

    __asm__ __volatile__(
        "mv sp, %[stack_top]\n"
        "j kernel_main\n"
        :
        : [stack_top] "r" (__stack_top)
    );
}