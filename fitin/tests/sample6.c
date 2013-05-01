#include <stdio.h>

#include "../../../valgrind/include/valgrind/fi_client.h"

int just_do_it() {
    int a = 1;
    FITIN_MONITOR_VARIABLE(a);
    a++;

    return a;
}

int main() {
    int i = 0, result = 0;

    for(; i < 10; ++i) {
        result = just_do_it();
    }
    
    printf("%d\n", result);

    return 0;
}
