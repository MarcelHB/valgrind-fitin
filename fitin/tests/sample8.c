#include <stdio.h>

#include "../../../valgrind/include/valgrind/fi_client.h"

int main() {
    unsigned int a = 1, ref = 0, flip = 0;
    FITIN_MONITOR_VARIABLE(a);

    asm("movl $0, %eax");
    asm("cpuid");
    asm("movl %%eax, %0":"=m"(ref)::);

    asm("movl %0, %%eax"::"m"(a):"%eax");
    asm("cpuid");
    asm("movl %%eax, %0":"=m"(flip)::);

    printf("%u\n", flip == ref);

    return 0;
}
