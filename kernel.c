#include "kernel.h"
#include "common.h"

struct process procs[PROCS_MAX]; //'process' struct array

extern char __bss[], __bss_end[], __stack_top[]; //we need to resolve as [] so that the variables turn into pointers
                                                  //same as using extern char __bss... and then passing &__bss to memset.
                                                  //otherwise it could treat __bss's value as the pointer, and if it's 0 then it could wipe out 0x00000000, crashing
extern char _binary_shell_bin_start[], _binary_shell_bin_size[];


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

//reads input to the debug console. returns -1 if there's no input
long getchar(void) {
    struct sbiret ret = sbi_call(0, 0, 0, 0, 0, 0, 0, 2);
    return ret.error;
}

__attribute__((naked)) 
void switch_context(uint32_t *prev_sp, uint32_t *next_sp){

    __asm__ __volatile__(
        //save calle-saved registers onto the current process's stack. All of this is defined by riscv's calling convenction
        "addi sp, sp, -13*4\n"
        "sw ra,  0  * 4(sp)\n"
        "sw s0,  1  * 4(sp)\n"
        "sw s1,  2  * 4(sp)\n"
        "sw s2,  3  * 4(sp)\n"
        "sw s3,  4  * 4(sp)\n"
        "sw s4,  5  * 4(sp)\n"
        "sw s5,  6  * 4(sp)\n"
        "sw s6,  7  * 4(sp)\n"
        "sw s7,  8  * 4(sp)\n"
        "sw s8,  9  * 4(sp)\n"
        "sw s9,  10 * 4(sp)\n"
        "sw s10, 11 * 4(sp)\n"
        "sw s11, 12 * 4(sp)\n"

        //switch stack pointer
        "sw sp, (a0)\n" // *prev_sp = sp;
        "lw sp, (a1)\n" // switches stack pointer to next stack

        // Restore callee-saved registers from the next process's stack.
        "lw ra,  0  * 4(sp)\n"  // Restore callee-saved registers only
        "lw s0,  1  * 4(sp)\n"
        "lw s1,  2  * 4(sp)\n"
        "lw s2,  3  * 4(sp)\n"
        "lw s3,  4  * 4(sp)\n"
        "lw s4,  5  * 4(sp)\n"
        "lw s5,  6  * 4(sp)\n"
        "lw s6,  7  * 4(sp)\n"
        "lw s7,  8  * 4(sp)\n"
        "lw s8,  9  * 4(sp)\n"
        "lw s9,  10 * 4(sp)\n"
        "lw s10, 11 * 4(sp)\n"
        "lw s11, 12 * 4(sp)\n"
        "addi sp, sp, 13 * 4\n"  // We've popped 13 4-byte registers from the stack
        "ret\n"
    );
}

struct process *current_proc;
struct process *idle_proc;

void yield(void){
    //runnable process search
    struct process *next = idle_proc;
    for (int i = 0; i < PROCS_MAX; i++) {
        struct process *proc = &procs[(current_proc->pid + i) % PROCS_MAX];
        if (proc->state == PROC_RUNNABLE && proc->pid > 0) {
            next = proc;
            break;
        }
    }

    //No runnable processes?

    if (next == current_proc)
        return;
    
    //saves in sscratch a pointer to the base of the kernel stack
    //now also switches table page when switching context
    __asm__ __volatile__(
        "sfence.vma\n" //similar to a memory fence, ensures changes to page tables are completed and clears cache of page_table entreis. Supervisor Memory-management fence. Guarantees that all previous writes to the table are done.
        "csrw satp, %[satp]\n"                                                                                                                                                
        "sfence.vma\n" //invalidates tlb
        "csrw sscratch, %[sscratch]\n"
        :
        :[satp] "r" (SATP_SV32 | ((uint32_t) next->page_table / PAGE_SIZE)), //sets up SATP bit 31 as 1, paging enabled. Then, divides ny PAGE_SIZE,obtaining the PPU and placing the bits on 0..19
         [sscratch] "r" ((uint32_t) &next->stack[sizeof(next->stack)])         //on boot, STAP is normally set to 0 so all virtual addresses act as physical.
    );
    struct process *prev = current_proc;
    current_proc = next;

    switch_context(&prev->sp, &next->sp);
}

