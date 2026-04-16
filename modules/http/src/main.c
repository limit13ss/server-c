#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "server.h"

int32_t main(void) {
    int32_t mainListenerSocketFD = server_initMainListener();

    printf("Socket fd=%d\n", mainListenerSocketFD);
    getchar();

    close(mainListenerSocketFD);
    return 0;
}
