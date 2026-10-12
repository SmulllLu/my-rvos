#include "os.h"

/*
 * 把格式串 s 按 vl 里的参数展开，写进 out（最多 n 字节，含结尾 '\0'）
 * out == NULL 时是"干跑"模式：一个字节都不写，只数长度
 * 返回值：完整输出应有的字符数（不含结尾 '\0'）
 */
static int _vsnprintf(char * out, size_t n, const char* s, va_list vl)
{
	int format = 0;   // 是否刚读到 '%'，正在解析格式符
	int longarg = 0;  // 是否见过 'l' 修饰符（比如 %lx）
	size_t pos = 0;   // 已产出的字符数；即使没真写，也照样累加
	for (; *s; s++) {
		if (format) {
			switch(*s) {
			case 'l': {       // 长度修饰符：只记标记，继续看后面的转换符
				longarg = 1;
				break;
			}
			case 'p': {       // 指针：先输出 "0x" 前缀
				longarg = 1;
				if (out && pos < n) {
					out[pos] = '0';
				}
				pos++;
				if (out && pos < n) {
					out[pos] = 'x';
				}
				pos++;
				// 注意：故意不写 break，直接落进 'x'，把地址按十六进制打出来
			}
			case 'x': {       // 十六进制
				long num = longarg ? va_arg(vl, long) : va_arg(vl, int);
				// 一个 hex 数字占 4 bit：32 位就是 8 位数字（下标 7..0）
				int hexdigits = 2*(longarg ? sizeof(long) : sizeof(int))-1;
				for(int i = hexdigits; i >= 0; i--) {
					int d = (num >> (4*i)) & 0xF;   // 从高位往低位取
					if (out && pos < n) {
						out[pos] = (d < 10 ? '0'+d : 'a'+d-10);
					}
					pos++;
				}
				longarg = 0;
				format = 0;
				break;
			}
			case 'd': {       // 十进制
				long num = longarg ? va_arg(vl, long) : va_arg(vl, int);
				if (num < 0) {    // 负数：先输出 '-'，再按正数处理
					num = -num;
					if (out && pos < n) {
						out[pos] = '-';
					}
					pos++;
				}
				long digits = 1;  // 先数出总共几位
				for (long nn = num; nn /= 10; digits++);
				// 取余先得低位数字，所以从右往左填
				for (int i = digits-1; i >= 0; i--) {
					if (out && pos + i < n) {
						out[pos + i] = '0' + (num % 10);
					}
					num /= 10;
				}
				pos += digits;
				longarg = 0;
				format = 0;
				break;
			}
			case 's': {       // 字符串
				const char* s2 = va_arg(vl, const char*);
				while (*s2) {
					if (out && pos < n) {
						out[pos] = *s2;
					}
					pos++;
					s2++;
				}
				longarg = 0;
				format = 0;
				break;
			}
			case 'c': {       // 单个字符（char 会被自动提升成 int 传进来）
				if (out && pos < n) {
					out[pos] = (char)va_arg(vl,int);
				}
				pos++;
				longarg = 0;
				format = 0;
				break;
			}
			default:          // 不认识的格式符：忽略
				break;
			}
		} else if (*s == '%') {   // 见到 '%'，下一个字符按格式符解析
			format = 1;
		} else {                  // 普通字符，原样抄过去
			if (out && pos < n) {
				out[pos] = *s;
			}
			pos++;
		}
    	}
	if (out && pos < n) {         // 正常结束：补上 '\0'
		out[pos] = 0;
	} else if (out && n) {        // 被 n 截断了：也要保证缓冲区以 '\0' 收尾
		out[n-1] = 0;
	}
	return pos;                    // 返回"完整输出"的长度（可能大于 n）
}

static char out_buf[1000]; // 输出缓冲区：_vprintf() 先把结果格式化到这里再发串口

// 两遍扫描：第一遍干跑数长度并检查会不会撑爆 out_buf，第二遍才真正写入
static int _vprintf(const char*s, va_list vl){
    int res = _vsnprintf(NULL, -1, s, vl);   // 干跑：只数不写
    if(res+1 >= sizeof(out_buf)){            // +1 是给结尾 '\0' 留的；装不下就报错停机
        uart_puts("error: output string size overflow\n");
        while(1){}
    }
    _vsnprintf(out_buf, res+1, s, vl);       // 真写：缓冲区正好够大，一次写完
    uart_puts(out_buf);
    return res;
}

// printf 本体：把可变参数打包成 vl，转交给 _vprintf
int printf(const char*s, ...){
    int res=0;
    va_list vl;
    va_start(vl, s);      // 游标定位到第一个可变参数
    res = _vprintf(s, vl);
    va_end(vl);           // 与 va_start 配对收尾
    return res;
}

void panic(char *s){
    printf("panic: ");    // 打印错误信息后死循环——裸机上这就是"崩溃停机"
    printf(s);
    printf("\n");
    while(1){};
}
