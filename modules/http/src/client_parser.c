#include "client_parser.h"
#include "common.h"
#include "linked_list.h"
#include "request.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// both are inclusive
typedef struct {
    uint64_t startPos;
    uint64_t endPos;
} Range;

int8_t Range_IsEmpty(Range value) { return value.startPos >= value.endPos; }

struct ParserContext {
    NKBuffer buffer;
    ParsingState state;
    uint32_t bodyExpectedLength;
    Range startLineRange;
    Range headersRange;
    HttpRequest *request;
};

/// ==================== ========== ====================
/// ==================== Public Api ====================
/// ==================== ========== ====================

NKBuffer initNKBuffer(void) {
    uint8_t *buf = calloc(REQUEST_HEADERS_BUFFER_SIZE, sizeof(uint8_t));
    if (!buf) {
        return NKBuffer_Empty();
    }

    return (NKBuffer){ .values   = buf,
                       .capacity = REQUEST_HEADERS_BUFFER_SIZE,
                       .length   = 0 };
}

ParserContext *ParserContext_Init(void) {
    ParserContext *ctx = calloc(1, sizeof(ParserContext));
    if (!ctx) {
        return NULL;
    }

    ctx->state              = Empty;
    ctx->buffer             = initNKBuffer();
    ctx->startLineRange     = (Range){ .startPos = 0, .endPos = 0 };
    ctx->headersRange       = (Range){ .startPos = 0, .endPos = 0 };
    ctx->bodyExpectedLength = 0;
    ctx->request            = NULL;

    return ctx;
}

void ParserContext_Free(ParserContext *ctx) {
    if (!ctx) {
        return;
    }

    NKBuffer_Free(ctx->buffer);

    free(ctx);
}

NKBuffer *ParserContext_GetBuffer(ParserContext *ctx) {
    if (!ctx) {
        return NULL;
    }

    return &ctx->buffer;
}

