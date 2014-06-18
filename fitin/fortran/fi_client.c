#include <string.h>
#include <fi_client.h> 

void fitin_monitor_memory_(void **ptr, size_t *size) {
    FITIN_MONITOR_MEMORY(*ptr, *size);
}

void fitin_monitor_field_f_(void **ptr, size_t *size, size_t *dims, size_t *dims_size) {
    FITIN_MONITOR_FIELD(*ptr, *size, *dims, *dims_size);
}

void fitin_unmonitor_memory_(void **ptr, size_t *size) {
    FITIN_UNMONITOR_MEMORY(*ptr, *size);
}
