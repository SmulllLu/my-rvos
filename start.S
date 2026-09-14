#include "platform.h"
    .equ    STACK_SIZE, 1024
    .global _start
    .text

_start:
    csrr    t0, mhartid
    mv  tp, t0
    bnez    t0, park
    slli    t0, t0, 10
    la  sp, stacks+STACK_SIZE

    add sp, sp, t0
    j start_kernel
    
park:
    wfi
    j park
.balign 16

# set stacks
stacks:
    .skip   STACK_SIZE*MAXNUM_CPU

    .end