#include "os.h"

#define HEADER_SIZE 16
#define ALIGN(x) (((x)+7) & ~7)
#define PAGE_SIZE 4096   /* page.c 里也有一个，但宏不跨文件，这边自己带一份 */

/* ---- 私有数据结构 ---- */
struct block {
    size_t size;           // 本块总大小（含块头）
    struct block *next;    // 下一个空闲块（仅空闲时有效）
    int flag;              // 0=空闲 1=占用
    char pad[4];           // 纯填充，sizeof == 16
};

static struct block *free_list;    // 空闲链表头，按地址升序

/* ---- 私有工具函数 ---- */

// 向 page_alloc 要新页，包成一个空闲块头插进名单；只进货，不切割
static void *_alloc_from_page(size_t need){
    int npages = (need + PAGE_SIZE - 1) / PAGE_SIZE;   // 向上取整要几页

    void *p = page_alloc(npages);
    if (p == NULL)
        return NULL;

    struct block *b = (struct block *)p;
    b->size = PAGE_SIZE * npages;   // 整片都归这个块管
    b->flag = 0;

    b->next = free_list;            // 头插法
    free_list = b;

    return b;
}

// 从 b 前部切走 need 字节，剩余 r 顶替 b 在名单上的位置
static void _split(struct block *prev, struct block *b, size_t need){
    struct block *r = (struct block *)((char *)b + need);
    r->size = b->size - need;   // 先算这笔账，再缩 b
    r->flag = 0;
    r->next = b->next;

    if (prev == NULL)
        free_list = r;
    else
        prev->next = r;

    b->size = need;
}

// 把物理相邻的空闲块两两吸收（要求名单按地址升序）
static void _merge(void){
    struct block *b = free_list;

    while (b != NULL && b->next != NULL) {
        // b 的物理尽头正好是下家 → 相邻 → 吃掉
        if ((char *)b + b->size == (char *)b->next) {
            b->size += b->next->size;
            b->next = b->next->next;
            // 不前进，长大的 b 可能还够得着下一个
        } else {
            b = b->next;
        }
    }
}

/* ---- 对外接口 ---- */

// 分配 size 字节，返回 8 对齐的用户指针（块头后）；失败返回 NULL
void *malloc(size_t size) {
    if (size == 0)
        return NULL;

    // 计算出要分配的大小
    size_t need = ALIGN(size + HEADER_SIZE);

    /* first-fit：prev/cur 并排走，找第一个够大的空闲块 */
    struct block *prev = NULL;
    struct block *cur = free_list;
    while (cur != NULL) {
        if (cur->size >= need)
            break;
        prev = cur;
        cur = cur->next;
    }

    // 名单里没有 → 进新货（自动挂上名单）
    if (cur == NULL) {
        prev = NULL;   // 新块头插队首，搜索留下的 prev 已过时，必须重置
        cur = _alloc_from_page(need);
        if (cur == NULL)
            return NULL;
    }

    // 剩余够立一个新头就切，否则整块给
    if (cur->size - need >= HEADER_SIZE) {
        _split(prev, cur, need);
    } else if (prev == NULL) {// 舍掉cur
        free_list = cur->next;
    } else {
        prev->next = cur->next;
    }

    cur->flag = 1;
    return (void *)(cur + 1);   // 跳过块头才是给用户的
}

// 归还内存；ptr 必须是 malloc 原样返回值，free(NULL) 无害
void free(void *ptr) {
    if (ptr == NULL)
        return;

    struct block *b = (struct block *)((char *)ptr - HEADER_SIZE);
    b->flag = 0;

    // 按地址找座位，保持名单升序
    struct block *prev = NULL;
    struct block *cur = free_list;
    while (cur != NULL && cur < b) {
        prev = cur;
        cur = cur->next;
    }

    if (prev == NULL)
        free_list = b;
    else
        prev->next = b;
    b->next = cur;

    _merge();   // 顺手把相邻的空闲块吃干净
}

/* ---- 测试 ---- */
void malloc_test(void) {
    printf("----- malloc test -----\n");

    /* 有 _split 后，连续 malloc 应只差 0x20（一 tone 32 字节） */
    void *p = malloc(10);
    printf("p  = %p\n", p);

    void *q = malloc(10);
    printf("q  = %p   (应与 p 差 0x20)\n", q);

    void *big = malloc(5000);
    printf("big = %p (5000 进的块，两页)\n", big);

    /* 碎片合并验证：free 掉 a、b2 后合并成 64，
       malloc(48) 的 need 正好 64，应复用 a 的地址 */
    void *a  = malloc(10);  printf("a  = %p\n", a);
    void *b2 = malloc(10);  printf("b2 = %p\n", b2);
    void *c  = malloc(10);  printf("c  = %p\n", c);
    free(a);
    free(b2);
    void *d = malloc(48);   printf("d  = %p  (应 == a)\n", d);
}
