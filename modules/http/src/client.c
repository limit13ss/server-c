#include "client.h"
#include "request.h"

#include <stdlib.h>

struct ClientContext {
    ClientBuffer *clientBuffer;
    ClientParsingState state;
    int32_t lastParsedByte;
    uint32_t bodyExpectedLength;
    HttpRequest *request;
};

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
    ctx->request            = NULL;

    return ctx;
}

void Client_FreeContext(ClientContext *ctx) {
    if (ctx == NULL) {
        return;
    }

    free(ctx->clientBuffer->buffer);
    free(ctx->clientBuffer);

    if (ctx->request != NULL) {
        Request_Free(ctx->request);
    }
    free(ctx);
}

ClientBuffer *Client_GetBuffer(ClientContext *ctx) {
    if (ctx == NULL) {
        return NULL;
    }

    return ctx->clientBuffer;
}

HttpRequest *Client_GetRequest(ClientContext *ctx) {
    if (ctx == NULL) {
        return NULL;
    }
    if (ctx->state != Complete) {
        return NULL;
    }

    return ctx->request;
}

/// ==================== =============== ====================
/// ==================== REQUEST PARSING ====================
/// ==================== =============== ====================

int32_t parseStartLine(ClientContext *ctx) {
    ctx->state = AwaitingStartLine;

    uint8_t *buf = ctx->clientBuffer->buffer;
    int32_t pos  = ctx->lastParsedByte;

    return pos;
}

ClientParsingState Request_TryParseHeaders(ClientContext *ctx) {
    if (ctx == NULL) {
        return UnknownError;
    }
    if (ctx->clientBuffer == NULL) {
        return UnknownError;
    }
    if (ctx->clientBuffer->buffer == NULL) {
        return UnknownError;
    }

    if (ctx->state == AwaitingBody || ctx->state == Complete ||
        ctx->state == GenericError || ctx->state == LongHeadersError) {
        return ctx->state;
    }

    ClientBuffer *cb = ctx->clientBuffer;
    if (ctx->lastParsedByte >= 0 &&
        (uint32_t)ctx->lastParsedByte >= cb->length) {
        return ctx->state;
    }

    while (ctx->lastParsedByte < 0 ||
           (uint32_t)ctx->lastParsedByte <= cb->length) {
        switch (ctx->state) {
        case Empty:
        case AwaitingStartLine: {
            parseStartLine(ctx);
            break;
        }

        case UnknownError:
        default: {
            ctx->state = UnknownError;
            break;
        }
        }
    }

    return ctx->state;
}
