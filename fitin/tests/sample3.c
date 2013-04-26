#include <stdio.h>

#include "../../../valgrind/include/valgrind/fi_client.h"

int main() {
    int a = 1, b = 0;
    FITIN_MONITOR_VARIABLE(a);

    b += a;
    b += a;
    b += a;

    printf("%d\n", a);
    return 0;
}
