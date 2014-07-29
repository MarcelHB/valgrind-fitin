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

void fitin_breakpoint5_f_(size_t *a, size_t *b, size_t *c, size_t *d, size_t *e) {
    FITIN_BREAKPOINT5(*a, *b, *c, *d, *e);
}
