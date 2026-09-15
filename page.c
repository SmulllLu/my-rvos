#include "os.h"

/*
 * Following global vars are defined in mem.S
 */
extern ptr_t TEXT_START;
extern ptr_t TEXT_END;
extern ptr_t DATA_START;
extern ptr_t DATA_END;
extern ptr_t RODATA_START;
extern ptr_t RODATA_END;
extern ptr_t BSS_START;
extern ptr_t BSS_END;
extern ptr_t HEAP_START;
extern ptr_t HEAP_SIZE;

static ptr_t _alloc_start = 0;// 动态内存的起始地址
static ptr_t _alloc_end = 0;// 动态内存的末尾地址
static uint32_t _num_pages = 0;

#define PAGE_SIZE 4096
#define PAGE_ORDER 12

#define PAGE_TAKEN (uint8_t)(1 << 0)
#define PAGE_LAST  (uint8_t)(1 << 1)

struct Page {
	uint8_t flags;
};

static inline void _clear(struct Page *page)
{
	page->flags = 0;
}

static inline int _is_free(struct Page *page)
{
	if (page->flags & PAGE_TAKEN) {
		return 0;
	} else {
		return 1;
	}
}

static inline void _set_flag(struct Page *page, uint8_t flags)
{
	page->flags |= flags;
}

static inline int _is_last(struct Page *page)
{
	if (page->flags & PAGE_LAST) {
		return 1;
	} else {
		return 0;
	}
}

static inline ptr_t _align_page(ptr_t address)
{
	ptr_t order = (1 << PAGE_ORDER) - 1;
	return (address + order) & (~order);
}

void page_init(){
    // 计算堆的开始对齐的区域
    ptr_t _heap_start_aligned = _align_page(HEAP_START);

    // 计算与保留区域的页的数量 -> RAM长度/页大小^2
    uint32_t num_reserved_pages = LENGTH_RAM / (PAGE_SIZE * PAGE_SIZE);

    // 计算舍去保留页之后的总页数 -> （堆的大小 - （堆的开始对齐区域 - 堆的开始区域））/ 页的大小 - 保留的页的大小
    _num_pages = (HEAP_SIZE - (_heap_start_aligned - HEAP_START))/PAGE_SIZE - num_reserved_pages;

    printf("HEAP_START = %p(aligned to %p), HEAP_SIZE=0x%lx,\n"
            "num of reserved pages = %d, num of pages to be allocated for heap = %d\n",
            HEAP_START,_heap_start_aligned,HEAP_SIZE,num_reserved_pages,_num_pages);

    struct Page *page = (struct Page *)HEAP_START;

    for (int i=0;i<_num_pages;i++){
        _clear(page);
        page++;
    }

    _alloc_start = _heap_start_aligned + num_reserved_pages * PAGE_SIZE;
    _alloc_end = _alloc_start + _num_pages * PAGE_SIZE;

    printf("TEXT:   %p -> %p\n",TEXT_START,TEXT_END);
    printf("DATA:   %p -> %p\n",DATA_START,DATA_END);
    printf("RODATA: %p -> %p\n",RODATA_START,RODATA_END);
    printf("BSS:    %p -> %p\n",BSS_START,BSS_END);
    printf("HEAP:   %p -> %p\n",_alloc_start,_alloc_end);

}

// 分配页
void *page_alloc(int npages){
    int found =0;
    struct Page *page_i = (struct Page *)HEAP_START;

    for(int i=0;i<=(_num_pages -npages);i++){
        // 找到了一个空的页面
        if(_is_free(page_i)){
            found =1;

            struct Page *page_j = page_i +1;
            for(int j=i+1 ; j<(i + npages) ; j++){

                if(!_is_free(page_j)){
                    found=0;
                    break;
                }
                page_j++;
            }
            // 如果found不为0,则说明已经找到了 npages个连续的页可以分配
            if(found){
                struct Page *page_k = page_i;
                for(int k = i ; k < (i + npages); k++){
                    _set_flag(page_k, PAGE_TAKEN);
                    page_k++;
                }
                page_k--;
                _set_flag(page_k, PAGE_LAST);
                return (void *)(_alloc_start + i *PAGE_SIZE);
            }
        }
        page_i++;

    }
    return NULL;
}

//释放页
void page_free(void *p){
    if(!p||((ptr_t)p > _alloc_end)){
        return ;
    }

    struct Page *page = (struct Page *)HEAP_START;
    page += ((ptr_t)p - _alloc_start)/PAGE_SIZE;

    while(!_is_free(page)){
        if(_is_last(page)){
            _clear(page);
            break;
        }
        _clear(page);
        page++;
    }
}