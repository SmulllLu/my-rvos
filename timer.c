#include "os.h"
extern void schedule(void); 
/* interval ~= 1s */
#define TIMER_INTERVAL CLINT_TIMEBASE_FREQ

// 设置定时器任务总量
#define MAX_TIMER 10
static struct timer timer_list[MAX_TIMER];

static uint32_t _tick = 0;

// 到点叫人
static inline void timer_check(){
    struct timer *t = &(timer_list[0]);
	
    for (int i = 0; i < MAX_TIMER; i++) {
		if (NULL != t->func) {
			if (_tick >= t->timeout_tick) {
				t->func(t->arg);

				/* once time, just delete it after timeout */
				t->func = NULL;
				t->arg = NULL;

				break;
			}
		}
		t++;
	}

}

void timer_load(int interval){

	int id = r_mhartid();

	*(uint64_t*)CLINT_MTIMECMP(id) = *(uint64_t*)CLINT_MTIME + interval;
}

void timer_init(){
	struct timer *t = &(timer_list[0]);
	for (int i = 0; i < MAX_TIMER; i++) {
		t->func = NULL;    /* use .func to flag if the item is used */
		t->arg = NULL;
		t++;
	}

    // 先装闹钟
    timer_load(TIMER_INTERVAL);
    // 开“定时器中断” 这一类
    w_mie(r_mie() | MIE_MTIE);
}

void timer_handler(){
    _tick++;

    printf("tick: %d\n", _tick);

    timer_check();

    // 重新装填
    timer_load(TIMER_INTERVAL);
    schedule(); 
}

struct timer *timer_create(void (*handler)(void *arg), void *arg, uint32_t timeout){
	if (NULL == handler || 0 == timeout) {
		return NULL;
	}
    
    // 上锁
    spin_lock();
    
    struct timer *t = &(timer_list[0]);
	for (int i = 0; i < MAX_TIMER; i++) {
		if (NULL == t->func) {
			break;
		}
		t++;
	}
	if (NULL != t->func) {
		spin_unlock();
		return NULL;
	}

	t->func = handler;
	t->arg = arg;
	t->timeout_tick = _tick + timeout;

    // 解锁
	spin_unlock();

    return t;
}

void timer_delete(struct timer *timer){

    spin_lock();

	struct timer *t = &(timer_list[0]);
	for (int i = 0; i < MAX_TIMER; i++) {
		if (t == timer) {
			t->func = NULL;
			t->arg = NULL;
			break;
		}
		t++;
	}

	spin_unlock();
    
}