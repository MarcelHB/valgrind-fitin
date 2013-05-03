#include <stdio.h>

#include "../../../valgrind/include/valgrind/fi_client.h"

int main() {
    unsigned int a = 0;
    FITIN_MONITOR_VARIABLE(a);

    unsigned char *pa;
    pa = (unsigned char*)&a; 

    *pa = *pa + 1;
    
    printf("%u\n", a);

    return 0;
}
