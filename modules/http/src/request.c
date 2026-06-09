#include "request.h"
#include "client.h"

#include <stdlib.h>

int32_t Request_TryParseBuffer(ClientContext *context) {
    if (context == NULL) {
        return -1;
    }
    if (context->clientBuffer == NULL) {
        return -1;
    }
    if (context->clientBuffer->buffer == NULL) {
        return -1;
    }

    ClientBuffer *cb = context->clientBuffer;
    if (cb->length > cb->capacity) {
        return -1;
    }

    return 0;
}
