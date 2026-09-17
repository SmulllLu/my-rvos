USE_LINKER_SCRIPT = true

SRCS_ASM = \
	start.S \
	mem.S \
	entry.S

SRCS_C = \
	kernel.c \
	uart.c \
	page.c \
	printf.c \
	malloc.c \
	sched.c \
	user.c

include ./common.mk
