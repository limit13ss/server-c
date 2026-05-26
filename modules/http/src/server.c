#include "server.h"
#include "common.h"
#include "request_parser.h"
#include "socket_option.h"
#include "thread_pool.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define WORKERS_COUNT_CLIENT_PARSERS 2

volatile sig_atomic_t g_isApplicationAlive = 1;

/// -------------------------------------
/// General operations
/// -------------------------------------

void handleProcSignal(int sig) {
    (void)sig;
    g_isApplicationAlive = 0;
}

/// -------------------------------------
/// Socket related operations
/// -------------------------------------

int32_t initSocket(uint8_t connectionQueueSize) {
    int32_t socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
        fprintf(stderr, "[ERR] Socket creation failed: %s\n", strerror(errno));
        return socketFd;
    }

    socket_option_reuseAddr(socketFd, true);

    struct sockaddr_in addr =
        (struct sockaddr_in){ .sin_family      = AF_INET,
                              .sin_port        = htons(SERVER_PORT),
                              .sin_addr.s_addr = INADDR_ANY };

    int32_t rCode = bind(socketFd, (struct sockaddr *)&addr, sizeof(addr));
    if (rCode == -1) {
        fprintf(stderr, "[ERR] Socket binding failed: %s\n", strerror(errno));
        close(socketFd);
        return rCode;
    }
    rCode = listen(socketFd, connectionQueueSize);
    if (rCode == -1) {
        fprintf(stderr, "[ERR] Connection listening failed: %s\n",
                strerror(errno));
        close(socketFd);
        return rCode;
    }

    return socketFd;
}

int32_t initMainListener(void) {
    int32_t socketFd = initSocket(SERVER_REQUESTS_QUEUE_SIZE);
    if (socketFd == -1) {
        return socketFd;
    }

    return socketFd;
}

/// -------------------------------------
/// Main connection operations
/// -------------------------------------

int32_t server_mainLoop(void) {
    struct sigaction sa = { .sa_handler = handleProcSignal,
                            .sa_flags   = SA_RESTART | SA_SIGINFO };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGKILL, &sa, NULL);

    int32_t mainSocketFd = initMainListener();
    if (mainSocketFd < 0) {
        return ERROR_SERVER_INVALID_SOCKET_DESCRIPTOR;
    }
    socket_option_setNonBlocking(mainSocketFd);

    ThreadPool_t *threadPool = ThreadPool_Create(WORKERS_COUNT_CLIENT_PARSERS);
    if (threadPool == NULL) {
        printf(stderr, "[ERR] Initialization error: ThreadPool_Create\n");
        goto errorCloseMainSocket;
    }

    int32_t epollFd = epoll_create1(0);
    if (epollFd < 0) {
        printf(stderr, "[ERR] Initialization error: epoll_create1\n");
        goto errorCloseThreadPool;
    }

    struct epoll_event ev = { .events = EPOLLIN, .data.fd = mainSocketFd };
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, mainSocketFd, &ev) < 0) {
        printf(stderr, "[ERR] Initialization error: epoll_ctl\n");
        goto errorCloseEpoll;
    }

    fprintf(stdout,
            "[INF] Server is initalized successfully.\n"
            "Listening on port :%d (Ctrl+C to stop)...\n",
            SERVER_PORT);

    int32_t ready = 0;
    struct epoll_event events[MAX_EVENTS];

    while (g_isApplicationAlive) {
        ready = epoll_wait(epollFd, events, MAX_EVENTS, -1);

        if (ready < 0) {
            if (errno == EINTR) {
                continue;
            }
            printf(stderr, "[ERR] Main loop error during \"epoll_wait\"\n");
            goto errorCloseEpoll;
        }

        for (int32_t i; i < ready; i++) {
            if (events[i].data.fd != mainSocketFd) {
                continue;
            }

            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);
            int32_t clientFd    = accept(
                mainSocketFd, (struct sockaddr *)(&clientAddr), &clientLen);

            if (clientFd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    continue;
                }
                printf(stderr, "[ERR] Main loop error during \"accept\"\n");
                break;
            }

            printf(stdout, "[INF] Accepted connection (fd=%d)\n", clientFd);

            switch (socket_option_setNonBlocking(clientFd)) {
            case -1:
                printf(stderr, "[ERR] \"setNonBlocking\": F_GETFL\n");
                close(clientFd);
                continue;
            case -2:
                printf(stderr, "[ERR] \"setNonBlocking\": F_SETFL\n");
                close(clientFd);
                continue;
            }

            ev.events  = EPOLLIN | EPOLLET;
            ev.data.fd = clientFd;
            if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
                fprintf(stderr, "[ERR] \"epoll_ctl\": clientFd (%d)\n",
                        clientFd);
                close(clientFd);
            }

            THREAD_POOL_SUBMIT_TASK(threadPool, parseClientRequest, int32_t,
                                    clientFd);
        }
    }

    close(epollFd);
    ThreadPool_Free(threadPool, true);
    close(mainSocketFd);

    return 0;

errorCloseEpoll:
    close(epollFd);
errorCloseThreadPool:
    ThreadPool_Free(threadPool, true);
errorCloseMainSocket:
    close(mainSocketFd);

    return 1;
}
