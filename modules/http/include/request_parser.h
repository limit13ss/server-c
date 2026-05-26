#ifndef NAZARK_REQUEST_PARSER_H
#define NAZARK_REQUEST_PARSER_H

#include <sys/epoll.h>
#include <unistd.h>

typedef struct ParseRequestArg {
    int32_t clientFd;
    struct epoll_event event;
} ParseRequestArg_t;

/// argument must be of type ParseRequestArg_t *
void parseClientRequest(void *arg);

#endif // NAZARK_REQUEST_PARSER_H
