USE_LINKER_SCRIPT = true
SYSCALL = y

# 头文件搜索路径：源文件里写的 #include "xxx.h" 靠这两行找到
CFLAGS += -Iinclude -Iarch/riscv

# 注意：start.S 必须排在第一位（见 common.mk 的说明），
# 它要落在 0x80000000，这是开发板复位后 PC 的落点。
SRCS_ASM = \
	arch/riscv/start.S \
	arch/riscv/mem.S \
	arch/riscv/entry.S \
	user/usys.S

SRCS_C = \
	kernel/kernel.c \
	drivers/uart.c \
	mm/page.c \
	lib/printf.c \
	mm/malloc.c \
	kernel/trap.c \
	kernel/sched.c \
	user/user.c \
	drivers/plic.c \
	kernel/timer.c \
	kernel/lock.c \
	kernel/syscall.c

include ./common.mk
