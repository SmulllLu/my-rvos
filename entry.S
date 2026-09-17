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
    csrrw   t6, mscratch,   t6  # 交换 t6 ↔ mscratch
    beqz    t6, 1f              # t6==0？→ 第一次切换，跳去恢复
    reg_save    t6              # 保存上一个任务的所有寄存器
    mv  t5, t6                  # t5 = t6 t5记住“旧任务上下体的地址”
    csrr    t6, mscratch        # 把真正的 t6 取回来
    STORE   t6, 30*SIZE_REG(t5) # 旧任务的 t6 也存好 
1:
    csrw    mscratch,   a0      # mscratch 指向新任务的 context
    mv      t6, a0              # t6 也指向它，当恢复的 base
    reg_restore t6              # 恢复新任务的全部寄存器
    ret                         # ra 已经是新任务的 ra → "跳"进新任务