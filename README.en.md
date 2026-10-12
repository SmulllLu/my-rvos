[中文](README.md) | **English**

# my-rvos

A **RISC-V 32-bit operating system kernel** written from scratch, running on QEMU's `virt` virtual
board. About 1,500 lines implementing task scheduling, interrupt handling, timers, spinlocks, system
calls, and M / U mode privilege separation.

Built while following the
[《循序渐进，学习开发一个 RISC-V 上的操作系统》](https://gitee.com/unicornx/riscv-operating-system-mooc)
course by Wang Chen / PLCT Lab.

| | |
|---|---|
| Toolchain | `riscv64-unknown-elf-gcc` 14.2.0 |
| Emulator | `qemu-system-riscv32` 10.1.0 |

## Features

| Subsystem | What's implemented | Location |
|---|---|---|
| Boot | Multi-hart bring-up, `.bss` zeroing, stack setup, PMP config | `arch/riscv/start.S` |
| UART | NS16550A driver — polled TX, interrupt-driven RX | `drivers/uart.c` |
| Formatted output | `printf` / `panic` (varargs, no libc) | `lib/printf.c` |
| Physical pages | Bitmap allocator, 4 KB pages | `mm/page.c` |
| Heap | Byte-granular `malloc` / `free` with splitting and coalescing | `mm/malloc.c` |
| Tasks | `task_create`, FIFO round-robin, context switching | `kernel/sched.c` |
| Traps | Vector table, save/restore of 29 general registers (`gp`/`tp` excluded) | `arch/riscv/entry.S`, `kernel/trap.c` |
| External IRQs | PLIC claim / complete, interrupt-driven UART | `drivers/plic.c` |
| Hardware timer | CLINT `mtime` / `mtimecmp`, periodic tick | `kernel/timer.c` |
| Preemption | `pc` added to the task context, forced switch from the timer IRQ | `kernel/sched.c` |
| Spinlock | Interrupt-disable `spin_lock` / `spin_unlock` | `kernel/lock.c` |
| Software timers | One hardware tick driving up to 10 timers of arbitrary duration | `kernel/timer.c` |
| System calls | U-mode `ecall`, kernel dispatches on `a7` | `kernel/syscall.c`, `user/usys.S` |

Not implemented yet: virtual memory / MMU, filesystem, networking, parallel multi-core.

## Getting started

```bash
# Ubuntu / Debian
sudo apt install gcc-riscv64-unknown-elf qemu-system-misc gdb-multiarch

make        # build; artifacts land in out/
make run    # boot under QEMU (Ctrl-A then X to quit)
```

| Other targets | Purpose |
|---|---|
| `make clean` | Remove `out/` |
| `make code` | Disassemble the kernel to compare against the C source |
| `make debug` | Start QEMU and attach `gdb-multiarch` |

### Output

```
Hello, RVOS!
HEAP_START = 0x80008190(aligned to 0x80009000), HEAP_SIZE=0x07ff7e70,
num of reserved pages = 8, num of pages to be allocated for heap = 32751
TEXT:   0x80000000 -> 0x8000440c
DATA:   0x80005000 -> 0x8000500c
RODATA: 0x8000440c -> 0x80004890
BSS:    0x80005010 -> 0x80008190
HEAP:   0x80011000 -> 0x88000000

----- malloc test -----
p  = 0x8007b010
q  = 0x8007b030   (should be p + 0x20)
big = 0x8007c010 (5000-byte block, spans two pages)
...

Task 0: Created!
Sync exception! Code = 8          <- ecall from U-mode
System call from U-mode!
--> sys_gethid, arg0 = 0x800057f8
system call returned!, hart id is 0
Task 0: Running...
timer interruption!
tick: 1
Task 1: Created!
Task 1: Running...
```

- `Code = 8` means the ecall came from U-mode; `Code = 11` would mean the task is still in M-mode
  and the privilege drop never took effect
- The two tasks alternate and `tick` climbs — every switch is driven by the timer interrupt, neither
  task ever yields voluntarily
- Typing while it runs produces `external interruption!` plus an echo, i.e. the PLIC and UART IRQ working

> Addresses come from a real run and shift as the code changes; `TEXT` always starts at `0x80000000`.

## Layout

```
my-rvos/
├── Makefile  common.mk  os.ld      build entry points
│
├── arch/riscv/     the ONLY code to change for a different CPU or board
│   ├── start.S          first instruction after power-on
│   ├── entry.S          trap entry, register save/restore, switch_to
│   ├── mem.S            memory access assembly
│   ├── riscv.h          CSR numbers, mstatus bitfields, exception-code masks
│   └── platform.h       peripheral address map for the QEMU virt board
│
├── kernel/         arch-independent core
│   ├── kernel.c         start_kernel: subsystem init order
│   ├── sched.c          FIFO scheduler, context switching
│   ├── trap.c           trap dispatch: interrupts / exceptions / syscalls
│   ├── timer.c          hardware tick + software timers
│   ├── lock.c           interrupt-disable spinlock
│   └── syscall.c        system call dispatch
│
├── mm/             memory management
├── drivers/        device drivers (uart, plic)
├── lib/            general utilities (printf)
├── include/        headers shared across subsystems (os.h, types.h, syscall.h)
├── user/           user-mode code and the ecall stub (user.c, user_api.h, usys.S)
└── out/            build artifacts (gitignored)
```

Top-level naming follows mainstream kernels (`arch/`, `kernel/`, `mm/`, `drivers/`, `lib/`, `include/`
as in Linux / Zephyr / RT-Thread; `user/` as in xv6). The split between `arch/riscv/` and the rest is
the split between "rewrite this to change architecture" and "architecture-independent".
`include/` holds only headers shared across subsystems — `syscall.h` lives there because it is the ABI
contract obeyed by both the kernel and user mode.

## Design notes

### Boot

```
Power-on PC = 0x80000000
      │
      ▼
   start.S ──► read mhartid → non-zero harts park in wfi
      │        zero .bss, carve a 1 KB stack per hart
      │        configure PMP, prepare mstatus.MPP
      ▼
start_kernel()
      │
      ├─ uart_init()      ├─ plic_init()    └─ os_main() creates tasks
      ├─ page_init()      ├─ timer_init()          │
      ├─ trap_init()      ├─ sched_init()          ▼
      │                                     schedule() ──► switch_to ──► mret into task
```

### Traps and context switching

`trap_vector` and `switch_to` share one convention: `mscratch` always points at the current task's
`struct context`; `t6` serves as the macros' base register, so `reg_save` / `reg_restore` omit it
(it is stored and loaded by hand); slot 30 is `t6`, slot 31 is `pc`.

```
hardware: mepc <- interrupted PC, mcause <- cause, PC <- mtvec, MIE <- 0
  │
  ▼
trap_vector:
  csrrw t6, mscratch, t6     <- swap in the context address as base (t6 parks in mscratch)
  reg_save t6                <- store 28 registers
  store t6 into slot 30
  store mepc into slot 31    <- where the next switch back resumes
  csrw  mscratch, t5         <- mscratch points back at the current task
  call  trap_handler(epc, cause, ctx)
  csrw  mepc, a0             <- return value = address to resume at
  reg_restore t6
  mret
```

The switch happens inside the trap: `trap_handler` calls `schedule()` → `switch_to(next)`, which
restores the next task's registers and executes `mret` directly. It never returns, so this trap's
return path is abandoned.

### Privilege levels and system calls

Tasks drop to U-mode via `mstatus.MPP = 0` and cannot read M-mode CSRs, so operations like
`r_mhartid()` are performed by the kernel on their behalf:

```
user mode                                 kernel (M-mode)
  │  gethid(&hid)                          user/usys.S
  │    li a7, SYS_gethid                    <- a7 = syscall number
  │    ecall ──────────────────────────────► trap_vector ──► trap_handler
  │                                          │  case 8: Environment Call from U-mode
  │                                          ▼
  │                                    do_syscall(cxt)       kernel/syscall.c
  │                                      dispatch on cxt->a7, call sys_gethid()
  │                                      read mhartid in M-mode, write back to cxt->a0
  │  ◄───────────────────────────────────────┘
  │    mepc += 4                            <- ecall is 4 bytes; must skip it
  ▼    mret
a0 holds the return value
```

Three things that matter:

1. **`return_pc += 4` is mandatory** — `mepc` points at the `ecall` instruction itself; without the
   skip you get an infinite loop
2. **A `struct context *` must be passed into `trap_handler`** — the return value goes into the
   caller's `a0`, and the context pointer is the only channel
3. **PMP must be configured** — every U-mode memory access goes through a PMP check; without it the
   task faults the moment it touches the UART MMIO

## Known limitations

- **Single core**: only hart 0 runs, the rest sit in `wfi`; the interrupt-disable `spin_lock` is
  useless on multiple cores
- **No virtual memory**: tasks use physical addresses directly — no page tables, no address-space isolation
- **Mixed cooperative and preemptive**: `task_yield()` is a voluntary yield via software interrupt,
  the timer tick preempts
- **PMP allows everything**: `pmpaddr0 = 0xffffffff` is a temporary compromise from the course; the real
  approach grants only the user's region
- **`spin_lock` is not nestable**: `spin_unlock` unconditionally re-enables interrupts

## References

- [riscv-operating-system-mooc](https://gitee.com/unicornx/riscv-operating-system-mooc) (Wang Chen / PLCT Lab)
- [The RISC-V Instruction Set Manual, Volume II: Privileged Architecture](https://riscv.org/technical/specifications/)
- [xv6: a simple, Unix-like teaching operating system](https://pdos.csail.mit.edu/6.828/2020/xv6.html)
