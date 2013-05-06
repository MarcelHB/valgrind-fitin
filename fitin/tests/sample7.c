#include <stdio.h>
#include <sys/time.h>

#include "../../include/valgrind/fi_client.h"

int main() {
    struct timeval tv, tv_copy;
    unsigned char byte, byte_copy, diff;
    FITIN_MONITOR_MEMORY(((unsigned char*) &tv)+7, 1);

    gettimeofday(&tv, NULL);
    tv_copy = tv;
    settimeofday(&tv, NULL);

    byte = *(((unsigned char*) &tv)+7); 
    byte_copy = *(((unsigned char*) &tv_copy)+7); 
    diff = byte ^ byte_copy;

    printf("%u\n", diff);

    return 0;
}
