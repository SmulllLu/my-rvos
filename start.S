#include "platform.h"
    .equ    STACK_SIZE, 1024
    .global _start
    .text

_start:
    csrr    t0, mhartid         # 读当前核的编号
    mv  tp, t0                  # tp 存 hartid，供后面的 r_tp() 使用
    bnez    t0, park            # 只有 0 号核干活，其它核原地待命

    la  a0, _bss_start          # 清 .bss 段：C 全局变量初值全 0 的保证
    la  a1, _bss_end
    bgeu    a0, a1, 2f
1:
    sw  zero,   (a0)         # 往 a0 指向的地址写 0（一个字 = 4 字节）
    addi    a0, a0, 4        # 指针前进 4 字节
    bltu    a0, a1, 1b       # 还没到末尾 → 回去继续
2:
    slli    t0, t0, 10              # hartid * 1024，算本核栈的偏移
    la  sp, stacks+STACK_SIZE       # sp 指向第一个核的栈顶（栈从高往低长）

    add sp, sp, t0                  # 挪到本核自己的栈上

    li  t0, 3 << 11 | 1 << 7    # MPP=3（mret 后回 M 模式）+ MPIE=1（mret 后开中断）
    csrs    mstatus, t0         # 给第一次 mret 手工准备"现场"（那是唯一没经过 trap 的 mret）

    j start_kernel              # 跳到 C 代码

park:
    wfi
    j park
.balign 16

# 所有核的栈空间
stacks:
    .skip   STACK_SIZE*MAXNUM_CPU

    .end
