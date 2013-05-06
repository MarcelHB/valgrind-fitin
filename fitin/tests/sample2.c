#include <stdio.h>

#include "../../include/valgrind/fi_client.h"

int main() {
    int a = 1;
    FITIN_MONITOR_VARIABLE(a);

    if(a) {
        printf("a GE 1\n");
    } else {
        printf("a LT 1\n");
    }

    return 0;
}
