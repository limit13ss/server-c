#ifndef NAZARK_CLIENT_H
#define NAZARK_CLIENT_H

#include <stdint.h>

#define REQUEST_HEADERS_BUFFER_SIZE 8192 // 8 KiB

typedef enum {
    Empty            = 0,
    AwaitingHeaders  = 1,
    AwaitingBody     = 2,
    Complete         = 3,
    GenericError     = 4,
    LongHeadersError = 5,
} ClientParsingState;

typedef struct {
    uint8_t *buffer;
    uint32_t capacity;
    uint32_t length;
} ClientBuffer;

typedef struct {
    ClientBuffer *clientBuffer;
    ClientParsingState state;
    int32_t lastParsedByte;
    uint32_t bodyExpectedLength;
} ClientContext;

ClientContext *Client_InitContext(void);
void Client_FreeContext(ClientContext *ctx);

#endif // NAZARK_CLIENT_H
