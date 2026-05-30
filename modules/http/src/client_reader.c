#include "client_reader.h"

#include <stdlib.h>
#include <sys/epoll.h>

void* clientReaderRoutine(void *arg) {
    if (arg == NULL) {
        return NULL;
    }

    ClientReaderArg_t *crArg = (ClientReaderArg_t *)arg;

    (void)crArg;
    return NULL;
}
