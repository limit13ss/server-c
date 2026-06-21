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

void freeParamArray(RequestParamArray arr) {
    if (!arr.values) {
        return;
    }

    for (size_t i = 0; i < arr.count; ++i) {
        KeyValuePair *pair = &arr.values[i];
        NKString_Free(pair->key);
        NKString_Free(pair->value);
        pair->key   = NKString_Empty();
        pair->value = NKString_Empty();
    }
}

void freeHeaderArray(RequestHeaderArray arr) {
    if (!arr.values) {
        return;
    }

    for (size_t i = 0; i < arr.count; ++i) {
        NKString_Free(arr.values[i].key);
        NKString_Free(arr.values[i].value);
    }
}

/// ==================== =============== ====================
/// ==================== Request Methods ====================
/// ==================== =============== ====================

RequestParamArray RequestParamArray_Empty(void) {
    return (RequestParamArray){ .values = NULL, .count = 0 };
}
int8_t RequestParamArray_IsEmpty(RequestParamArray value) {
    return !value.values || !value.count;
}

RequestHeaderArray RequestHeaderArray_Empty(void) {
    return (RequestHeaderArray){ .values = NULL, .count = 0 };
}
int8_t RequestHeaderArray_IsEmpty(RequestHeaderArray value) {
    return !value.values || !value.count;
}

HttpRequest *Request_Init(void) {
    HttpRequest *request = NULL;

    request = calloc(1, sizeof(HttpRequest));
    if (!request) {
        return NULL;
    }

    request->startLine =
        (RequestStartLine){ .method     = UNKNOWN,
                            .targetPath = NKString_Empty(),
                            .params     = RequestParamArray_Empty(),
                            .protocol   = NKString_Empty() };
    request->headers = RequestHeaderArray_Empty();
    request->body    = NKString_Empty();

    return request;
}

void Request_Free(HttpRequest *req) {
    if (!req) {
        return;
    }

    NKString_Free(req->startLine.targetPath);
    freeParamArray(req->startLine.params);
    NKString_Free(req->startLine.protocol);
    req->startLine.targetPath = NKString_Empty();
    req->startLine.params     = RequestParamArray_Empty();
    req->startLine.protocol   = NKString_Empty();
    req->startLine.method     = UNKNOWN;

    freeHeaderArray(req->headers);
    NKString_Free(req->body);
    req->headers = RequestHeaderArray_Empty();
    req->body    = NKString_Empty();
}

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
