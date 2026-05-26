#include "request_parser.h"

#include <unistd.h>

void parseClientRequest(void *arg) {
    if (arg == NULL) {
        return;
    }

    ParseRequestArg_t *requestArg = (ParseRequestArg_t *)(arg);

    close(requestArg->clientFd);
}