void handle_syscall(struct trap_frame *f) {
    switch (f->a3) {
        case SYS_EXIT:
            printf("process %d exited\n", current_proc->pid);
            current_proc->state = PROC_EXITED; //for simplicity, we only mark the process as exited.
            yield();                           //on a practical OS, you'll need to free resources held by the process such as
            PANIC("unreachable");              //page tables and allocated mem regions
        case SYS_GETCHAR:
            while (1) {
                long ch = getchar();
                if (ch >= 0) {
                    f->a0 = ch;
                    break;
                }

                yield(); //yields cpu to other processes
            }
            break;
        case SYS_PUTCHAR:
            putchar(f->a0);
            break;
        default:
            PANIC("woah unexpected syscall! a3=%x\n", f->a3);
    }
}


void handle_trap(struct trap_frame *f){
    uint32_t scause = READ_CSR(scause);
    uint32_t stval = READ_CSR(stval);
    uint32_t user_pc = READ_CSR(sepc);

    if (scause == SCAUSE_ECALL) {
        handle_syscall(f);
        user_pc += 4; //user_pc points to the ecall. We must make it advance so it doesn't loop forever
    } else {
            PANIC("MOUSE CAUGHT! (unexpected trap) scause=%x, stval=%x, sepc=%x\n", scause, stval, user_pc);
    }

    WRITE_CSR(sepc, user_pc);

}
__attribute__((naked))
__attribute__((aligned(4))) //last 2 bytes of the address on the stvec register define the mode, so they must be 4 byte aligned (so that the address ends in 00, if not, the cpu would ignore the last 2 bits and jump to an erroneous address)
void kernel_entry(void) {   //defines what the kernel shall do when the CPU generates an exception, in other words, this is the exception entry point
    __asm__ __volatile__(
        "csrrw sp, sscratch, sp\n"    //swap operation, result: sp points to kernel stack of curr process, sscratch holds value of sp (user stack) at exception time    
        "addi sp, sp, -4 * 31\n"
        "sw ra, 4 * 0(sp)\n"
        "sw gp, 4 * 1(sp)\n"
        "sw tp, 4 * 2(sp)\n"
        "sw t0, 4 * 3(sp)\n"
        "sw t1, 4 * 4(sp)\n"
        "sw t2, 4 * 5(sp)\n"
        "sw t3, 4 * 6(sp)\n"
        "sw t4, 4 * 7(sp)\n"
        "sw t5, 4 * 8(sp)\n"
        "sw t6, 4 * 9(sp)\n"
        "sw a0, 4 * 10(sp)\n"
        "sw a1, 4 * 11(sp)\n"
        "sw a2, 4 * 12(sp)\n"
        "sw a3, 4 * 13(sp)\n"
        "sw a4, 4 * 14(sp)\n"
        "sw a5, 4 * 15(sp)\n"
        "sw a6, 4 * 16(sp)\n"
        "sw a7, 4 * 17(sp)\n"
        "sw s0, 4 * 18(sp)\n"
        "sw s1, 4 * 19(sp)\n"
        "sw s2, 4 * 20(sp)\n"
        "sw s3, 4 * 21(sp)\n"
        "sw s4, 4 * 22(sp)\n"
        "sw s5, 4 * 23(sp)\n"
        "sw s6, 4 * 24(sp)\n"
        "sw s7, 4 * 25(sp)\n"
        "sw s8, 4 * 26(sp)\n"
        "sw s9, 4 * 27(sp)\n"
        "sw s10, 4 * 28(sp)\n"
        "sw s11, 4 * 29(sp)\n"
        
        // Retrieve and save the sp at the time of exception.
        "csrr a0, sscratch\n"
        "sw a0, 4 * 30(sp)\n"
        
        //reset kernel stack
        "addi a0, sp, 4 * 31\n"
        "csrw sscratch, a0\n"

        "mv a0, sp\n"
        "call handle_trap\n"
        
        "lw ra, 4 * 0(sp)\n"
        "lw gp, 4 * 1(sp)\n"
        "lw tp, 4 * 2(sp)\n"
        "lw t0, 4 * 3(sp)\n"
        "lw t1, 4 * 4(sp)\n"
        "lw t2, 4 * 5(sp)\n"
        "lw t3, 4 * 6(sp)\n"
        "lw t4, 4 * 7(sp)\n"
        "lw t5, 4 * 8(sp)\n"
        "lw t6, 4 * 9(sp)\n"
        "lw a0, 4 * 10(sp)\n"
        "lw a1, 4 * 11(sp)\n"
        "lw a2, 4 * 12(sp)\n"
        "lw a3, 4 * 13(sp)\n"
        "lw a4, 4 * 14(sp)\n"
        "lw a5, 4 * 15(sp)\n"
        "lw a6, 4 * 16(sp)\n"
        "lw a7, 4 * 17(sp)\n"
        "lw s0, 4 * 18(sp)\n"
        "lw s1, 4 * 19(sp)\n"
        "lw s2, 4 * 20(sp)\n"
        "lw s3, 4 * 21(sp)\n"
        "lw s4, 4 * 22(sp)\n"
        "lw s5, 4 * 23(sp)\n"
        "lw s6, 4 * 24(sp)\n"
        "lw s7, 4 * 25(sp)\n"
        "lw s8, 4 * 26(sp)\n"
        "lw s9, 4 * 27(sp)\n"
        "lw s10, 4 * 28(sp)\n"
        "lw s11, 4 * 29(sp)\n"
        "lw sp,  4 * 30(sp)\n" //i forgor lol
        "sret\n"

    );
}


