#include "os.h"

extern void switch_to(struct context *next);

#define MAX_TASKS 10
#define STACK_SIZE 1024

// 在标准的 RISC-V 调用约定中，栈指针 sp 始终是 16 字节对齐的。
uint8_t __attribute__((aligned(16))) task_stack[MAX_TASKS][STACK_SIZE];
struct  context ctx_tasks[MAX_TASKS];

static int _top = 0;
static int _current = -1;

void sched_init(){
    w_mscratch(0);
    // 启用机器模式软件中断
	w_mie(r_mie() | MIE_MSIE);
}

// 实现一个简单的循环 FIFO 调度器
void schedule(){
    if(_top <= 0){
        panic("Num of task should be greater than zero!");
		return;
    }

    _current = (_current + 1) % _top;
    struct context *next = &(ctx_tasks[_current]);
    switch_to(next);
}

// 创建一个task
int task_create(void (*start_routin)(void)){
    if(_top < MAX_TASKS){
		ctx_tasks[_top].sp = (reg_t) &task_stack[_top][STACK_SIZE];
		ctx_tasks[_top].pc = (reg_t) start_routin;
        _top++;
        return 0;
    }else{
        return -1;
    }
}

void task_yield(){
	int id = r_mhartid();
	*(uint32_t*)CLINT_MSIP(id) = 1;
}

// 一个非常粗糙的实现，只是为了消耗 CPU
void task_delay(volatile int count){
	count *= 50000;
	while (count--);
}