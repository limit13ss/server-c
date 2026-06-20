#include "client_parser.h"
#include "array_utils.h"
#include "common.h"
#include "linked_list.h"
#include "request.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// both are inclusive
typedef struct {
    int64_t startPos;
    int64_t endPos;
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
    NKBuffer *cb = &ctx->buffer;

    if (cb->length <= 0) {
        return -1;
    }
    if (ctx->headersRange.startPos < 0) {
        ctx->state = BadRequestError;
        return -1;
    }

    int64_t headEndPos = indexOfSeqSkip(
        cb->values, cb->length, DOUBLE_HTTP_SEPARATOR,
        DOUBLE_HTTP_SEPARATOR_LEN, (uint32_t)ctx->headersRange.startPos);

    if (headEndPos == -1) {
        if (cb->length >= cb->capacity) {
            ctx->state = LongHeadersError;
            return -1;
        }

        int64_t separatorIdx = indexOfSeqSkip(
            cb->values, cb->length, HTTP_SEPARATOR, HTTP_SEPARATOR_LEN,
            (uint32_t)ctx->headersRange.startPos);

        // case: request without single header
        if (separatorIdx == ctx->headersRange.startPos) {
            ctx->headersRange.startPos = -1;
            ctx->headersRange.endPos   = -1;
            ctx->state                 = CompleteHeaders;
            return 0;
        }
    }

    ctx->headersRange.endPos = headEndPos - 1;
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

    uint8_t *buf           = ctx->buffer.values + ctx->startLineRange.startPos;
    char *bufStr           = (char *)buf;
    uint32_t bufLen        = (uint32_t)(ctx->startLineRange.endPos + 1 -
                                 ctx->startLineRange.startPos);
    RequestStartLine *line = &ctx->request->startLine;

    int64_t lIdx = 0, rIdx = 0;
    rIdx = indexOf(buf, bufLen, SPACE);
    if (rIdx < lIdx || rIdx > UINT8_MAX) {
        return -1;
    }

    line->method = MethodFromString(bufStr, (uint8_t)rIdx);
    if (line->method == UNKNOWN) {
        return -1;
    }

    lIdx = rIdx + 1;
    rIdx = indexOfSkip(buf, bufLen, SPACE, (uint32_t)lIdx);
    if (rIdx <= lIdx || rIdx > UINT16_MAX) {
        return -1;
    }

    int64_t paramsIdx = indexOfSkip(buf, bufLen, '?', (uint32_t)lIdx);
    if (paramsIdx == -1) {
        line->targetPath =
            NKString_CopyFrom(bufStr + lIdx, (uint64_t)(rIdx - lIdx));
    } else {
        line->targetPath =
            NKString_CopyFrom(bufStr + lIdx, (uint64_t)(paramsIdx - lIdx));
        line->params =
            parseParams(buf + paramsIdx + 1, (uint64_t)(rIdx - paramsIdx - 1));
    }

    lIdx = rIdx + 1;
    line->protocol =
        NKString_CopyFrom(bufStr + lIdx, (uint64_t)(bufLen - lIdx));

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