extern char __free_ram[], __free_ram_end[];

paddr_t alloc_pages(uint32_t n){ //acts as a Bump allocator / Linear allocator. Simple and used when deallocation is not needed
    static paddr_t next_paddr = (paddr_t) __free_ram;
    paddr_t paddr = next_paddr;
    next_paddr += n * PAGE_SIZE;

    if (next_paddr > (paddr_t) __free_ram_end)
        PANIC("out of memory");
    
    memset((void*) paddr, 0, n * PAGE_SIZE); //0-initializing data
    return paddr;                            //normally, when a userland program reads garbage data its due to the fact that the free
                                             //funciton doen't clean the memory and malloc operates on big memory blocks, so reutilizing previously
                                            //freed memory regions without initializing the memory can be tricky. Calloc zero-initializes.

}

/* Next function ensures first-level page table entry exists (creating the
 * second-level table if needed) and then sets the second-level page table entry
 * to map the physical page
 */

void map_page(uint32_t *table1, uint32_t vaddr, paddr_t paddr, uint32_t flags){
    if (!is_aligned(vaddr, PAGE_SIZE))
        PANIC("unaligned vaddr %x", vaddr);
    
    if (!is_aligned(paddr, PAGE_SIZE))
        PANIC("unaligned paddr %x", paddr); //pages need to be aligned to a 4kb boundary

    uint32_t vpn1 = (vaddr >> 22) & 0x3ff; //we need the 10 last bits
    if ((table1[vpn1] & PAGE_V) == 0){
        //create 1st level page if it doesn't exist
        uint32_t pt_paddr = alloc_pages(1);
        table1[vpn1] = ((pt_paddr / PAGE_SIZE) << 10 | PAGE_V); //dividing by page_size erases the 12 bit offset, activating validity bit
    }

    //set the 2nd level page table entry to map the physical page
    uint32_t vpn0 = (vaddr >> 12) & 0x3ff;
    uint32_t *table0 = (uint32_t *) ((table1[vpn1] >> 10) * PAGE_SIZE);
    table0[vpn0] = ((paddr / PAGE_SIZE) << 10) | flags | PAGE_V; //contains PPN physical page number
}



extern char __kernel_base[];

void user_entry(void) {
    __asm__ __volatile__(
        "csrw sepc, %[sepc]       \n" //supervisor exception pc, where sret jumps to (normally it is used to return from exception/interrupt, but we can abuse this as shows)
        "csrw sstatus, %[sstatus] \n" //enables hw interrupts setting SPIE bit in sstatus reg
        "sret                     \n" //switch from s-mode to u-mode
        :
        : [sepc] "r" (USER_BASE),
          [sstatus] "r" (SSTATUS_SPIE)
    );
}

