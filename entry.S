#define LOAD		lw
#define STORE		sw
#define SIZE_REG	4

// 把当前所有通用寄存器依次存进 \base（某块内存地址）指向的 struct context
.macro reg_save base
    STORE ra,   0*SIZE_REG(\base)
    STORE sp,   1*SIZE_REG(\base)
    STORE t0,   4*SIZE_REG(\base)
    STORE t1,   5*SIZE_REG(\base)
    STORE t2,   6*SIZE_REG(\base)
    STORE s0,   7*SIZE_REG(\base)
    STORE s1,   8*SIZE_REG(\base)
    STORE a0,   9*SIZE_REG(\base)
    STORE a1,  10*SIZE_REG(\base)
    STORE a2,  11*SIZE_REG(\base)
    STORE a3,  12*SIZE_REG(\base)
    STORE a4,  13*SIZE_REG(\base)
    STORE a5,  14*SIZE_REG(\base)
    STORE a6,  15*SIZE_REG(\base)
    STORE a7,  16*SIZE_REG(\base)
    STORE s2,  17*SIZE_REG(\base)
    STORE s3,  18*SIZE_REG(\base)
    STORE s4,  19*SIZE_REG(\base)
    STORE s5,  20*SIZE_REG(\base)
    STORE s6,  21*SIZE_REG(\base)
    STORE s7,  22*SIZE_REG(\base)
    STORE s8,  23*SIZE_REG(\base)
    STORE s9,  24*SIZE_REG(\base)
    STORE s10, 25*SIZE_REG(\base)
    STORE s11, 26*SIZE_REG(\base)
    STORE t3,  27*SIZE_REG(\base)
    STORE t4,  28*SIZE_REG(\base)
    STORE t5,  29*SIZE_REG(\base)
.endm
// 取出地址
.macro reg_restore base
	LOAD ra,   0*SIZE_REG(\base)
	LOAD sp,   1*SIZE_REG(\base)
	LOAD t0,   4*SIZE_REG(\base)
	LOAD t1,   5*SIZE_REG(\base)
	LOAD t2,   6*SIZE_REG(\base)
	LOAD s0,   7*SIZE_REG(\base)
	LOAD s1,   8*SIZE_REG(\base)
	LOAD a0,   9*SIZE_REG(\base)
	LOAD a1,  10*SIZE_REG(\base)
	LOAD a2,  11*SIZE_REG(\base)
	LOAD a3,  12*SIZE_REG(\base)
	LOAD a4,  13*SIZE_REG(\base)
	LOAD a5,  14*SIZE_REG(\base)
	LOAD a6,  15*SIZE_REG(\base)
	LOAD a7,  16*SIZE_REG(\base)
	LOAD s2,  17*SIZE_REG(\base)
	LOAD s3,  18*SIZE_REG(\base)
	LOAD s4,  19*SIZE_REG(\base)
	LOAD s5,  20*SIZE_REG(\base)
	LOAD s6,  21*SIZE_REG(\base)
	LOAD s7,  22*SIZE_REG(\base)
	LOAD s8,  23*SIZE_REG(\base)
	LOAD s9,  24*SIZE_REG(\base)
	LOAD s10, 25*SIZE_REG(\base)
	LOAD s11, 26*SIZE_REG(\base)
	LOAD t3,  27*SIZE_REG(\base)
	LOAD t4,  28*SIZE_REG(\base)
	LOAD t5,  29*SIZE_REG(\base)
	LOAD t6,  30*SIZE_REG(\base)
.endm

.text

.globl switch_to
.balign 4
switch_to:
    csrw    mscratch,   a0
    # 设置下一个任务的mepc到pc中
    LOAD    a1, 31*SIZE_REG(a0)
    csrw    mepc,   a1
    
    mv      t6, a0              # t6 也指向它，当恢复的 base
    reg_restore t6              # 恢复新任务的全部寄存器
    
    mret                         # ra 已经是新任务的 ra → "跳"进新任务

# 中断/异常入口：trap 发生时硬件把 PC 跳到 mtvec 指向的这里。
# 进来时 mepc/mcause 已由硬件写好，mscratch 里是当前任务的 context 指针。
.globl trap_vector
.balign 4                       # trap 入口地址必须 4 字节对齐
trap_vector:

    csrrw   t6, mscratch,   t6  # 换出当前任务 context 地址当基址，t6 真身寄存在 mscratch
    reg_save    t6              # 存 28 个寄存器（reg_save 宏里故意不含 t6）

    mv      t5, t6              # t5 拿稳 context 地址
    csrr    t6, mscratch        # 把 t6 真身取回来
    STORE   t6, 30*SIZE_REG(t5) # 补存第 30 格——和 switch_to 同一个套路

    csrr    a0, mepc
    STORE   a0, 31*SIZE_REG(t5)

    csrw    mscratch,   t5      # mscratch 指回当前任务 context（刚被 t6 真身暂用）

    csrr    a0, mepc            # 参数1：被打断的 PC
    csrr    a1, mcause          # 参数2：trap 原因编号
    call    trap_handler        # C 处理函数：trap_handler(epc, cause)

    csrw    mepc,   a0          # 返回值 = 回去继续执行的地址（通常原样还回来）

    csrr    t6, mscratch        # 重新取 context 地址（trap_handler 里可能切换了任务）
    reg_restore t6              # 恢复全部 30 个寄存器

    mret                        # 从 mepc 处"返回"，像什么都没发生
