#pragma once

//next macros are defined in stdarg, but we will use the builtins directly. We could use that lib as its always provided by the compiler by the C standard

#define va_list __builtin_va_list  //declares variable that will store info needed to retrieve the additional arguments
#define va_start __builtin_va_start //initializes va_list to retrieve arguments: va_start(list, last_fixed_arg) where last_fixed_arg is the arg before (...)
#define va_arg __builtin_va_arg //returns the next argument of the list: va_arg(list, type) must be caled n times, one for each argument, and it shall not exceed the number of args passed.
#define va_end __builtin_va_end // va_end(list) cleans up all resources associated.

void printf(const char *fmt, ...);