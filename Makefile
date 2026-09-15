USE_LINKER_SCRIPT = true

SRCS_ASM = \
	start.S \
	mem.S \

SRCS_C = \
	kernel.c \
	uart.c \
	page.c \
	printf.c \

include ./common.mk
