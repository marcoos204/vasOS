#pragma once
#include "common.h"

__attribute__((noreturn)) void exit(void); //noreturn -> functions that terminate app, throw exceptions, loop...
                                           //if this function returns control, then its UB
                                           //atrtribute used to optimize code and hide compiler warnings 
void putchar(char ch);

int getchar(void);