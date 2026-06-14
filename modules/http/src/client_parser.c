#include "client_parser.h"
#include "array_utils.h"
#include "request.h"

#include <stdlib.h>

// both are inclusive
typedef struct {
    int64_t startPos;
    int64_t endPos;
} Range;

int8_t Range_IsInitial(Range value) {
    return value.startPos == -1 || value.endPos == -1;
}

struct ParserContext {
    NKBuffer *buffer;
    ParsingState state;
    Range startLineRange;
    Range headersRange;
    uint32_t bodyExpectedLength;
    HttpRequest *request;
};

/// ==================== ========== ====================
/// ==================== Public Api ====================
/// ==================== ========== ====================

NKBuffer *initNKBuffer(void) {
    uint8_t *buf = calloc(REQUEST_HEADERS_BUFFER_SIZE, sizeof(uint8_t));
    if (!buf) {
        return NULL;
    }

    NKBuffer *cb = calloc(1, sizeof(NKBuffer));
    if (!cb) {
        free(buf);
        return NULL;
    }
    *cb = (NKBuffer){ .values   = buf,
                      .capacity = REQUEST_HEADERS_BUFFER_SIZE,
                      .length   = 0 };
    return cb;
}

ParserContext *ParserContext_Init(void) {
    ParserContext *ctx = calloc(1, sizeof(ParserContext));
    if (!ctx) {
        return NULL;
    }

    NKBuffer *buffer = initNKBuffer();
    if (buffer) {
        free(ctx);
        return NULL;
    }

    ctx->state              = Empty;
    ctx->buffer             = buffer;
    ctx->startLineRange     = (Range){ .startPos = -1, .endPos = -1 };
    ctx->headersRange       = (Range){ .startPos = -1, .endPos = -1 };
    ctx->bodyExpectedLength = 0;
    ctx->request            = NULL;

    return ctx;
}

void ParserContext_Free(ParserContext *ctx) {
    if (!ctx) {
        return;
    }

    free(ctx->buffer->values);
    free(ctx->buffer);

    if (ctx->request) {
        Request_Free(ctx->request);
    }
    free(ctx);
}

NKBuffer *ParserContext_GetBuffer(ParserContext *ctx) {
    if (ctx == NULL) {
        return NULL;
    }

    return ctx->buffer;
}

HttpRequest *Parser_TryGetRequest(ParserContext *ctx) {
    if (ctx == NULL) {
        return NULL;
    }
    if (ctx->state != Complete && ctx->state != AwaitingBody) {
        return NULL;
    }

    return ctx->request;
}

/// ==================== =============== ====================
/// ==================== REQUEST PARSING ====================
/// ==================== =============== ====================

int32_t findStartLine(ParserContext *ctx) {
    NKBuffer *cb = ctx->buffer;

    if (cb->length <= 0) {
        return -1;
    }

    if (ctx->startLineRange.startPos < 0) {
        ctx->startLineRange.startPos = 0;
    }

    int64_t lineEndPos =
        indexOfSeq(cb->values, cb->length, HTTP_SEPARATOR, HTTP_SEPARATOR_LEN);

    if (lineEndPos == -1) {
        return -1;
    }

    ctx->startLineRange.endPos = lineEndPos - 1;
    ctx->headersRange.startPos = lineEndPos + HTTP_SEPARATOR_LEN;

    ctx->state = AwaitingHeaders;

    return 0;
}

int32_t findHeaders(ParserContext *ctx) {
    NKBuffer *cb = ctx->buffer;

    if (cb->length <= 0) {
        return -1;
    }
    if (ctx->headersRange.startPos < 0) {
        ctx->state = BadRequestError;
        return -1;
    }

    int64_t headEndPos = indexOfSeqOff(
        cb->values, cb->length, DOUBLE_HTTP_SEPARATOR,
        DOUBLE_HTTP_SEPARATOR_LEN, (uint32_t)ctx->headersRange.startPos);

    if (headEndPos == -1 && cb->length >= cb->capacity) {
        ctx->state = LongHeadersError;
        return -1;
    }

    ctx->headersRange.endPos = headEndPos - 1;
    ctx->state               = CompleteHeaders;

    return 0;
}

int32_t parseStartLine(ParserContext *ctx) {
    if (!ctx->request) {
        ctx->state = BadRequestError;
        return -1;
    }
    if (Range_IsInitial(ctx->startLineRange)) {
        ctx->state = BadRequestError;
        return -1;
    }

    // TODO: implement

    return -1;
}

int32_t parseHeaders(ParserContext *ctx) {
    if (!ctx->request) {
        ctx->state = BadRequestError;
        return -1;
    }
    if (Range_IsInitial(ctx->headersRange)) {
        ctx->state = BadRequestError;
        return -1;
    }

    // TODO: implement

    return -1;
}

int32_t initRequest(ParserContext *ctx) {
    ctx->request = Request_Init();
    if (!ctx->request) {
        return -1;
    }

    return 0;
}

ParsingState Parser_TryParseHeaders(ParserContext *ctx) {
    if (ctx == NULL) {
        return UnknownError;
    }
    if (ctx->buffer == NULL) {
        return UnknownError;
    }
    if (ctx->buffer->values == NULL) {
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
            if (initRequest(ctx) || parseStartLine(ctx) || parseHeaders(ctx)) {
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
