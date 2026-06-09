#ifndef NAZARK_INCLUDE_H
#define NAZARK_INCLUDE_H

#include "client.h"
#include "common.h"

#include <stdint.h>

/// ==================== ======= ====================
/// ==================== REQUEST ====================
/// ==================== ======= ====================

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

HttpRequest *Request_Init(void);

/// ==================== =============== ====================
/// ==================== REQUEST PARSING ====================
/// ==================== =============== ====================

int32_t Request_TryParseBuffer(ClientContext *context);

#endif // NAZARK_INCLUDE_H
