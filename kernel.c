#include "os.h"

extern void uart_init(void);
extern void page_init(void);
extern void page_test(void);
extern void malloc_test(void);

void start_kernel(void){

	uart_init();
	uart_puts("Hello, RVOS!\n");

	page_init();
    page_test();
    malloc_test();

    while (1) {
        char c = uart_getc();        // 阻塞等一个键

        if (c == '\r' || c == '\n') // 按了回车？
            uart_puts("\r\n");      // → 输出 回到行首+下移一行
        else
            uart_putc(c);           // 普通字符 → 原样发回（回显）
    }
}