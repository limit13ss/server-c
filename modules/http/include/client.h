#ifndef NAZARK_CLIENT_H
#define NAZARK_CLIENT_H

#include "request.h"

#include <stdint.h>

#define REQUEST_HEADERS_BUFFER_SIZE 8192 // 8 KiB

typedef enum {
    Empty             = 0,
    AwaitingStartLine = 1,
    AwaitingHeaders   = 2,
    AwaitingBody      = 3,
    Complete          = 4,
    GenericError      = 5,
    LongHeadersError  = 6,
    UnknownError      = -1
} ClientParsingState;

typedef struct {
    uint8_t *buffer;
    uint32_t capacity;
    uint32_t length;
} ClientBuffer;

typedef struct ClientContext ClientContext;

ClientContext *Client_InitContext(void);
void Client_FreeContext(ClientContext *ctx);

ClientBuffer *Client_GetBuffer(ClientContext *ctx);

/// ==================== =============== ====================
/// ==================== REQUEST PARSING ====================
/// ==================== =============== ====================

ClientParsingState Request_TryParseHeaders(ClientContext *ctx);
HttpRequest *Client_GetRequest(ClientContext *ctx);

#endif // NAZARK_CLIENT_H
