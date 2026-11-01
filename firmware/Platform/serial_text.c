#include "serial_text.h"
#include <stdarg.h>
#include <stdint.h>
extern int __io_putchar(int);
static void number(uint32_t n,unsigned base) {
    char digits[32]; unsigned count=0;
    do { digits[count++]="0123456789abcdef"[n%base]; n/=base; } while(n);
    while(count) __io_putchar(digits[--count]);
}
/* Deliberately limited to the firmware's integer/string telemetry format. */
void serial_text(const char *f,...) {
    va_list ap; va_start(ap,f);
    while(*f) {
        if(*f++!='%') { __io_putchar(f[-1]); continue; }
        int wide=0; if(*f=='l') { wide=1; f++; }
        char type=*f++;
        if(type=='s') { const char *s=va_arg(ap,const char *); while(*s) __io_putchar(*s++); }
        else if(type=='d') {
            int32_t n=wide ? va_arg(ap,long) : va_arg(ap,int);
            if(n<0) { __io_putchar('-'); number(0u-(uint32_t)n,10); }
            else number((uint32_t)n,10);
        } else if(type=='u'||type=='x') number(wide ? va_arg(ap,unsigned long) : va_arg(ap,unsigned),type=='x'?16:10);
        else if(type=='c') __io_putchar(va_arg(ap,int));
        else __io_putchar(type);
    }
    va_end(ap);
}
