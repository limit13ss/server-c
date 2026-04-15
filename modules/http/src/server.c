#include "server.h"
#include "socket_option.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

int32_t socketInit(uint8_t connectionQueueSize) {
    int32_t socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
        fprintf(stderr, "[ERR] Socket creation failed: %s\n", strerror(errno));
        return socketFd;
    }

    struct sockaddr_in addr =
        (struct sockaddr_in){ .sin_family = AF_INET, .sin_port = htons(SERVER_PORT), .sin_addr.s_addr = INADDR_ANY };

    socket_option_reuseAddr(socketFd, true);
    int32_t rCode = bind(socketFd, (struct sockaddr *)&addr, sizeof(addr));
    if (rCode == -1) {
        fprintf(stderr, "[ERR] Socket binding failed: %s\n", strerror(errno));
        close(socketFd);
        return rCode;
    }
    rCode = listen(socketFd, connectionQueueSize);
    if (rCode == -1) {
        fprintf(stderr, "[ERR] Connection listening failed: %s\n", strerror(errno));
        close(socketFd);
        return rCode;
    }

    return socketFd;
}

int32_t server_initMainListener(void) {
    int32_t socketFd = socketInit(SERVER_REQUESTS_QUEUE_SIZE);
    if (socketFd == -1) {
        return socketFd;
    }

    fprintf(stdout, "[INF] Initialized successfully.\nListening on port :%d\n", SERVER_PORT);
    return socketFd;
}

void server_mainLoop(void) {
    while (1) {
    }
}
