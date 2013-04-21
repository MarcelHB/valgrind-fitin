#include <stdio.h>

#include "../../../valgrind/include/valgrind/fi_client.h"

int main() {
    int a = 1;
    FITIN_MONITOR_VARIABLE(a);

    a++;
    a++;
    a++;

    printf("%u\n", a);
    return 0;
}
