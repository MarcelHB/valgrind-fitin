#include <stdio.h>

#include "../../../valgrind/include/valgrind/fi_client.h"

int main() {
    int a = 1, b = 0;
    FITIN_MONITOR_VARIABLE(a);

    a++;

    b = a;

    printf("%d %d\n", a, b);

    return 0;
}
