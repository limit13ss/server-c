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

typedef struct {
    NKString key;
    NKString value;
} KeyValuePair;

typedef struct {
    KeyValuePair *values;
    uint16_t count;
} RequestParamArray;

RequestParamArray RequestParamArray_Empty(void);
int8_t RequestParamArray_IsEmpty(RequestParamArray value);

typedef struct {
    KeyValuePair *values;
    uint16_t count;
} RequestHeaderArray;

RequestHeaderArray RequestHeaderArray_Empty(void);
int8_t RequestHeaderArray_IsEmpty(RequestHeaderArray value);

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
    RequestParamArray params;
    NKString protocol;
} RequestStartLine;

typedef struct {
    RequestStartLine startLine;
    RequestHeaderArray headers;
    NKString body;
} HttpRequest;

/// ==================== ========= ====================
/// ==================== FUNCTIONS ====================
/// ==================== ========= ====================

HttpRequest *Request_Init(void);
void Request_Free(HttpRequest *req);
HttpMethod MethodFromString(const char *str, uint16_t strLen);

#endif // NAZARK_INCLUDE_H
