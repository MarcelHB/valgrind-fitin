#include <fi_client.h> 

void fitin_monitor_memory_(void **ptr, unsigned int *size) {
    FITIN_MONITOR_MEMORY(*ptr, *size);
}

void fitin_unmonitor_memory_(void **ptr, unsigned int *size) {
    FITIN_UNMONITOR_MEMORY(*ptr, *size);
}
