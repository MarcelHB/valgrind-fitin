#include <stdio.h>

#include "../../../valgrind/include/valgrind/fi_client.h"

/** This test won't flip anything for the actual logic, but at least
 *  it ensure the register tracking feature is working and that
 *  FITIn-reg can deal with smaller loads, as Valgrind generates
 *  according code for some reason.
 */
int main() {
    char a = 1, b = 0, c = 1;
    FITIN_MONITOR_VARIABLE(a);

    asm("movzxb %0, %%eax"::"m"(a):"%eax");
    asm("inc %eax");
    asm("movl %%eax, %0":"=m"(b)::);
    c += b;

    printf("%d\n", b);

    return 0;
}
