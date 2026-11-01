#include "serial_text.h"
#include <assert.h>
#include <string.h>
#include <limits.h>
static char output[256]; static unsigned used;
int __io_putchar(int c) { assert(used+1<sizeof(output)); output[used++]=(char)c; output[used]=0; return c; }
int main(void) {
    serial_text("%s %u %d %lu %ld %x %c %%","SunAlign",0u,INT_MIN,4294967295ul,-2147483647l,0x68u,'Z');
    assert(!strcmp(output,"SunAlign 0 -2147483648 4294967295 -2147483647 68 Z %"));
    return 0;
}
