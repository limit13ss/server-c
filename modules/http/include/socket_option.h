#ifndef NAZARK_SOCKET_OPTION_H
#define NAZARK_SOCKET_OPTION_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/socket.h>

void socket_option_reuseAddr(int32_t fd, bool value) {
    int32_t opt = value ? 1 : 0;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
}

#endif // NAZARK_SOCKET_OPTION_H
