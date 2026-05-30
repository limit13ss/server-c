#include "server.h"
#include "client_reader.h"
#include "common.h"
#include "concurrent_linked_list.h"
#include "socket_option.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define MAX_EVENTS 10
#define WORKERS_COUNT_CLIENT_PARSERS 2

volatile sig_atomic_t g_isApplicationAlive = 1;
bool isApplicationAlive(void) { return (bool)g_isApplicationAlive; }

void handleProcSignal(int sig) {
    (void)sig;
    g_isApplicationAlive = 0;
}

bool processClient(int32_t clientFd) {
    (void)clientFd;
    return false;
}

void int32Deallocator(void *data) {
    if (data == NULL) {
        return;
    }
    free(data);
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

int32_t mainLoopAction(int32_t mainSocketFd, int32_t epollFd,
                       struct epoll_event *ev,
                       ConcurrentLinkedList_t *clientList) {
    int32_t ready = 0;
    struct epoll_event events[MAX_EVENTS];

    ready = epoll_wait(epollFd, events, MAX_EVENTS, -1);

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

        printf(stdout, "[INF] Accepted connection (fd=%d)\n", clientFd);

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

        ev->events  = EPOLLIN | EPOLLET;
        ev->data.fd = clientFd;
        if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, ev) == -1) {
            fprintf(stderr, "[ERR] epoll_ctl: clientFd (%d)\n", clientFd);
            close(clientFd);
            continue;
        }

        int32_t *clientFdPtr = malloc(sizeof(int32_t));
        if (clientFdPtr == NULL) {
            fprintf(stderr, "[ERR] mainLoopAction: malloc clientFdPtr\n");
            close(clientFd);
            continue;
        }

        *clientFdPtr = clientFd;
        if (!ConcurrentLinkedList_AddEnd(clientList, (void *)clientFdPtr)) {
            fprintf(stderr,
                    "[ERR] mainLoopAction: ConcurrentLinkedList_AddEnd\n");
            free(clientFdPtr);
            close(clientFd);
            continue;
        }
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

    int32_t epollFd = epoll_create1(0);
    if (epollFd != 0) {
        printf(stderr, "[ERR] Initialization error: epoll_create1 - %d\n",
               errno);
        returnCode = -1;
        goto closeMainSocket;
    }

    struct epoll_event ev = { .events = EPOLLIN, .data.fd = mainSocketFd };
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, mainSocketFd, &ev) != 0) {
        printf(stderr, "[ERR] Initialization error: epoll_ctl - %d\n", errno);
        returnCode = -1;
        goto closeEpoll;
    }

    ConcurrentLinkedList_t *clientList = ConcurrentLinkedList_Create(&int32Deallocator);
    if (clientList == NULL) {
        printf(stderr, "[ERR] Initialization error: LinkedList_Create\n");
        returnCode = -1;
        goto closeEpoll;
    }

    ClientReaderArg_t clientReaderArg = { .clients = clientList,
                                          .event   = &ev,
                                          .isApplicationAlive =
                                              &isApplicationAlive };

    pthread_t clientReaderThread;
    if (pthread_create(&clientReaderThread, NULL, &clientReaderRoutine,
                       &clientReaderArg) != 0) {
        printf(stderr, "[ERR] Initialization error: pthread_create\n");
        returnCode = -1;
        goto freeLinkedList;
    }

    fprintf(stdout,
            "[INF] Server is initalized successfully.\n"
            "Listening on port :%d (Ctrl+C to stop)...\n",
            SERVER_PORT);

    while (isApplicationAlive()) {
        returnCode = mainLoopAction(mainSocketFd, epollFd, &ev, clientList);
        if (returnCode) {
            break;
        }
    }

freeLinkedList:
    ConcurrentLinkedList_Free(clientList);
    pthread_cancel(clientReaderThread);
    pthread_join(clientReaderThread, NULL);

closeEpoll:
    close(epollFd);

closeMainSocket:
    close(mainSocketFd);

    return returnCode;
}
