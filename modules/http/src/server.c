#include "server.h"
#include "client_reader.h"
#include "common.h"
#include "socket_option.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define MAX_SIMULTANEOUS_CONNECTION_EVENTS 10

volatile sig_atomic_t g_isApplicationAlive = 1;

bool isApplicationAlive(void) { return (bool)g_isApplicationAlive; }

void stopApplication(void) { g_isApplicationAlive = 0; }

void handleProcSignal(int sig) {
    (void)sig;
    stopApplication();
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

    ready =
        epoll_wait(g_mainEpoll, events, MAX_SIMULTANEOUS_CONNECTION_EVENTS, -1);

    if (ready < 0) {
        if (errno == EINTR) {
            return 0;
        }
        printf(stderr, "[ERR] Main loop error during epoll_wait\n");
        return -1;
    }

    for (int32_t i; i < ready; i++) {
        if (events[i].data.fd != mainSocketFd) {
            continue;
        }

        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int32_t clientFd =
            accept(mainSocketFd, (struct sockaddr *)(&clientAddr), &clientLen);

        if (clientFd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            printf(stderr, "[ERR] Main loop error during accept: %d\n", errno);
            return errno;
        }

        switch (socket_option_setNonBlocking(clientFd)) {
        case -1:
            printf(stderr, "[ERR] setNonBlocking: F_GETFL\n");
            close(clientFd);
            continue;
        case -2:
            printf(stderr, "[ERR] setNonBlocking: F_SETFL\n");
            close(clientFd);
            continue;
        }

        struct epoll_event ev = { .events  = EPOLLIN | EPOLLET,
                                  .data.fd = clientFd };
        if (epoll_ctl(g_clientsEpoll, EPOLL_CTL_ADD, clientFd, &ev) != 0) {
            printf(stderr, "[ERR] epoll_ctl: ADD clientFd=%d\n", clientFd);
            close(clientFd);
        }

        printf(stdout, "[INF] Accepted connection (fd=%d)\n", clientFd);
    }

    return 0;
}

int32_t server_start(void) {
    int32_t returnCode = 0;

    struct sigaction sa = { .sa_handler = handleProcSignal,
                            .sa_flags   = SA_RESTART | SA_SIGINFO };
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGKILL, &sa, NULL);

    int32_t mainSocketFd = initSocket(SERVER_REQUESTS_QUEUE_SIZE);
    if (mainSocketFd < 0) {
        return ERROR_SERVER_INVALID_SOCKET_DESCRIPTOR;
    }
    socket_option_setNonBlocking(mainSocketFd);

    g_mainEpoll = epoll_create1(0);
    if (g_mainEpoll != 0) {
        printf(stderr,
               "[ERR] Initialization error: epoll_create1 (g_mainEpoll) - %d\n",
               errno);
        returnCode = -1;
        goto closeMainSocket;
    }
    g_clientsEpoll = epoll_create1(0);
    if (g_clientsEpoll != 0) {
        printf(
            stderr,
            "[ERR] Initialization error: epoll_create1 (g_clientsEpoll) - %d\n",
            errno);
        returnCode = -1;
        goto closeMainEpoll;
    }

    struct epoll_event ev = { .events = EPOLLIN, .data.fd = mainSocketFd };
    if (epoll_ctl(g_mainEpoll, EPOLL_CTL_ADD, mainSocketFd, &ev) != 0) {
        printf(stderr, "[ERR] Initialization error: epoll_ctl - %d\n", errno);
        returnCode = -1;
        goto closeAllEpolls;
    }

    ClientReaderArg_t clientReaderArg =
        (ClientReaderArg_t){ .epoll              = g_clientsEpoll,
                             .isApplicationAlive = &isApplicationAlive,
                             .threadFailed       = false };

    pthread_t clientReaderThread;
    if (pthread_create(&clientReaderThread, NULL, &clientReaderRoutine,
                       &clientReaderArg) != 0) {
        printf(stderr, "[ERR] Initialization error: pthread_create\n");
        returnCode = -1;
        goto closeAllEpolls;
    }

    fprintf(stdout,
            "[INF] Server is initalized successfully.\n"
            "Listening on port :%d (Ctrl+C to stop)...\n",
            SERVER_PORT);

    while (isApplicationAlive()) {
        if (clientReaderArg.threadFailed) {
            printf(stderr, "[ERR] Client reading thread had failed\n");
            stopApplication();
            break;
        }

        returnCode = mainLoopAction(mainSocketFd);
        if (returnCode) {
            break;
        }
    }

    pthread_cancel(clientReaderThread);
    pthread_join(clientReaderThread, NULL);

closeAllEpolls:
    close(g_clientsEpoll);

closeMainEpoll:
    close(g_mainEpoll);

closeMainSocket:
    close(mainSocketFd);

    return returnCode;
}
