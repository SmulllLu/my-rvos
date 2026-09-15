extern void uart_init(void);
extern void uart_puts(char *s);
extern char uart_getc(void);
extern int uart_putc(char ch);

void start_kernel(void){

    uart_init();
	uart_puts("Hello, RVOS!\n");

    while (1) {
        char c = uart_getc();        // 阻塞等一个键

        if (c == '\r' || c == '\n') // 按了回车？
            uart_puts("\r\n");      // → 输出 回到行首+下移一行
        else
            uart_putc(c);           // 普通字符 → 原样发回（回显）
    }
}