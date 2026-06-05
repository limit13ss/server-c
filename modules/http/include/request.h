#ifndef NAZARK_INCLUDE_H
#define NAZARK_INCLUDE_H

#include "common.h"
#include <stdint.h>

typedef struct {
    NKString *params;
    uint16_t count;
} RequestParamArray;

typedef struct {
    NKString key;
    NKString value;
} RequestHeader;

typedef struct {
    RequestHeader *headers;
    uint16_t count;
} RequestHeaderArray;

typedef enum {
    GET     = 0,
    POST    = 1,
    PUT     = 2,
    PATCH   = 3,
    DELETE  = 4,
    CONNECT = 5,
    HEAD    = 6,
    OPTIONS = 7,
    TRACE   = 8
} HttpMethod;

typedef struct {
    HttpMethod method;
    NKString targetPath;
    RequestParamArray params;
    RequestHeaderArray headers;
    NKString body;
} HttpRequestData;

/// ===================== =========== =====================
/// ===================== HttpRequest =====================
/// ===================== =========== =====================

typedef struct HttpRequest HttpRequest_t;

int32_t HttpRequest_TryParseStartLine(HttpRequest_t *req, uint8_t *data,
                                      uint32_t startPos, uint32_t endPos);
int32_t HttpRequest_TryParseHeaders(HttpRequest_t *req, uint8_t *data,
                                    uint32_t startPos, uint32_t endPos);
int32_t HttpRequest_AppendBody(HttpRequest_t *req, uint8_t *data,
                               uint32_t startPos, uint32_t endPos);

int32_t HttpRequest_StartLineComplete(HttpRequest_t *req);
int32_t HttpRequest_HeadersComplete(HttpRequest_t *req);
int32_t HttpRequest_RequestComplete(HttpRequest_t *req);

NKString HttpRequest_GetStartLine(HttpRequest_t *req);
NKString HttpRequest_GetHeaders(HttpRequest_t *req);
NKString HttpRequest_GetBody(HttpRequest_t *req);

HttpMethod HttpRequest_GetMethod(HttpRequest_t *req);
NKString HttpRequest_GetTargetPath(HttpRequest_t *req);
RequestParamArray HttpRequest_GetParams(HttpRequest_t *req);

HttpRequestData HttpRequest_GetAllData(HttpRequest_t *req);

#endif // NAZARK_INCLUDE_H
