#include "client_reader.h"
#include "client.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define EPOLL_WAIT_TIMEOUT_MS 500
#define MAX_FDS 2048

/// ===================== ======= =====================
/// ===================== GLOBALS =====================
/// ===================== ======= =====================

ClientContext **g_clientContexts;

/// ===================== ============ =====================
/// ===================== CB FUNCTIONS =====================
/// ===================== ============ =====================

ClientContext *getContext(const int32_t fd) {
    if (fd < 0) {
        return NULL;
    }

    if (g_clientContexts[fd] == NULL) {
        g_clientContexts[fd] = Client_InitContext();
    }

    return g_clientContexts[fd];
}

void freeContext(const int32_t fd) {
    if (fd < 0) {
        return;
    }

    if (g_clientContexts[fd] == NULL) {
        return;
    }

    Client_FreeContext(g_clientContexts[fd]);
    g_clientContexts[fd] = NULL;
}

int32_t readHeaders(const int32_t fd) {
    ClientContext *ctx = getContext(fd);
    if (ctx == NULL) {
        return -1;
    }

    ClientBuffer *cb = Client_GetBuffer(ctx);
    if (cb->length >= cb->capacity) {
        return -1;
    }

    int32_t read = (int32_t)recv(fd, cb->buffer + cb->length,
                                 cb->capacity - cb->length, 0);

    if (read > 0) {
        cb->length += (uint32_t)read;
    }
    return read;
}

/// ===================== ============ =====================
/// ===================== MAIN ROUTINE =====================
/// ===================== ============ =====================

void *clientReaderRoutine(void *arg) {
    if (arg == NULL) {
        return NULL;
    }

    ClientReaderArg_t *crArg = (ClientReaderArg_t *)arg;
    g_clientContexts         = calloc(MAX_FDS, sizeof(ClientBuffer *));
    if (g_clientContexts == NULL) {
        fprintf(
            stderr,
            "[ERR] Client reader thread error - malloc(g_clientsBuffers).\n");
        goto returnFailed;
    }

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
                    "[ERR] Client reader loop error during epoll_wait.\n");
            goto returnFailed;
        }

        for (int32_t i = 0; i < ready; ++i) {
            int32_t activeFd = events[i].data.fd;

            if (activeFd == crArg->stopEventFd) {
                fprintf(stdout,
                        "[INF] Client reader thread received stop signal.\n");
                return NULL;
            }

            int32_t read = readHeaders(activeFd);
            if (read < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                    continue;
                }

                fprintf(stderr,
                        "[ERR] Client reader thread error - readFromClient\n");
                goto closeClient;
            } else if (read == 0) {
                fprintf(stdout,
                        "[INF] Client reader, peer closed connection fd=%d\n",
                        activeFd);
                goto closeClient;
            }

            Request_TryParseHeaders(getContext(activeFd));

            // TODO: parse here
            continue;

        closeClient:
            freeContext(activeFd);
            epoll_ctl(epoll, EPOLL_CTL_DEL, activeFd, NULL);
            close(activeFd);
        }
    }

    return NULL;

returnFailed:
    crArg->threadFailed = true;
    return NULL;
}
