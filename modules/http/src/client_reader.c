#include "client_reader.h"

#include <errno.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

void *clientReaderRoutine(void *arg) {
    if (arg == NULL) {
        return NULL;
    }

    ClientReaderArg_t *crArg = (ClientReaderArg_t *)arg;

    int32_t ready = 0;
    int32_t epoll = crArg->epoll;
    struct epoll_event events[MAX_SIMULTANEOUS_READ_EVENTS];

    while (crArg->isApplicationAlive()) {
        ready = epoll_wait(epoll, events, MAX_SIMULTANEOUS_READ_EVENTS, -1);

        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            printf(stderr,
                   "[ERR] Client reader loop error during epoll_wait\n");
            crArg->threadFailed = true;
            return NULL;
        }

        for (int32_t i; i < ready; ++i) {
            int32_t clientFd = events[i].data.fd;

            // TODO: read here

            close(clientFd);
            epoll_ctl(epoll, EPOLL_CTL_DEL, clientFd, NULL);
        }
    }

    return NULL;
}
