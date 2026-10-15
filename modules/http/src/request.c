#include "request.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *str;
    uint8_t strLen;
    HttpMethod value;
} MethodMapping;

const MethodMapping METHODS_MAP[] = {
    { .str = "GET", .strLen = 3, .value = GET },
    { .str = "POST", .strLen = 4, .value = POST },
    { .str = "PUT", .strLen = 3, .value = PUT },
    { .str = "PATCH", .strLen = 5, .value = PATCH },
    { .str = "DELETE", .strLen = 6, .value = DELETE },
    { .str = "CONNECT", .strLen = 7, .value = CONNECT },
    { .str = "HEAD", .strLen = 4, .value = HEAD },
    { .str = "OPTIONS", .strLen = 7, .value = OPTIONS },
    { .str = "TRACE", .strLen = 5, .value = TRACE }
};
#define METHOD_COUNT 9

/// ==================== ========= ====================
/// ==================== FUNCTIONS ====================
/// ==================== ========= ====================

HttpMethod MethodFromString(const char *str, uint16_t strLen) {
    if (strLen == 0) {
        return UNKNOWN;
    }

    for (size_t i = 0; i < METHOD_COUNT; ++i) {
        const MethodMapping mapping = METHODS_MAP[i];
        if (mapping.strLen == strLen &&
            strncmp(mapping.str, str, strLen) == 0) {
            return mapping.value;
        }
    }

    return UNKNOWN;
}

RequestStartLine RequestStartLine_Init(void) {
    return (RequestStartLine) {
        .method = UNKNOWN,
        .targetPath = NKString_Empty(),
        .params = NULL,
        .paramsCount = 0,
        .protocol = NKString_Empty();
    };
}

void RequestStartLine_Free(RequestStartLine *rsl) {
    if (!rsl) {
        return;
    }

    rsl->method = UNKNOWN;
    NKString_Free(rsl->targetPath);

    if (rsl->params) {
        for (uint16_t i = 0; i < rsl->paramsCount; ++i) {
            NKStringPair_Free(rsl->params[i]);
        }
        free(rsl->params);
        rsl->params = NULL;
    }
    rsl->paramsCount = 0;

    NKString_Free(rsl->protocol);
}

HttpRequest HttpRequest_Init(void) {
    return (HttpRequest) {
        .startLine = RequestStartLine_Init(),
        .headers = NULL,
        .headersCount = 0,
        .body = NKBuffer_Empty(),
    };
}

void HttpRequest_Free(HttpRequest *req) {
    if (!req) {
        return;
    }

    RequestStartLine_Free(req->startLine);
    
    if (req->headers) {
        for (uint16_t i = 0; i < req->headersCount; ++i) {
            NKStringPair_Free(req->headers[i]);
        }
        req->headers = NULL;
    }
    req->headersCount = 0;

    NKBuffer_Free(req->body);
    req->body = NKBuffer_Empty();
}