#ifndef NAZARK_INCLUDE_H
#define NAZARK_INCLUDE_H

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

int32_t HttpRequest_Init(HttpRequest *req);

/// ==================== =============== ====================
/// ==================== REQUEST PARSING ====================
/// ==================== =============== ====================

typedef enum {
    Empty           = 0,
    Error           = 1,
    AwaitingHeaders = 2,
    AwaitingBody    = 3,
    Complete        = 4,
} HttpRequestParsingState;

typedef struct {
    HttpRequestParsingState state;
    uint8_t *buffer;
    uint32_t bufferLength;
    int32_t lastProcessedByte;
} HttpRequestStateInfo;

int32_t TryParseHttpRequest(uint8_t *buffer, uint32_t bufferLength,
                            HttpRequestStateInfo *info);

#endif // NAZARK_INCLUDE_H
