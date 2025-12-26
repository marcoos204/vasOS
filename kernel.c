typedef unsigned char uint8_t;
typedef unsigned int uint32_t;
typedef uint32_t size_t;

extern char __bss[], __bss_end[], __stack_top[]; //we need to resolve as [] so that the variables turn into pointers
                                                  //same as using extern char __bss... and then passing &__bss to memset.
                                                  //otherwise it could treat __bss's value as the pointer, and if it's 0 then it could wipe out 0x00000000, crashing

void *memset(void *buf, char c, size_t n) {
   uint8_t *p = (uint8_t *) buf; //casts buf addr to 8bit, needed to define the data size to write in buf
   while (n--)
       *p++ = c; //assigns byte of mem to c value
   return buf;
}

//some bootloaders may zero-clear bss; we do it just in case
void kernel_main(void) {
    
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss); //initializes bss region to 0
    for (;;); 
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