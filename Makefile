USE_LINKER_SCRIPT = true
SYSCALL = y

SRCS_ASM = \
	start.S \
	mem.S \
	entry.S \
	usys.S

SRCS_C = \
	kernel.c \
	uart.c \
	page.c \
	printf.c \
	malloc.c \
	trap.c \
	sched.c \
	user.c \
	plic.c \
	timer.c \
	lock.c	\
	syscall.c \

include ./common.mk
