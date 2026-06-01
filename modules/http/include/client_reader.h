#ifndef NAZARK_CLIENT_READER_H
#define NAZARK_CLIENT_READER_H

#include <stdbool.h>
#include <sys/epoll.h>

#define MAX_SIMULTANEOUS_READ_EVENTS 10

typedef struct ClientReaderArg {
    int32_t epoll;
    bool (*isApplicationAlive)(void);
    bool threadFailed;
} ClientReaderArg_t;

/// argument of type ClientReaderArg_t *
void *clientReaderRoutine(void *arg);

#endif // NAZARK_CLIENT_READER_H
