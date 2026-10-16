#include "client_parser.h"
#include "common.h"
#include "linked_list.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// both are inclusive
typedef struct {
    uint64_t startPos;
    uint64_t endPos;
} Range;

int8_t Range_IsEmpty(Range value) { return value.startPos >= value.endPos; }

struct ParserContext {
    NKBuffer buffer;
    RequestParsingState state;
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

    return &(ctx->buffer);
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

bool parseParams(const uint8_t *data, uint64_t dataLen, NKStringPair** outParams, uint16_t *outParamsCount);
void freeParamListElement(void *arg);

int32_t findStartLine(ParserContext *ctx) {
    NKBuffer *cb = &ctx->buffer;

    if (cb->length <= 0) {
        return -1;
    }

    uint64_t slStart = ctx->startLineRange.startPos;
    while (slStart < cb->length && cb->values[slStart] <= (uint8_t)(' ')) {
        ++slStart;
    }

    if (slStart >= cb->length) {
        ctx->state = AwaitingStartLine;
        ctx->startLineRange.startPos = slStart;
        return 0;
    }

    void *sepPtr = memmem(
        cb->values + slStart,
        cb->length - slStart,
        HTTP_SEPARATOR, HTTP_SEPARATOR_LEN);

    if (!sepPtr) {
        return -1;
    }

    ctx->startLineRange.endPos = (uint64_t)sepPtr - (uint64_t)cb->values - 1;
    ctx->headersRange.startPos = ctx->startLineRange.endPos + HTTP_SEPARATOR_LEN + 1;

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

    void *sepPtr = memmem(cb->values + ctx->headersRange.startPos,
        cb->length - ctx->headersRange.startPos,
        DOUBLE_HTTP_SEPARATOR, DOUBLE_HTTP_SEPARATOR_LEN);

    if (!sepPtr) {
        if (cb->length >= cb->capacity) {
            ctx->state = LongHeadersError;
            return -1;
        }

        sepPtr = memmem(cb->values + ctx->headersRange.startPos,
            cb->length - ctx->headersRange.startPos,
            HTTP_SEPARATOR, HTTP_SEPARATOR_LEN);

        // case: request without headers at all
        // "GET /path HTTP/1.0\r\n\r\n"
        //                         ^
        //                         |
        //                  headersRange.startPos
        if (sepPtr == cb->values + ctx->headersRange.startPos) {
            ctx->headersRange.startPos = 0;
            ctx->headersRange.endPos   = 0;
            ctx->state                 = Complete;
            return 0;

        // case: didn't find "\r\n\r\n" but found "\r\n"
        // and "cb->length < cb->capacity"
        // continue reading from client
        } else {
            ctx->state = AwaitingHeaders;
            return 0;
        }
    }

    ctx->headersRange.endPos = (uint64_t)sepPtr - (uint64_t)cb->values - 1;
    ctx->state               = CompleteHeaders;

    return 0;
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
        winPtr = (void *)((uint64_t)qPtr + 1);
        winLen = (uint64_t)spPtr - (uint64_t)winPtr;

        if (!parseParams(winPtr, winLen, &(line->params), &(line->paramsCount))) {
            return -1;
        }

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

    // TODO: implement

    return -1;
}

RequestParsingState Parser_TryParseRequest(ParserContext *ctx) {
    if (ctx == NULL) {
        return UnknownError;
    }
    if (NKBuffer_IsValidAndEmpty(ctx->buffer)) {
        return UnknownError;
    }

    switch (ctx->state) {
    case Empty: {
        ctx->state = AwaitingStartLine;
    }
    case AwaitingStartLine:
    case AwaitingHeaders:
    case CompleteHeaders: 
    case AwaitingBody: {
        // do nothing -> continue
        break;
    }

    case Complete:
    case GenericError:
    case LongHeadersError:
    case BadRequestError:
    case InternalError:
    case UnknownError:
    default: {
        return ctx->state;
    }
    }

    while (1) {
        switch (ctx->state) {
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
            ctx->request = calloc(1, sizeof *(ctx->request));
            if (!ctx->request) {
                ctx->state = InternalError;
                break;
            }

            *ctx->request = HttpRequest_Init();

            if (parseStartLine(ctx) != 0 || parseHeaders(ctx) != 0) {
                HttpRequest_Free(ctx->request);
                ctx->request = NULL;

                ctx->state = BadRequestError;
            }
            break;
        }
        case AwaitingBody: {
            // TODO: implement
            break;
        }

        case Complete:
        case BadRequestError:
        case GenericError:
        case LongHeadersError:
        case InternalError:
        case UnknownError:
        default: {
            goto returnLabel;
        }
        }
    }

returnLabel:
    return ctx->state;
}

/// ==================== ======= ====================
/// ==================== HELPERS ====================
/// ==================== ======= ====================

bool parseParams(const uint8_t *data, uint64_t dataLen, NKStringPair** outParams, uint16_t *outParamsCount) {
    if (!data || !dataLen) {
        *outParams = NULL;
        *outParamsCount = 0;
        return true;
    }

    LinkedList_t *paramsList = LinkedList_Create(freeParamListElement);
    if (!paramsList) {
        return false;
    }

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

        NKStringPair* pair = calloc(1, sizeof *pair);
        if (!pair) {
            NKString_Free(key);
            NKString_Free(value);
            goto freeListReturnFalse;
        }

        (*pair)[0] = key;
        (*pair)[1] = value;

        if (!LinkedList_AddEnd(paramsList, pair)) {
            NKString_Free(key);
            NKString_Free(value);
            free(pair);
            goto freeListReturnFalse;
        }

        token = strtok(NULL, "&");
    }

    uint64_t paramsCount = 0;
    if (!LinkedList_Length(paramsList, &paramsCount) ||
        paramsCount > UINT16_MAX) {
        goto freeListReturnFalse;
    }

    *outParamsCount = (uint16_t)paramsCount;
    *outParams = calloc(*outParamsCount, sizeof(NKStringPair));
    if (!(*outParams)) {
        goto freeListReturnFalse;
    }

    uint16_t i = 0;
    void *item;
    while (LinkedList_PopStart(paramsList, &item)) {
        NKStringPair *pair = item;
        (*outParams)[i][0] = (*pair)[0];
        (*outParams)[i][1] = (*pair)[1];
        ++i;
        free(pair);
    }

    LinkedList_Free(paramsList);
    return true;

freeListReturnFalse:
    LinkedList_Free(paramsList);
    
    if (*outParams) {
        free(*outParams);
        *outParams = NULL;
    }
    *outParamsCount = 0;

    return false;
}

void freeParamListElement(void *arg) {
    if (arg == NULL) {
        return;
    }

    NKStringPair *pair = (NKStringPair *)arg;
    NKStringPair_Free(*pair);
}