HttpRequest *Parser_TryGetRequest(ParserContext *ctx) {
    if (!ctx) {
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
    NKBuffer *cb = &ctx->buffer;

    if (cb->length <= 0) {
        return -1;
    }

    void *sepPtr =
        memmem(cb->values, cb->length, HTTP_SEPARATOR, HTTP_SEPARATOR_LEN);

    if (!sepPtr) {
        return -1;
    }

    ctx->startLineRange.endPos = (uint64_t)sepPtr - (uint64_t)cb->values - 1;
    ctx->headersRange.startPos = (uint64_t)sepPtr + HTTP_SEPARATOR_LEN;

    ctx->state = AwaitingHeaders;

    return 0;
}

int32_t findHeaders(ParserContext *ctx) {
    NKBuffer *cb = &ctx->buffer;

    if (cb->length <= 0) {
        return -1;
    }
    if (Range_IsEmpty(ctx->startLineRange) || !ctx->headersRange.startPos) {
        ctx->state = BadRequestError;
        return -1;
    }

    void *sepPtr = memmem(cb->values + ctx->headersRange.startPos, cb->length,
                          DOUBLE_HTTP_SEPARATOR, DOUBLE_HTTP_SEPARATOR_LEN);

    if (!sepPtr) {
        if (cb->length >= cb->capacity) {
            ctx->state = LongHeadersError;
            return -1;
        }

        sepPtr = memmem(cb->values + ctx->headersRange.startPos, cb->length,
                        HTTP_SEPARATOR, HTTP_SEPARATOR_LEN);

        // case: request without headers at all
        // "GET /path HTTP/1.0\r\n\r\n"
        //                         ^
        //                         |
        //                  headersRange.startPos
        if (sepPtr == cb->values + ctx->headersRange.startPos) {
            ctx->headersRange.startPos = 0;
            ctx->headersRange.endPos   = 0;
            ctx->state                 = CompleteHeaders;
            return 0;
        }
    }

    ctx->headersRange.endPos = (uint64_t)sepPtr - (uint64_t)cb->values - 1;
    ctx->state               = CompleteHeaders;

    return 0;
}

void freeKeyValuePair(void *arg) {
    if (!arg) {
        return;
    }
    KeyValuePair *pair = (KeyValuePair *)arg;
    NKString_Free(pair->key);
    NKString_Free(pair->value);
}

RequestParamArray parseParams(const uint8_t *data, uint64_t dataLen) {
    if (!data || !dataLen) {
        return RequestParamArray_Empty();
    }

    LinkedList_t *paramsList = LinkedList_Create(freeKeyValuePair);
    if (!paramsList) {
        return RequestParamArray_Empty();
    }

    KeyValuePair *pair = NULL;

    char *query = (char *)data;
    char *token = strtok(query, "&");
    while (token) {
        NKString key   = NKString_Empty();
        NKString value = NKString_Empty();

        uint64_t tokenOff   = (uint64_t)token;
        uint64_t tokenLen   = 0;
        uint64_t eqOff      = (uint64_t)strchr(token, '=');
        uint64_t nextAndOff = (uint64_t)strchr(token, '&');

        if (nextAndOff >= tokenOff) {
            tokenLen = nextAndOff - tokenOff;
        } else {
            tokenLen = dataLen - tokenOff;
        }

        if (eqOff > tokenOff) {
            key = NKString_CopyFrom(token, eqOff - tokenOff);
            if (eqOff < tokenLen) {
                value = NKString_CopyFrom(token + eqOff + 1, tokenLen - eqOff);
            } else {
                value = NKString_Empty();
            }
        } else {
            key = NKString_CopyFrom(token, tokenLen);
        }

        pair = calloc(1, sizeof(KeyValuePair));
        if (!pair) {
            NKString_Free(key);
            NKString_Free(value);
            goto freeListReturnEmpty;
        }

        pair->key   = key;
        pair->value = value;
        if (!LinkedList_AddEnd(paramsList, pair)) {
            NKString_Free(key);
            NKString_Free(value);
            goto freeListReturnEmpty;
        }
    }

    uint64_t paramsCount = 0;
    if (!LinkedList_Length(paramsList, &paramsCount) ||
        paramsCount > UINT16_MAX) {
        goto freeListReturnEmpty;
    }
    KeyValuePair *arr = calloc(paramsCount, sizeof(KeyValuePair));
    if (!arr) {
        goto freeListReturnEmpty;
    }

    uint16_t i            = 0;
    KeyValuePair *outPair = NULL;
    while (LinkedList_PopStart(paramsList, (void **)&outPair)) {
        arr[i] = *outPair;
        ++i;
    }

    return (RequestParamArray){ .values = arr, .count = (uint16_t)paramsCount };

freeListReturnEmpty:
    LinkedList_Free(paramsList);
    return RequestParamArray_Empty();
}

int32_t parseStartLine(ParserContext *ctx) {
    if (!ctx->request) {
        return -1;
    }

    if (Range_IsEmpty(ctx->startLineRange)) {
        return -1;
    }

    RequestStartLine *line = &ctx->request->startLine;
    void *winPtr           = ctx->buffer.values + ctx->startLineRange.startPos;
    uint64_t winLen        = 0;
    uint64_t slLen         = (uint64_t)(ctx->startLineRange.endPos + 1 -
                                ctx->startLineRange.startPos);
    void *spPtr            = NULL;

    spPtr  = memchr(winPtr, SPACE, slLen);
    winLen = (uint64_t)spPtr - (uint64_t)winPtr;
    if (!spPtr || winLen > UINT16_MAX) {
        return -1;
    }
    // processed winLen bytes + 1 space byte
    slLen -= winLen + 1;

    line->method = MethodFromString(winPtr, (uint16_t)winLen);
    if (line->method == UNKNOWN) {
        return -1;
    }
    // points to start of target
    winPtr = (void *)((uint64_t)spPtr + 1);

    spPtr  = memchr(winPtr, SPACE, slLen);
    winLen = (uint64_t)spPtr - (uint64_t)winPtr;
    if (!spPtr || winLen > UINT16_MAX) {
        return -1;
    }

    void *qPtr = memchr(winPtr, '?', winLen);
    if (qPtr) {
        winLen           = (uint64_t)qPtr - (uint64_t)winPtr;
        line->targetPath = NKString_CopyFrom(winPtr, winLen);
        // processed target len + 1 'question mark' byte
        slLen -= winLen + 1;

        // points to start of parameters
        winPtr       = (void *)((uint64_t)qPtr + 1);
        winLen       = (uint64_t)spPtr - (uint64_t)winPtr;
        line->params = parseParams(winPtr, winLen);
        slLen -= winLen + 1;
    } else {
        line->targetPath = NKString_CopyFrom(winPtr, winLen);
        slLen -= winLen + 1;
    }

    winPtr         = (void *)((uint64_t)spPtr + 1);
    line->protocol = NKString_CopyFrom(winPtr, slLen);

    return 0;
}

int32_t parseHeaders(ParserContext *ctx) {
    if (!ctx->request) {
        ctx->state = BadRequestError;
        return -1;
    }

    // case: no headers at all, valid for HTTP/1.0 ->
    // let it be sourted out later
    if (Range_IsEmpty(ctx->headersRange)) {
        ctx->state = Complete;
        return 0;
    }

    // TODO: implement

    return -1;
}

int32_t initRequest(ParserContext *ctx) {
    if (ctx->request) {
        return 0;
    }

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
    if (NKBuffer_IsValidAndEmpty(ctx->buffer)) {
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
            if (findStartLine(ctx)) {
                goto returnLabel;
            }
            break;
        }

        case AwaitingHeaders: {
            if (findHeaders(ctx)) {
                goto returnLabel;
            }
            break;
        }

        case CompleteHeaders: {
            if (initRequest(ctx) || parseStartLine(ctx) || parseHeaders(ctx)) {
                if (ctx->request) {
                    Request_Free(ctx->request);
                    ctx->request = NULL;
                }
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
