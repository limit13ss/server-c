#include "server.h"
#include "client_reader.h"
#include "common.h"
#include "socket_option.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/socket.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define MAX_SIMULTANEOUS_CONNECTION_EVENTS 10
#define EPOLL_WAIT_TIMEOUT_MS 500

volatile sig_atomic_t g_isServerAlive = 1;

bool isServerAlive(void) { return (bool)g_isServerAlive; }

void stopServer(void) { g_isServerAlive = 0; }

void handleProcSignal(int sig) {
    (void)sig;
    stopServer();
}

int32_t g_mainEpoll    = 0;
int32_t g_clientsEpoll = 0;

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

int32_t mainLoopAction(int32_t mainSocketFd) {
    int32_t ready = 0;
    struct epoll_event events[MAX_SIMULTANEOUS_CONNECTION_EVENTS];

    ready = epoll_wait(g_mainEpoll, events, MAX_SIMULTANEOUS_CONNECTION_EVENTS,
                       EPOLL_WAIT_TIMEOUT_MS);

    if (ready < 0) {
        if (errno == EINTR) {
            return 0;
        }
        fprintf(stderr, "[ERR] Main loop error during epoll_wait\n");
        return -1;
    }

    for (int32_t i = 0; i < ready; i++) {
        if (events[i].data.fd != mainSocketFd) {
            continue;
        }

        while (true) {
            // draining all pending connection in one wake
            struct sockaddr_in clientAddr;
            socklen_t clientLen = sizeof(clientAddr);

            int32_t clientFd = accept(
                mainSocketFd, (struct sockaddr *)(&clientAddr), &clientLen);

            if (clientFd == -1) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    break;
                }
                fprintf(stderr, "[ERR] Main loop error during accept: %d\n",
                        errno);
                return errno;
            }

            switch (socket_option_setNonBlocking(clientFd)) {
            case -1:
                fprintf(stderr, "[ERR] setNonBlocking: F_GETFL\n");
                close(clientFd);
                continue;
            case -2:
                fprintf(stderr, "[ERR] setNonBlocking: F_SETFL\n");
                close(clientFd);
                continue;
            }

            struct epoll_event ev = { .events  = EPOLLIN | EPOLLET,
                                      .data.fd = clientFd };

            if (epoll_ctl(g_clientsEpoll, EPOLL_CTL_ADD, clientFd, &ev) != 0) {
                fprintf(stderr, "[ERR] epoll_ctl: ADD clientFd=%d\n", clientFd);
                close(clientFd);
            }

            fprintf(stdout, "[INF] Accepted connection (fd=%d)\n", clientFd);
        }
    }

    return 0;
}

int32_t server_Run(void) {
    int32_t returnCode = 0;

    struct sigaction sa = { .sa_handler = handleProcSignal,
                            .sa_flags   = SA_RESTART | SA_SIGINFO };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);

    int32_t mainSocketFd = initSocket(SERVER_REQUESTS_QUEUE_SIZE);
    if (mainSocketFd < 0) {
        return ERROR_SERVER_INVALID_SOCKET_DESCRIPTOR;
    }
    socket_option_setNonBlocking(mainSocketFd);

    g_mainEpoll = epoll_create1(0);
    if (g_mainEpoll < 0) {
        fprintf(
            stderr,
            "[ERR] Initialization error: epoll_create1 (g_mainEpoll) - %d\n",
            errno);
        returnCode = -1;
        goto closeMainSocket;
    }
    g_clientsEpoll = epoll_create1(0);
    if (g_clientsEpoll < 0) {
        fprintf(
            stderr,
            "[ERR] Initialization error: epoll_create1 (g_clientsEpoll) - %d\n",
            errno);
        returnCode = -1;
        goto closeMainEpoll;
    }

    struct epoll_event ev = { .events = EPOLLIN, .data.fd = mainSocketFd };
    if (epoll_ctl(g_mainEpoll, EPOLL_CTL_ADD, mainSocketFd, &ev) != 0) {
        fprintf(stderr, "[ERR] Initialization error: epoll_ctl - %d\n", errno);
        returnCode = -1;
        goto closeAllEpolls;
    }

    int32_t stopEventFd = eventfd(0, EFD_NONBLOCK);
    if (stopEventFd < 0) {
        fprintf(stderr, "[ERR] Initialization error: eventfd - %d\n", errno);
        returnCode = -1;
        goto closeAllEpolls;
    }

    struct epoll_event stopEv = { .events = EPOLLIN, .data.fd = stopEventFd };
    if (epoll_ctl(g_clientsEpoll, EPOLL_CTL_ADD, stopEventFd, &stopEv) != 0) {
        fprintf(stderr, "[ERR] epoll_ctl failed for stopEventFd\n");
        returnCode = -1;
        goto closeStopEventFd;
    }

    ClientReaderArg_t clientReaderArg =
        (ClientReaderArg_t){ .epoll         = g_clientsEpoll,
                             .stopEventFd   = stopEventFd,
                             .isServerAlive = &isServerAlive,
                             .threadFailed  = false };

    pthread_t clientReaderThread;
    if (pthread_create(&clientReaderThread, NULL, &clientReaderRoutine,
                       &clientReaderArg) != 0) {
        fprintf(stderr, "[ERR] Initialization error: pthread_create\n");
        returnCode = -1;
        goto closeAllEpolls;
    }

    fprintf(stdout,
            "[INF] Server is initalized successfully.\n"
            "Listening on port :%d (Ctrl+C to stop)...\n",
            SERVER_PORT);

    while (isServerAlive()) {
        if (clientReaderArg.threadFailed) {
            fprintf(stderr, "[ERR] Client reading thread had failed\n");
            stopServer();
            break;
        }

        returnCode = mainLoopAction(mainSocketFd);
        if (returnCode) {
            break;
        }
    }

    stopServer();

    uint64_t stopSignal = 1;
    if (write(stopEventFd, &stopSignal, sizeof(stopSignal)) < 0) {
        fprintf(stderr, "[ERR] Failed to write to stopEventFd\n");
    }

    pthread_join(clientReaderThread, NULL);

closeStopEventFd:
    close(stopEventFd);
closeAllEpolls:
    close(g_clientsEpoll);
closeMainEpoll:
    close(g_mainEpoll);
closeMainSocket:
    close(mainSocketFd);

    return returnCode;
}
