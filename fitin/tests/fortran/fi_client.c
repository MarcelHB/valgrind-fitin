#include "../../../include/valgrind/fi_client.h"

void fitin_monitor_memory_(void **ptr, unsigned int size) {
    FITIN_MONITOR_MEMORY(*ptr, size);
}
