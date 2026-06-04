#include "client_reader.h"

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#define EPOLL_WAIT_TIMEOUT_MS 500
#define MAX_FDS 2048
#define BUFFER_SIZE 16384 // 16 KiB

typedef struct {
    uint8_t *buffer;
    uint32_t bufLen;
    uint32_t startPos;
} ClientBuffer;

ClientBuffer **g_clientsBuffers;

ClientBuffer *getBuffer(const int32_t fd) {
    if (fd < 0) {
        return NULL;
    }

    if (g_clientsBuffers[fd] == NULL) {
        g_clientsBuffers[fd] = malloc(sizeof(ClientBuffer));

        uint8_t *buf = calloc(BUFFER_SIZE, sizeof(uint8_t));
        if (buf == NULL) {
            free(g_clientsBuffers[fd]);
            g_clientsBuffers[fd] = NULL;
            return NULL;
        }

        *g_clientsBuffers[fd] = (ClientBuffer){ .buffer   = buf,
                                                .bufLen   = BUFFER_SIZE,
                                                .startPos = 0 };
    }

    return g_clientsBuffers[fd];
}

void freeBuffer(const int32_t fd) {
    if (fd < 0) {
        return;
    }

    if (g_clientsBuffers[fd] == NULL) {
        return;
    }

    free(g_clientsBuffers[fd]->buffer);
    free(g_clientsBuffers[fd]);
    g_clientsBuffers[fd] = NULL;
}

int32_t readFromClient(const int32_t fd) {
    ClientBuffer *cb = getBuffer(fd);
    if (cb == NULL) {
        return -1;
    }

    return (int32_t)recv(fd, cb->buffer + cb->startPos,
                         cb->bufLen - cb->startPos, 0);
}

void *clientReaderRoutine(void *arg) {
    if (arg == NULL) {
        return NULL;
    }

    ClientReaderArg_t *crArg = (ClientReaderArg_t *)arg;
    g_clientsBuffers         = calloc(MAX_FDS, sizeof(ClientBuffer *));
    if (g_clientsBuffers == NULL) {
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

            int32_t read = readFromClient(activeFd);
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

            ClientBuffer *cb = getBuffer(activeFd);
            fprintf(stdout, "%s", cb->buffer);
            // TODO: parse here
            continue;

        closeClient:
            freeBuffer(activeFd);
            epoll_ctl(epoll, EPOLL_CTL_DEL, activeFd, NULL);
            close(activeFd);
        }
    }

    return NULL;

returnFailed:
    crArg->threadFailed = true;
    return NULL;
}
