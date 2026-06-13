#include "request.h"
#include "common.h"

#include <stdlib.h>

void freeParamArray(RequestParamArray arr) {
    if (!arr.values) {
        return;
    }

    for (size_t i = 0; i < arr.count; ++i) {
        NKString_Free(arr.values[i]);
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
        (RequestStartLine){ .method     = EMPTY,
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
    req->startLine.method     = EMPTY;

    freeHeaderArray(req->headers);
    NKString_Free(req->body);
    req->headers = RequestHeaderArray_Empty();
    req->body    = NKString_Empty();
}
