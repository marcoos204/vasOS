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

#define PROCS_MAX 8 //Max number of processes
#define PROC_UNUSED 0 //unused process control structure
#define PROC_RUNNABLE 1 //Runabble process
#define PROC_EXITED 2 //exited process


struct process {
    int pid;        //Process ID
    int state;      //Process state: PROC_UNUSED or PROC_RUNNABLE
    vaddr_t sp;     //Stack pointer
    uint32_t *page_table;
    uint8_t stack[8192]; //kernel stack

};

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

#define SATP_SV32 (1u << 31) //single bit in the satp register, "enable paging in sv32 mode". PAGE_* are flags to be set in page table entries
#define PAGE_V (1 << 0) // "Valid" bit (entry is enabled)
#define PAGE_R (1 << 1) // Readable
#define PAGE_W (1 << 2) // Writable
#define PAGE_X (1 << 3) // Executable
#define PAGE_U (1 << 4) // User (accesible in user mode)
#define USER_BASE 0x1000000 // in ELF the load address its stored in the file header Since our app is a raw binary, we need to prepare a fixed value
#define SSTATUS_SPIE (1<<5)
#define SCAUSE_ECALL 8

//virtio-related definitions in kernel.h

#define SECTOR_SIZE       512
#define VIRTQ_ENTRY_NUM   16
#define VIRTIO_DEVICE_BLK 2
#define VIRTIO_BLK_PADDR  0x10001000 //physical device driver address
#define VIRTIO_REG_MAGIC         0x00
#define VIRTIO_REG_VERSION       0x04
#define VIRTIO_REG_DEVICE_ID     0x08
#define VIRTIO_REG_PAGE_SIZE     0x28
#define VIRTIO_REG_QUEUE_SEL     0x30
#define VIRTIO_REG_QUEUE_NUM_MAX 0x34
#define VIRTIO_REG_QUEUE_NUM     0x38
#define VIRTIO_REG_QUEUE_PFN     0x40
#define VIRTIO_REG_QUEUE_READY   0x44
#define VIRTIO_REG_QUEUE_NOTIFY  0x50
#define VIRTIO_REG_DEVICE_STATUS 0x70
#define VIRTIO_REG_DEVICE_CONFIG 0x100
#define VIRTIO_STATUS_ACK       1
#define VIRTIO_STATUS_DRIVER    2
#define VIRTIO_STATUS_DRIVER_OK 4
#define VIRTQ_DESC_F_NEXT          1
#define VIRTQ_DESC_F_WRITE         2
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
#define VIRTIO_BLK_T_IN  0
#define VIRTIO_BLK_T_OUT 1

//Virtqueue descriptor dable entry

struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
} __attribute__((packed));

//Virtqueue Available RIng

struct virtq_avail {
    uint16_t flags;
    uint16_t index;
    uint16_t ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// Virtqueue Used Ring entry.

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
} __attribute__((packed));

// Virtqueue Used Ring.
struct virtq_used {
    uint16_t flags;
    uint16_t index;
    struct virtq_used_elem ring[VIRTQ_ENTRY_NUM];
} __attribute__((packed));

// Virtqueue.
struct virtio_virtq {
    struct virtq_desc descs[VIRTQ_ENTRY_NUM];
    struct virtq_avail avail;
    struct virtq_used used __attribute__((aligned(PAGE_SIZE)));
    int queue_index;
    volatile uint16_t *used_index;
    uint16_t last_used_index;
} __attribute__((packed));

// Virtio-blk request.
struct virtio_blk_req {

    //first descriptor, read-only from the device
    uint32_t type;
    uint32_t reserved;
    uint64_t sector;

    //second descriptor, writable by the device if its a read op
    uint8_t data[512];

    //third descriptor, writable by the device
    uint8_t status;
} __attribute__((packed));

#define FILES_MAX      2
#define DISK_MAX_SIZE  align_up(sizeof(struct file) * FILES_MAX, SECTOR_SIZE)
#define SSTATUS_SUM  (1 << 18)

struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    char type;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char padding[12];
    char data[];      // Array pointing to the data area following the header
                      // (flexible array member)
} __attribute__((packed));

struct file {
    bool in_use;      // Indicates if this file entry is in use
    char name[100];   // File name
    char data[1024];  // File content
    size_t size;      // File size
};
