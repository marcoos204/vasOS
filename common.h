#pragma once

typedef int bool;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef uint32_t size_t;
typedef uint32_t paddr_t; //type representing physical memory addresses
typedef uint32_t vaddr_t; //type representing virtual memory addresses. Equivalent to uintptr_t in the std library

#define true 1
#define false 0
#define NULL ((void *) 0)
#define align_up(value, align)      __builtin_align_up(value, align) //rounds up the value to the nearest multiple of align. align must be power of 2
#define is_aligned(value, align)    __builtin_is_aligned(value, align) //checks if value is a multiple of align. align must be a power of 2
#define offsetof(type, member)      __builtin_offsetof(type, member) //returns offset of a member within a struct (how many bytes from the start of the structure)

//next macros are defined in stdarg, but we will use the builtins directly. We could use that lib as its always provided by the compiler by the C standard

#define va_list __builtin_va_list  //declares variable that will store info needed to retrieve the additional arguments
#define va_start __builtin_va_start //initializes va_list to retrieve arguments: va_start(list, last_fixed_arg) where last_fixed_arg is the arg before (...)
#define va_arg __builtin_va_arg //returns the next argument of the list: va_arg(list, type) must be caled n times, one for each argument, and it shall not exceed the number of args passed.
#define va_end __builtin_va_end // va_end(list) cleans up all resources associated.

void *memset(void *buf, char c, size_t n);
void *memcpy(void *dst, const void *src, size_t n);
char *strcpy(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);

void printf(const char *fmt, ...);