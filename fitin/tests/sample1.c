#include <stdio.h>

#include "../../include/valgrind/fi_client.h"

int main() {
    int a = 1;
    FITIN_MONITOR_VARIABLE(a);

    a++;
    a++;
    a++;

    printf("%u\n", a);
    return 0;
}
