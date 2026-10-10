#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#define MAXNUM_CPU 8 // CPU core

#define LENGTH_RAM 128*1024*1024 // used in os.ld 

#define UART0 0x10000000L //UARTO init address

#define UART0_IRQ 10 // UART0 在 PLIC 里的中断号，QEMU virt 约定（see virt.h）

#define PLIC_BASE 0x0c000000L
#define PLIC_PRIORITY(id) (PLIC_BASE + (id) * 4)
#define PLIC_PENDING(id) (PLIC_BASE + 0x1000 + ((id) / 32) * 4)
#define PLIC_MENABLE(hart, id) (PLIC_BASE + 0x2000 + (hart) * 0x80 + ((id) / 32) * 4)
#define PLIC_MTHRESHOLD(hart) (PLIC_BASE + 0x200000 + (hart) * 0x1000)
#define PLIC_MCLAIM(hart) (PLIC_BASE + 0x200004 + (hart) * 0x1000)
#define PLIC_MCOMPLETE(hart) (PLIC_BASE + 0x200004 + (hart) * 0x1000)

#define CLINT_BASE 0x2000000L
#define CLINT_MSIP(hartid) (CLINT_BASE + 4 * (hartid))
#define CLINT_MTIMECMP(hartid) (CLINT_BASE + 0x4000 + 8 * (hartid))
#define CLINT_MTIME (CLINT_BASE + 0xBFF8)

#define CLINT_TIMEBASE_FREQ 10000000

#endif
