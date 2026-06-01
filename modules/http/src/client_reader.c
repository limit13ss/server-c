#include "client_reader.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <unistd.h>

#define EPOLL_WAIT_TIMEOUT_MS 500

void *clientReaderRoutine(void *arg) {
    if (arg == NULL) {
        return NULL;
    }

    ClientReaderArg_t *crArg = (ClientReaderArg_t *)arg;

    int32_t ready = 0;
    int32_t epoll = crArg->epoll;
    struct epoll_event events[MAX_SIMULTANEOUS_READ_EVENTS];

    while (crArg->isServerAlive()) {
        ready = epoll_wait(epoll, events, MAX_SIMULTANEOUS_READ_EVENTS, -1);

        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            fprintf(stderr,
                    "[ERR] Client reader loop error during epoll_wait\n");
            crArg->threadFailed = true;
            return NULL;
        }

        for (int32_t i = 0; i < ready; ++i) {
            int32_t activeFd = events[i].data.fd;

            if (activeFd == crArg->stopEventFd) {
                fprintf(stdout, "[INF] Reader thread received stop signal.\n");
                return NULL;
            }

            // TODO: read here

            epoll_ctl(epoll, EPOLL_CTL_DEL, activeFd, NULL);
            close(activeFd);
        }
    }

    return NULL;
}
