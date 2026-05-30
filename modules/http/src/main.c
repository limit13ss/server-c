#include <stdint.h>
#include <unistd.h>

#include "server.h"

int32_t main(void) {
    server_mainLoop();
    return 0;
}
