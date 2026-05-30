#ifndef NAZARK_CLIENT_READER_H
#define NAZARK_CLIENT_READER_H

#include "concurrent_linked_list.h"

#include <sys/epoll.h>
#include <stdbool.h>

typedef struct ClientReaderArg {
    ConcurrentLinkedList_t *clients;
    struct epoll_event *event;
    bool (*isApplicationAlive)(void);
} ClientReaderArg_t;

/// argument of type ClientReaderArg_t *
void* clientReaderRoutine(void *arg);

#endif // NAZARK_CLIENT_READER_H
