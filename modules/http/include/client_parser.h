#ifndef NAZARK_CLIENT_PARSER_H
#define NAZARK_CLIENT_PARSER_H

#include "common.h"
#include "request.h"

#include <stdint.h>

#define REQUEST_HEADERS_BUFFER_SIZE 8192 // 8 KiB

typedef enum {
    Empty             = 0,
    AwaitingStartLine = 1,
    AwaitingHeaders   = 2,
    CompleteHeaders   = 3,
    AwaitingBody      = 4,
    Complete          = 5,
    GenericError      = 6,
    LongHeadersError  = 7,
    BadRequestError   = 8,
    InternalError     = 9,
    UnknownError      = -1
} RequestParsingState;

typedef struct ParserContext ParserContext;

ParserContext *ParserContext_Init(void);
void ParserContext_Free(ParserContext *ctx);
NKBuffer *ParserContext_GetBuffer(ParserContext *ctx);

RequestParsingState Parser_TryParseRequest(ParserContext *ctx);
HttpRequest *Parser_TryGetRequest(ParserContext *ctx);

#endif // NAZARK_CLIENT_PARSER_H
