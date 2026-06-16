#include "client_reader.h"
#include "client_parser.h"

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

ParserContext **g_parserContexts;

/// ===================== ============ =====================
/// ===================== CB FUNCTIONS =====================
/// ===================== ============ =====================

ParserContext *getContext(const int32_t fd) {
    if (fd < 0) {
        return NULL;
    }

    if (g_parserContexts[fd] == NULL) {
        g_parserContexts[fd] = ParserContext_Init();
    }

    return g_parserContexts[fd];
}

void freeContext(const int32_t fd) {
    if (fd < 0) {
        return;
    }

    if (g_parserContexts[fd] == NULL) {
        return;
    }

    ParserContext_Free(g_parserContexts[fd]);
    g_parserContexts[fd] = NULL;
}

int32_t readHeaders(const int32_t fd) {
    ParserContext *ctx = getContext(fd);
    if (ctx == NULL) {
        return -1;
    }

    NKBuffer *buf = ParserContext_GetBuffer(ctx);
    if (buf->length >= buf->capacity) {
        return -1;
    }

    int32_t read = (int32_t)recv(fd, buf->values + buf->length,
                                 buf->capacity - buf->length, 0);

    if (read > 0) {
        buf->length += (uint32_t)read;
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
    g_parserContexts         = calloc(MAX_FDS, sizeof(NKBuffer *));
    if (g_parserContexts == NULL) {
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

            ParsingState currentState =
                Parser_TryParseHeaders(getContext(activeFd));

            if (currentState) {
                // TODO: act based on ctx->state
            }

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
