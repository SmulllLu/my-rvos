#include "os.h"

#define DELAY 5000

/* ch09 的临界区加锁测试，本章暂时关闭（原因见 user_task0 下面的注释块） */
// #define USE_LOCK

struct userdata {
	int counter;
	char *str;
};

/* Jack must be global */
struct userdata person = {0, "Jack"};

/* 定时器到期时被内核回调的函数。参数就是创建时传进来的 &person */
void timer_func(void *arg)
{
	if (NULL == arg)
		return;

	struct userdata *param = (struct userdata *)arg;

	param->counter++;
	printf("======> TIMEOUT: %s: %d\n", param->str, param->counter);
}

void user_task0(void){
	uart_puts("Task 0: Created!\n");

	/*
	 * ch10：创建三个软件定时器，到期时间是「当前 tick + 3 / + 5 / + 7」。
	 *
	 * 这里 _tick 还是 0 —— 因为 task0 是第一次 schedule() 就轮到的任务。
	 * 所以它们会正好在 tick 3 / 5 / 7 触发。
	 * 关键：创建的那一刻 _tick 是多少，决定了后面所有一切。
	 */
	struct timer *t1 = timer_create(timer_func, &person, 3);
	if (NULL == t1) {
		printf("timer_create() failed!\n");
	}
	struct timer *t2 = timer_create(timer_func, &person, 5);
	if (NULL == t2) {
		printf("timer_create() failed!\n");
	}
	struct timer *t3 = timer_create(timer_func, &person, 7);
	if (NULL == t3) {
		printf("timer_create() failed!\n");
	}

	while (1) {
		uart_puts("Task 0: Running...\n");
		task_delay(DELAY);
	}
}

/*
 * ----------------------------------------------------------------------
 * ch09 的临界区加锁测试（原 user_task0 的内容，本章先注释掉）
 *
 * 关掉的两个原因：
 *   1. 临界区里 spin_lock() 要关中断约 2.5 秒（5 次 task_delay(5000)）。
 *      这期间所有中断被屏蔽，硬件 tick 和软件定时器回调全被冻住，
 *      会严重干扰本章「定时器准不准」的观察。
 *   2. 本章只需要一个干净的任务框架来验证软件定时器。
 *
 * 想回看这段效果时：打开上面的 USE_LOCK，并把下面这段换回 user_task0。
 *
void user_task0(void){
	uart_puts("Task 0: Created!\n");
	while (1) {
#ifdef USE_LOCK
		spin_lock();
#endif
		uart_puts("Task 0: Begin...\n");
		for (int i = 0; i < 5; i++) {
			uart_puts("Task 0: Running...\n");
			task_delay(DELAY);
		}
		uart_puts("Task 0: End...\n");
#ifdef USE_LOCK
		spin_unlock();
#endif
	}
}
 * ----------------------------------------------------------------------
 */

void user_task1(void){
	uart_puts("Task 1: Created!\n");
	while (1) {
		uart_puts("Task 1: Running... \n");
		task_delay(DELAY);
	}
}


/* NOTICE: DON'T LOOP INFINITELY IN main() */
void os_main(void)
{
	task_create(user_task0);
	task_create(user_task1);
	// task_create(user_task2);   /* 本章只要两个任务，避免输出刷屏 */
}
