#include "client_parser.h"
#include <string.h>

int main(void) {
    const char *exampleRequest = "GET /index.html HTTP/1.0\r\n\r\n\0";
    ParserContext *ctx         = ParserContext_Init();

    NKBuffer *buffer = ParserContext_GetBuffer(ctx);
    size_t len       = strnlen(exampleRequest, buffer->capacity);
    strncpy((char *)buffer->values, exampleRequest, len);
    buffer->length = (uint32_t)len;

    Parser_TryParseHeaders(ctx);

    return 0;
}