struct process *create_process(const void *image, size_t image_size) {
    // Find an unused process control structure.
    struct process *proc = NULL;
    int i;
    for (i = 0; i < PROCS_MAX; i++){
        if (procs[i].state == PROC_UNUSED) {

            proc = &procs[i];
            break;
        }
    }

    if (!proc)
        PANIC("no free process slots");
    
    //zero initialize callee-saved registers in stack and add pc of the first instruction of the process
    //this is kinda "trickng the OS into thinking we are going to execute a previously ran process"
    
    uint32_t *sp = (uint32_t *) &proc->stack[sizeof(proc->stack)]; //reverse of &proc->stack
    *--sp = 0;                      // s11
    *--sp = 0;                      // s10
    *--sp = 0;                      // s9
    *--sp = 0;                      // s8
    *--sp = 0;                      // s7
    *--sp = 0;                      // s6
    *--sp = 0;                      // s5
    *--sp = 0;                      // s4
    *--sp = 0;                      // s3
    *--sp = 0;                      // s2
    *--sp = 0;                      // s1
    *--sp = 0;                      // s0
    *--sp = (uint32_t) user_entry;  // ra (changed from pc)

    // Map kernel pages. All table pages of an userspace process must point to the same physical kernel memory addresses

    uint32_t *page_table = (uint32_t *) alloc_pages(1);
    for (paddr_t paddr = (paddr_t) __kernel_base; paddr < (paddr_t) __free_ram_end; paddr += PAGE_SIZE)
    map_page (page_table, paddr, paddr, PAGE_R | PAGE_W | PAGE_X); //kernel is mapping vaddr as paddr itself! The program can keep working after enabling paging
    
    //map user pages
    for (uint32_t off = 0; off < image_size; off += PAGE_SIZE) {
        paddr_t page = alloc_pages(1);

        //what if the data to be copied is smaller than the page size?

        size_t remaining = image_size - off;
        size_t copy_size = PAGE_SIZE <= remaining ? PAGE_SIZE : remaining;
        
        //fills and maps the page
        //memcopy is necessary instead of mapping directly aS IMAGE + OFF in the phys address
        //so if the current process calls another then it will have another different phys

        memcpy((void*) page, image + off, copy_size);
        map_page(page_table, USER_BASE + off, page, PAGE_U | PAGE_R | PAGE_W | PAGE_X) ;

    }

    // Initialize fields.
    proc->pid = i + 1;
    proc->state = PROC_RUNNABLE;
    proc->sp = (uint32_t) sp;
    proc->page_table = page_table;
    return proc;


}

void delay(void) {
    for (int i = 0; i < 30000000; i++)
        __asm__ __volatile__("nop");
}


struct process *proc_a;
struct process *proc_b;

void proc_a_entry(void){
    printf("Starting process A\n");
    while (1) {
        putchar('A');
        yield();
        delay();
    }
}

void proc_b_entry(void){
    printf("Starting process B\n");
    while (1) {
        putchar('B');
        yield();
        delay();
    }
}


//some bootloaders may zero-clear bss; we do it just in case
void kernel_main(void) {
    
    memset(__bss, 0, (size_t) __bss_end - (size_t) __bss); //initializes bss region to 0

    WRITE_CSR(stvec, (uint32_t) kernel_entry); //sets the kernel exception handler routine
    
    idle_proc = create_process(NULL, 0); //updt
    idle_proc->pid = 0; // idle
    current_proc = idle_proc;

    create_process(_binary_shell_bin_start, (size_t) _binary_shell_bin_size);

    //proc_a = create_process((uint32_t) proc_a_entry);
    //proc_b = create_process((uint32_t) proc_b_entry);
    
    yield();
    
    PANIC("switch idle process!");

    printf("\n\n Hello %s\n", "World!");
    printf("1 + 2 = %d, %x\n", 1 + 2, 0x1234abcd);

    paddr_t paddr0 = alloc_pages(2);
    paddr_t paddr1 = alloc_pages(1);
    printf("alloc_pages test: paddr0=%x\n", paddr0);
    printf("alloc_pages test: paddr1=%x\n", paddr1);

    PANIC("booted!");

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