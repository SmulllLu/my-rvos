**中文** | [English](README.en.md)

# my-rvos

一个从零手写的 **RISC-V 32 位操作系统内核**，跑在 QEMU 的 `virt` 虚拟开发板上。约 1500 行代码，
实现了任务调度、中断处理、定时器、自旋锁、系统调用，以及 M / U 模式的特权级隔离。

跟随[《循序渐进，学习开发一个 RISC-V 上的操作系统》](https://gitee.com/unicornx/riscv-operating-system-mooc)
（汪辰 / PLCT Lab）课程实现。

| | |
|---|---|
| 工具链 | `riscv64-unknown-elf-gcc` 14.2.0 |
| 模拟器 | `qemu-system-riscv32` 10.1.0 |

## 功能一览

| 子系统 | 实现内容 | 位置 |
|---|---|---|
| 启动 | 多核引导、`.bss` 清零、栈分配、PMP 配置 | `arch/riscv/start.S` |
| 串口 | NS16550A 驱动，轮询发送 + 中断接收 | `drivers/uart.c` |
| 格式化输出 | `printf` / `panic`（可变参数，无 libc） | `lib/printf.c` |
| 物理页分配 | 位图式分配器，4KB 一页 | `mm/page.c` |
| 堆分配 | 字节粒度 `malloc` / `free`，带块合并与分裂 | `mm/malloc.c` |
| 任务 | `task_create`、FIFO 轮转调度、上下文切换 | `kernel/sched.c` |
| trap | 向量表、29 个通用寄存器保存恢复（`gp`/`tp` 除外） | `arch/riscv/entry.S`、`kernel/trap.c` |
| 外部中断 | PLIC claim / complete，UART 中断驱动 | `drivers/plic.c` |
| 硬件定时器 | CLINT `mtime` / `mtimecmp`，周期性 tick | `kernel/timer.c` |
| 抢占式调度 | 上下文增加 `pc`，定时器中断里强制切换 | `kernel/sched.c` |
| 自旋锁 | 关中断实现 `spin_lock` / `spin_unlock` | `kernel/lock.c` |
| 软件定时器 | 一个硬件节拍驱动最多 10 个任意时长的定时任务 | `kernel/timer.c` |
| 系统调用 | U 模式 `ecall` 陷入，内核按 `a7` 分发 | `kernel/syscall.c`、`user/usys.S` |

尚未实现：虚拟内存 / MMU、文件系统、网络、多核并行。

## 快速开始

```bash
# Ubuntu / Debian
sudo apt install gcc-riscv64-unknown-elf qemu-system-misc gdb-multiarch

make        # 编译，产物在 out/
make run    # 在 QEMU 里运行（Ctrl-A 再 X 退出）
```

| 其他目标 | 作用 |
|---|---|
| `make clean` | 清掉 `out/` |
| `make code` | 反汇编内核，对照 C 源码看编译结果 |
| `make debug` | 启动 QEMU 并挂上 `gdb-multiarch` |

### 运行效果

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
q  = 0x8007b030   (应与 p 差 0x20)
big = 0x8007c010 (5000 进的块，两页)
...

Task 0: Created!
Sync exception! Code = 8          ← ecall from U-mode
System call from U-mode!
--> sys_gethid, arg0 = 0x800057f8
system call returned!, hart id is 0
Task 0: Running...
timer interruption!
tick: 1
Task 1: Created!
Task 1: Running...
```

- `Code = 8` 表示 ecall 来自 U 模式；若为 `Code = 11` 则说明任务仍在 M 模式，特权级没降下去
- 两个任务交替打印、`tick` 递增，切换完全由定时器中断驱动，任务本身没有主动让出
- 运行中敲键盘会出现 `external interruption!` 和回显，这是 PLIC + UART 中断在工作

> 地址来自某次真实运行，改动代码后整体平移；`TEXT` 起点恒为 `0x80000000`。

## 目录结构

```
my-rvos/
├── Makefile  common.mk  os.ld      构建入口
│
├── arch/riscv/     换 CPU / 换开发板时唯一要改的代码
│   ├── start.S          上电第一条指令
│   ├── entry.S          trap 入口、寄存器保存恢复、switch_to
│   ├── mem.S            内存读写汇编
│   ├── riscv.h          CSR 编号、mstatus 位域、异常码掩码
│   └── platform.h       QEMU virt 板的外设地址表
│
├── kernel/         内核核心，与架构无关
│   ├── kernel.c         start_kernel：子系统初始化顺序
│   ├── sched.c          FIFO 调度器、上下文切换
│   ├── trap.c           trap 分发：中断 / 异常 / 系统调用
│   ├── timer.c          硬件节拍 + 软件定时器
│   ├── lock.c           关中断版自旋锁
│   └── syscall.c        系统调用分发
│
├── mm/             内存管理
├── drivers/        设备驱动（uart、plic）
├── lib/            通用库（printf）
├── include/        跨子系统共享的头（os.h、types.h、syscall.h）
├── user/           用户态代码与 ecall 桩（user.c、user_api.h、usys.S）
└── out/            构建产物（已 gitignore）
```

顶层命名对齐主流内核（Linux / Zephyr / RT-Thread 的 `arch/`、`kernel/`、`mm/`、`drivers/`、`lib/`、`include/`，
`user/` 对齐 xv6）。`arch/riscv/` 与其余目录的分界即"换架构要重写"与"与架构无关"的分界。
`include/` 只放跨子系统共享的头文件 —— `syscall.h` 放在这里，是因为它是内核与用户态共同遵守的 ABI 契约。

## 关键设计

### 启动流程

```
上电 PC = 0x80000000
      │
      ▼
   start.S ──► 读 mhartid → 非 0 号核 wfi 待命
      │        清 .bss、给每个核分 1KB 栈
      │        配置 PMP、准备 mstatus.MPP
      ▼
start_kernel()
      │
      ├─ uart_init()      ├─ plic_init()    └─ os_main() 创建任务
      ├─ page_init()      ├─ timer_init()          │
      ├─ trap_init()      ├─ sched_init()          ▼
      │                                     schedule() ──► switch_to ──► mret 进入任务
```

### trap 与任务切换

`trap_vector` 与 `switch_to` 共用一套约定：`mscratch` 始终指向当前任务的 `struct context`；
`t6` 作为宏的基址寄存器，所以 `reg_save` / `reg_restore` 不含 `t6`（单独手工存取）；
槽位 30 是 `t6`，槽位 31 是 `pc`。

```
硬件：mepc ← 被打断的 PC，mcause ← 原因，PC ← mtvec，MIE ← 0
  │
  ▼
trap_vector:
  csrrw t6, mscratch, t6     ← 换出 context 地址当基址（t6 真身寄存在 mscratch）
  reg_save t6                ← 存 28 个寄存器
  补存 t6 到槽位 30
  存 mepc 到槽位 31          ← 下次切回来时从这里恢复
  csrw  mscratch, t5         ← mscratch 指回当前任务
  call  trap_handler(epc, cause, ctx)
  csrw  mepc, a0             ← 返回值 = 回去继续执行的地址
  reg_restore t6
  mret
```

切换发生在 trap 内部：`trap_handler` 调用 `schedule()` → `switch_to(next)`，
后者恢复下一个任务的寄存器后直接 `mret`，从不返回，本次 trap 的返回路径就此作废。

### 特权级与系统调用

任务通过 `mstatus.MPP = 0` 降到 U 模式运行，无法读取 M 模式的 CSR，`r_mhartid()` 这类操作由内核代劳：

```
用户态                                    内核（M 模式）
  │  gethid(&hid)                          user/usys.S
  │    li a7, SYS_gethid                    ← a7 = 系统调用号
  │    ecall ──────────────────────────────► trap_vector ──► trap_handler
  │                                          │  case 8: Environment Call from U-mode
  │                                          ▼
  │                                    do_syscall(cxt)       kernel/syscall.c
  │                                      按 cxt->a7 分发，调用 sys_gethid()
  │                                      在 M 模式下读 mhartid，结果写回 cxt->a0
  │  ◄───────────────────────────────────────┘
  │    mepc += 4                            ← ecall 是 4 字节指令，必须跳过
  ▼    mret
a0 里拿到返回值
```

三个要点：

1. **`return_pc += 4` 不能省** —— `mepc` 指向 `ecall` 这条指令自己，不跳过就是死循环
2. **必须把 `struct context *` 传进 `trap_handler`** —— 返回值要写回调用者的 `a0`，context 是唯一通道
3. **PMP 必须配置** —— U 模式每次访存都过 PMP 检查，未配置时任务一访问 UART 的 MMIO 就异常

## 已知限制

- **单核**：只有 0 号核运行，其余核 `wfi` 待命；关中断实现的 `spin_lock` 在多核上无效
- **无虚拟内存**：任务直接使用物理地址，无页表、无地址空间隔离
- **协作与抢占混合**：`task_yield()` 是软中断触发的主动让出，定时器 tick 是抢占
- **PMP 全放行**：`pmpaddr0 = 0xffffffff` 是课程的临时妥协，真实做法是只放行用户区域
- **`spin_lock` 不可嵌套**：`spin_unlock` 无条件开中断，嵌套会提前解锁

## 参考

- [riscv-operating-system-mooc](https://gitee.com/unicornx/riscv-operating-system-mooc)（汪辰 / PLCT Lab）
- [The RISC-V Instruction Set Manual, Volume II: Privileged Architecture](https://riscv.org/technical/specifications/)
- [xv6: a simple, Unix-like teaching operating system](https://pdos.csail.mit.edu/6.828/2020/xv6.html)
