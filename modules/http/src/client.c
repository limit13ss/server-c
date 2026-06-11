#include "client.h"
#include "array_utils.h"
#include "request.h"

#include <stdlib.h>

// both are inclusive
typedef struct {
    int64_t startPos;
    int64_t endPos;
} Range;

struct ClientContext {
    ClientBuffer *clientBuffer;
    ClientParsingState state;
    Range startLineRange;
    Range headersRange;
    uint32_t bodyExpectedLength;
    HttpRequest *request;
};

int8_t isDefaultRange(Range range) {
    return range.endPos == -1 && range.startPos == -1;
}

/// ==================== ========== ====================
/// ==================== Public Api ====================
/// ==================== ========== ====================

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
    ctx->startLineRange     = (Range){ .startPos = -1, .endPos = -1 };
    ctx->headersRange       = (Range){ .startPos = -1, .endPos = -1 };
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

int32_t findStartLine(ClientContext *ctx) {
    ClientBuffer *cb = ctx->clientBuffer;

    if (cb->length <= 0) {
        return -1;
    }

    if (ctx->startLineRange.startPos < 0) {
        ctx->startLineRange.startPos = 0;
    }

    int64_t lineEndPos =
        indexOfSeq(cb->buffer, cb->length, HTTP_SEPARATOR, HTTP_SEPARATOR_LEN);

    if (lineEndPos == -1) {
        return -1;
    }

    ctx->startLineRange.endPos = lineEndPos - 1;
    ctx->headersRange.startPos = lineEndPos + HTTP_SEPARATOR_LEN;

    ctx->state = AwaitingHeaders;

    return 0;
}

int32_t findHeaders(ClientContext *ctx) {
    ClientBuffer *cb = ctx->clientBuffer;

    if (cb->length <= 0) {
        return -1;
    }
    if (ctx->headersRange.startPos < 0) {
        ctx->state = BadRequestError;
        return -1;
    }

    int64_t headEndPos = indexOfSeqOff(
        cb->buffer, cb->length, DOUBLE_HTTP_SEPARATOR,
        DOUBLE_HTTP_SEPARATOR_LEN, (uint32_t)ctx->headersRange.startPos);

    if (headEndPos == -1 && cb->length >= cb->capacity) {
        ctx->state = LongHeadersError;
        return -1;
    }

    ctx->headersRange.endPos = headEndPos - 1;
    ctx->state               = CompleteHeaders;

    return 0;
}

int32_t parseStartLine(ClientContext *ctx) { return -1; }

int32_t parseHeaders(ClientContext *ctx) { return -1; }

int32_t initRequest(ClientContext *ctx) {
    ctx->request = calloc(1, sizeof(HttpRequest));
    if (ctx->request == NULL) {
        return -1;
    }

    return 0;
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

    while (1) {
        switch (ctx->state) {

        case Empty: {
            ctx->state = AwaitingStartLine;
            break;
        }

        case AwaitingStartLine: {
            if (findStartLine(ctx) != 0) {
                goto returnLabel;
            }
            break;
        }

        case AwaitingHeaders: {
            if (findHeaders(ctx) != 0) {
                goto returnLabel;
            }
            break;
        }

        case CompleteHeaders: {
            if (initRequest(ctx) != 0 || parseStartLine(ctx) != 0 ||
                parseHeaders(ctx) != 0) {
                ctx->state = BadRequestError;
            }
            break;
        }
        case AwaitingBody:
        case Complete:
        case BadRequestError:
        case GenericError:
        case LongHeadersError:
        case UnknownError:
        default: {
            goto returnLabel;
        }
        }
    }

returnLabel:
    return ctx->state;
}
