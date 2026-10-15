#ifndef NAZARK_INCLUDE_H
#define NAZARK_INCLUDE_H

#include "common.h"

#include <stdint.h>

/// ==================== ========= ====================
/// ==================== CONSTANTS ====================
/// ==================== ========= ====================

static const uint8_t SPACE = ' ';

static const uint8_t HTTP_SEPARATOR[] = { '\r', '\n' };
#define HTTP_SEPARATOR_LEN 2

static const uint8_t DOUBLE_HTTP_SEPARATOR[] = { '\r', '\n', '\r', '\n' };
#define DOUBLE_HTTP_SEPARATOR_LEN 4

/// ==================== ======= ====================
/// ==================== STRUCTS ====================
/// ==================== ======= ====================

typedef NKString NKStringPair[2];

typedef enum {
    GET     = 0,
    POST    = 1,
    PUT     = 2,
    PATCH   = 3,
    DELETE  = 4,
    CONNECT = 5,
    HEAD    = 6,
    OPTIONS = 7,
    TRACE   = 8,
    UNKNOWN = -1
} HttpMethod;

typedef struct {
    HttpMethod method;
    NKString targetPath;
    NKStringPair *params;
    uint16_t paramsCount;
    NKString protocol;
} RequestStartLine;

typedef struct {
    RequestStartLine startLine;
    NKStringPair *headers;
    uint16_t headersCount;
    NKBuffer body;
} HttpRequest;

/// ==================== ========= ====================
/// ==================== FUNCTIONS ====================
/// ==================== ========= ====================

void NKStringPair_Free(NKStringPair pair);

HttpRequest HttpRequest_Init(void);
void HttpRequest_Free(HttpRequest *req);

RequestStartLine RequestStartLine_Init(void);
void RequestStartLine_Free(RequestStartLine *rsl);

HttpMethod MethodFromString(const char *str, uint16_t strLen);

#endif // NAZARK_INCLUDE_H
