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
} HttpRequest;

void Request_Free(HttpRequest *req);

#endif // NAZARK_INCLUDE_H
