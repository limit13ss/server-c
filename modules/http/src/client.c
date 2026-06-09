#include "client.h"

#include <stdlib.h>

ClientBuffer *initClientBuffer(void) {
    uint8_t *buf = calloc(REQUEST_HEADERS_BUFFER_SIZE, sizeof(uint8_t));
    if (buf == NULL) {
        return NULL;
    }

    ClientBuffer *cb = calloc(1, sizeof(ClientBuffer));
    if (cb == NULL) {
        free(buf);
        return NULL;
    }
    *cb = (ClientBuffer){ .buffer   = buf,
                          .capacity = REQUEST_HEADERS_BUFFER_SIZE,
                          .length   = 0 };
    return cb;
}

ClientContext *Client_InitContext(void) {
    ClientContext *ctx = calloc(1, sizeof(ClientContext));
    if (ctx == NULL) {
        return NULL;
    }

    ClientBuffer *cb = initClientBuffer();
    if (cb == NULL) {
        free(ctx);
        return NULL;
    }

    ctx->state              = Empty;
    ctx->clientBuffer       = cb;
    ctx->lastParsedByte     = -1;
    ctx->bodyExpectedLength = 0;

    return ctx;
}

void Client_FreeContext(ClientContext *ctx) {
    if (ctx == NULL) {
        return;
    }

    free(ctx->clientBuffer->buffer);
    free(ctx->clientBuffer);
    free(ctx);
}
