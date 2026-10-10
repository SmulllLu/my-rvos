#include "os.h"

/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

static uint32_t _tick = 0;

void timer_load(int interval){

	int id = r_mhartid();

	*(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

void timer_init(){
    // 先装闹钟
    timer_load(TIMER_INTERVAL);
    // 开“定时器中断” 这一类
    w_mie(r_mie() | MIE_MTIE);
    // 总开关（plic_init 里已开过，这里冗余无害）
    w_mstatus(r_mstatus() | MSTATUS_MIE);
}

void timer_handler(){
    _tick++;

    printf("tick: %d\n", _tick);
    // 重新装填
    timer_load(TIMER_INTERVAL);
}
