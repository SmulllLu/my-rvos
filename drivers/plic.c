#include "os.h"

void plic_init(void)
{
	int hart = r_tp();          // 当前核的 hartid：PLIC 每组寄存器是每核独立一份

	/*
	 * 设置 UART0 的优先级。
	 *
	 * 每个 PLIC 中断源都可以通过写它对应的 32 位内存映射优先级寄存器来指定优先级。
	 * QEMU-virt（与 FU540-C000 相同）支持 7 级优先级。
	 * 优先级 0 是保留值，含义是"永远不打断"，等同于禁用该中断。
	 * 优先级 1 是最低的可用优先级，7 是最高。
	 * 优先级相同的多个中断之间用中断号（ID）决出先后：
	 * ID 越小，实际优先级越高。
	 */
	*(uint32_t*)PLIC_PRIORITY(UART0_IRQ) = 1;

	/*
	 * 使能 UART0
	 *
	 * 每个全局中断通过把 enables 寄存器里对应的那一位置 1 来使能。
	 */
	*(uint32_t*)PLIC_MENABLE(hart, UART0_IRQ)= (1 << (UART0_IRQ % 32));

	/*
	 * 设置 UART0 的优先级门槛（threshold）。
	 *
	 * PLIC 会屏蔽所有优先级小于等于 threshold 的中断。
	 * threshold 最大为 7。
	 * 例如：threshold 取 0 时放行所有优先级非 0 的中断，
	 * 而取 7 时屏蔽一切中断。
	 * 注意：threshold 是 PLIC 全局的一个值，不是每个中断源各自的。
	 */
	*(uint32_t*)PLIC_MTHRESHOLD(hart) = 0;

	/* 使能机器模式的外部中断（mie.MEIE 位）。 */
	w_mie(r_mie() | MIE_MEIE);
}

/*
 * 说明：
 *	向 PLIC 询问当前该处理哪个中断。
 *	读 claim 寄存器即完成一次"认领"：返回当前优先级最高的
 *	待处理中断的 ID；若没有待处理中断则返回 0。
 *	认领成功的同时，硬件会原子地清掉该中断源对应的 pending 位。
 * 返回值：
 *	优先级最高的待处理中断 ID；没有待处理中断时为 0。
 */
int plic_claim(void)
{
	int hart = r_tp();
	int irq = *(uint32_t*)PLIC_MCLAIM(hart);
	return irq;
}

/*
 * 说明：
 *	把从 claim 拿到中断 ID（irq）写回 complete 寄存器，
 *	即告诉 PLIC 这个中断我们已经处理完了。
 *	PLIC 并不检查写入的完成 ID 是否等于该核上次认领的 ID。
 *	如果写入的 ID 对应该核当前未使能的中断源，这次完成会被静默忽略。
 * 返回值：无
 */
void plic_complete(int irq)
{
	int hart = r_tp();
	*(uint32_t*)PLIC_MCOMPLETE(hart) = irq;
}
