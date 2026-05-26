#ifndef NAZARK_SOCKET_OPTION_H
#define NAZARK_SOCKET_OPTION_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

void socket_option_reuseAddr(int32_t fd, bool value) {
    int32_t opt = value ? 1 : 0;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

int32_t socket_option_setNonBlocking(int32_t fd) {
    int32_t flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        return -2;
    }
    return 0;
}

#endif // NAZARK_SOCKET_OPTION_H
