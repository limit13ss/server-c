#include "client_parser.h"
#include "test_common.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Internals of src/client_parser.c
// The struct is mirrored here so the tests can set up and inspect the parser
// state. It MUST be kept in sync with the definition in client_parser.c.
// ---------------------------------------------------------------------------

typedef struct {
    uint64_t startPos;
    uint64_t endPos;
} Range;

struct ParserContext {
    NKBuffer buffer;
    RequestParsingState state;
    uint32_t bodyExpectedLength;
    Range startLineRange;
    Range headersRange;
    HttpRequest *request;
};

int32_t findStartLine(ParserContext *ctx);
int32_t findHeaders(ParserContext *ctx);
int32_t parseStartLine(ParserContext *ctx);
int32_t parseHeaders(ParserContext *ctx);

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ParserContext *contextWith(const char *data) {
    ParserContext *ctx = ParserContext_Init();
    assert(ctx);

    size_t len = strlen(data);
    assert(len <= ctx->buffer.capacity);
    memcpy(ctx->buffer.values, data, len);
    ctx->buffer.length = (uint32_t)len;
    ctx->state         = AwaitingStartLine;

    return ctx;
}

static ParserContext *contextAfterStartLine(const char *data) {
    ParserContext *ctx = contextWith(data);

    const char *lineEnd = strstr(data, "\r\n");
    assert(lineEnd && lineEnd != data);
    uint64_t lineEndPos = (uint64_t)(lineEnd - data);

    ctx->startLineRange = (Range){ .startPos = 0, .endPos = lineEndPos - 1 };
    ctx->headersRange   = (Range){ .startPos = lineEndPos + 2, .endPos = 0 };
    ctx->state          = AwaitingHeaders;

    return ctx;
}

static ParserContext *contextAfterHeaders(const char *data) {
    ParserContext *ctx = contextAfterStartLine(data);

    const char *headersEnd = strstr(data + ctx->headersRange.startPos - 2,
                                    "\r\n\r\n");
    if (headersEnd == data + ctx->headersRange.startPos - 2) {
        // no headers at all
        ctx->headersRange = (Range){ .startPos = 0, .endPos = 0 };
    } else {
        assert(headersEnd);
        ctx->headersRange.endPos = (uint64_t)(headersEnd - data) - 1;
    }
    ctx->state = CompleteHeaders;

    return ctx;
}

// Context as Parser_TryParseRequest leaves it right before parsing.
static ParserContext *contextForParsing(const char *data) {
    ParserContext *ctx = contextAfterHeaders(data);

    ctx->request = malloc(sizeof *ctx->request);
    assert(ctx->request);
    *ctx->request = HttpRequest_Init();

    return ctx;
}

static void freeContext(ParserContext *ctx) {
    if (ctx->request) {
        HttpRequest_Free(ctx->request);
        free(ctx->request);
        ctx->request = NULL;
    }
    ParserContext_Free(ctx);
}

static bool stringEquals(NKString str, const char *expected) {
    size_t len = strlen(expected);
    if (str.len != len) {
        return false;
    }
    return len == 0 || memcmp(str.data, expected, len) == 0;
}

static bool pairEquals(const NKStringPair pair, const char *key,
                       const char *value) {
    return stringEquals(pair[0], key) && stringEquals(pair[1], value);
}

// ---------------------------------------------------------------------------
// findStartLine
// ---------------------------------------------------------------------------

void testFindStartLine_EmptyBuffer(void) {
    ParserContext *ctx = contextWith("");

    assert(findStartLine(ctx) == -1);
    assert(ctx->state == AwaitingStartLine);

    freeContext(ctx);
}

void testFindStartLine_NoSeparator(void) {
    ParserContext *ctx = contextWith("GET /index.html HTTP/1.1");

    assert(findStartLine(ctx) == -1);
    assert(ctx->state == AwaitingStartLine);

    freeContext(ctx);
}

void testFindStartLine_StartLineOnly(void) {
    ParserContext *ctx = contextWith("GET /index.html HTTP/1.1\r\n");

    assert(findStartLine(ctx) == 0);
    assert(ctx->state == AwaitingHeaders);
    assert(ctx->startLineRange.startPos == 0);
    assert(ctx->startLineRange.endPos == 23);
    assert(ctx->headersRange.startPos == 26);

    freeContext(ctx);
}

void testFindStartLine_StopsAtFirstSeparator(void) {
    ParserContext *ctx = contextWith("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");

    assert(findStartLine(ctx) == 0);
    assert(ctx->state == AwaitingHeaders);
    assert(ctx->startLineRange.startPos == 0);
    assert(ctx->startLineRange.endPos == 13);
    assert(ctx->headersRange.startPos == 16);

    freeContext(ctx);
}

void testFindStartLine_DataArrivesInChunks(void) {
    ParserContext *ctx = contextWith("GET / HTTP/1.1\r\n");

    ctx->buffer.length = 9; // "GET / HTT"
    assert(findStartLine(ctx) == -1);
    assert(ctx->state == AwaitingStartLine);

    ctx->buffer.length = 15; // "GET / HTTP/1.1\r"
    assert(findStartLine(ctx) == -1);
    assert(ctx->state == AwaitingStartLine);

    ctx->buffer.length = 16; // "GET / HTTP/1.1\r\n"
    assert(findStartLine(ctx) == 0);
    assert(ctx->state == AwaitingHeaders);
    assert(ctx->startLineRange.startPos == 0);
    assert(ctx->startLineRange.endPos == 13);
    assert(ctx->headersRange.startPos == 16);

    freeContext(ctx);
}

void testFindStartLine_LeadingEmptyLine_RangesStayInBuffer(void) {
    ParserContext *ctx = contextWith("\r\nGET / HTTP/1.1\r\n");

    if (findStartLine(ctx) == 0) {
        assert(ctx->startLineRange.endPos < ctx->buffer.length);
        assert(ctx->headersRange.startPos <= ctx->buffer.length);
    }

    freeContext(ctx);
}

// ---------------------------------------------------------------------------
// findHeaders
// ---------------------------------------------------------------------------

void testFindHeaders_EmptyBuffer(void) {
    ParserContext *ctx = contextAfterStartLine("GET / HTTP/1.1\r\n");
    ctx->buffer.length = 0;

    assert(findHeaders(ctx) == -1);

    freeContext(ctx);
}

void testFindHeaders_StartLineNotFound(void) {
    ParserContext *ctx = contextWith("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ctx->state         = AwaitingHeaders;

    assert(findHeaders(ctx) == -1);
    assert(ctx->state == BadRequestError);

    freeContext(ctx);
}

void testFindHeaders_HeadersStartNotSet(void) {
    ParserContext *ctx =
        contextAfterStartLine("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    ctx->headersRange.startPos = 0;

    assert(findHeaders(ctx) == -1);
    assert(ctx->state == BadRequestError);

    freeContext(ctx);
}

void testFindHeaders_NothingAfterStartLine(void) {
    ParserContext *ctx = contextAfterStartLine("GET / HTTP/1.1\r\n");

    assert(findHeaders(ctx) == 0);
    assert(ctx->state == AwaitingHeaders);

    freeContext(ctx);
}

void testFindHeaders_NoHeaders(void) {
    ParserContext *ctx = contextAfterStartLine("GET / HTTP/1.1\r\n\r\n");

    assert(findHeaders(ctx) == 0);
    assert(ctx->state == Complete);
    assert(ctx->headersRange.startPos == 0);
    assert(ctx->headersRange.endPos == 0);

    freeContext(ctx);
}

void testFindHeaders_SingleHeader(void) {
    ParserContext *ctx =
        contextAfterStartLine("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");

    assert(findHeaders(ctx) == 0);
    assert(ctx->state == CompleteHeaders);
    assert(ctx->headersRange.startPos == 16);
    assert(ctx->headersRange.endPos == 30);

    freeContext(ctx);
}

void testFindHeaders_MultipleHeaders(void) {
    ParserContext *ctx = contextAfterStartLine(
        "GET / HTTP/1.1\r\nHost: localhost\r\nAccept: */*\r\n\r\n");

    assert(findHeaders(ctx) == 0);
    assert(ctx->state == CompleteHeaders);
    assert(ctx->headersRange.startPos == 16);
    assert(ctx->headersRange.endPos == 43);

    freeContext(ctx);
}

void testFindHeaders_StopsAtFirstEmptyLine(void) {
    // the body contains "\r\n\r\n" as well
    ParserContext *ctx = contextAfterStartLine(
        "POST / HTTP/1.1\r\nContent-Length: 6\r\n\r\na\r\n\r\nb");

    assert(findHeaders(ctx) == 0);
    assert(ctx->state == CompleteHeaders);
    assert(ctx->headersRange.startPos == 17);
    assert(ctx->headersRange.endPos == 33);

    freeContext(ctx);
}

void testFindHeaders_DataArrivesInChunks(void) {
    // bytes past buffer.length were not received yet and must be ignored
    ParserContext *ctx =
        contextAfterStartLine("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");

    ctx->buffer.length = 17; // "...\r\nH"
    assert(findHeaders(ctx) == 0);
    assert(ctx->state == AwaitingHeaders);

    ctx->buffer.length = 20; // "...\r\nHost"
    assert(findHeaders(ctx) == 0);
    assert(ctx->state == AwaitingHeaders);

    ctx->buffer.length = 25; // "...\r\nHost: loc"
    assert(findHeaders(ctx) == 0);
    assert(ctx->state == AwaitingHeaders);

    ctx->buffer.length = 35; // "...\r\nHost: localhost\r\n\r\n"
    assert(findHeaders(ctx) == 0);
    assert(ctx->state == CompleteHeaders);
    assert(ctx->headersRange.startPos == 16);
    assert(ctx->headersRange.endPos == 30);

    freeContext(ctx);
}

void testFindHeaders_BufferFull(void) {
    ParserContext *ctx = contextAfterStartLine("GET / HTTP/1.1\r\nX-Long: ");
    NKBuffer *buf      = &ctx->buffer;
    memset(buf->values + buf->length, 'a', buf->capacity - buf->length);
    buf->length = buf->capacity;

    assert(findHeaders(ctx) == -1);
    assert(ctx->state == LongHeadersError);

    freeContext(ctx);
}

void testFindHeaders_BufferFullWithCompleteHeaders(void) {
    ParserContext *ctx = contextAfterStartLine("GET / HTTP/1.1\r\nX-Long: ");
    NKBuffer *buf      = &ctx->buffer;
    memset(buf->values + buf->length, 'a', buf->capacity - buf->length);
    memcpy(buf->values + buf->capacity - 4, "\r\n\r\n", 4);
    buf->length = buf->capacity;

    assert(findHeaders(ctx) == 0);
    assert(ctx->state == CompleteHeaders);
    assert(ctx->headersRange.startPos == 16);
    assert(ctx->headersRange.endPos == buf->capacity - 5);

    freeContext(ctx);
}

// ---------------------------------------------------------------------------
// parseStartLine
// ---------------------------------------------------------------------------

static void assertParseStartLineFails(const char *data) {
    ParserContext *ctx = contextForParsing(data);
    assert(parseStartLine(ctx) == -1);
    freeContext(ctx);
}

void testParseStartLine_NoRequest(void) {
    ParserContext *ctx = contextAfterHeaders("GET / HTTP/1.1\r\n\r\n");
    assert(ctx->request == NULL);

    assert(parseStartLine(ctx) == -1);

    freeContext(ctx);
}

void testParseStartLine_EmptyRange(void) {
    ParserContext *ctx  = contextForParsing("GET / HTTP/1.1\r\n\r\n");
    ctx->startLineRange = (Range){ .startPos = 0, .endPos = 0 };

    assert(parseStartLine(ctx) == -1);

    freeContext(ctx);
}

void testParseStartLine_Simple(void) {
    ParserContext *ctx = contextForParsing("GET /index.html HTTP/1.0\r\n\r\n");

    assert(parseStartLine(ctx) == 0);

    RequestStartLine *line = &ctx->request->startLine;
    assert(line->method == GET);
    assert(stringEquals(line->targetPath, "/index.html"));
    assert(line->params == NULL);
    assert(line->paramsCount == 0);
    assert(stringEquals(line->protocol, "HTTP/1.0"));

    freeContext(ctx);
}

void testParseStartLine_WithHeaders(void) {
    ParserContext *ctx = contextForParsing(
        "POST /api/users HTTP/1.1\r\nHost: localhost\r\nContent-Length: "
        "0\r\n\r\n");

    assert(parseStartLine(ctx) == 0);

    RequestStartLine *line = &ctx->request->startLine;
    assert(line->method == POST);
    assert(stringEquals(line->targetPath, "/api/users"));
    assert(line->params == NULL);
    assert(line->paramsCount == 0);
    assert(stringEquals(line->protocol, "HTTP/1.1"));

    freeContext(ctx);
}

void testParseStartLine_AllMethods(void) {
    static const struct {
        const char *str;
        HttpMethod value;
    } methods[] = {
        { "GET", GET },         { "POST", POST },       { "PUT", PUT },
        { "PATCH", PATCH },     { "DELETE", DELETE },   { "CONNECT", CONNECT },
        { "HEAD", HEAD },       { "OPTIONS", OPTIONS }, { "TRACE", TRACE },
    };

    for (size_t i = 0; i < sizeof methods / sizeof methods[0]; ++i) {
        char data[64];
        snprintf(data, sizeof data, "%s / HTTP/1.1\r\n\r\n", methods[i].str);
        ParserContext *ctx = contextForParsing(data);

        assert(parseStartLine(ctx) == 0);
        assert(ctx->request->startLine.method == methods[i].value);

        freeContext(ctx);
    }
}

void testParseStartLine_UnknownMethod(void) {
    assertParseStartLineFails("FETCH / HTTP/1.1\r\n\r\n");
    assertParseStartLineFails("get / HTTP/1.1\r\n\r\n");
}

void testParseStartLine_MissingTargetAndProtocol(void) {
    assertParseStartLineFails("GET\r\n\r\n");
}

void testParseStartLine_MissingProtocol(void) {
    assertParseStartLineFails("GET /index.html\r\n\r\n");
    assertParseStartLineFails("GET /index.html\r\nHost: localhost\r\n\r\n");
}

void testParseStartLine_SingleParam(void) {
    ParserContext *ctx = contextForParsing("GET /search?q=http HTTP/1.1\r\n\r\n");

    assert(parseStartLine(ctx) == 0);

    RequestStartLine *line = &ctx->request->startLine;
    assert(line->method == GET);
    assert(stringEquals(line->targetPath, "/search"));
    assert(line->paramsCount == 1);
    assert(line->params);
    assert(pairEquals(line->params[0], "q", "http"));
    assert(stringEquals(line->protocol, "HTTP/1.1"));

    freeContext(ctx);
}

void testParseStartLine_MultipleParams(void) {
    ParserContext *ctx = contextForParsing(
        "GET /search?q=http&lang=en&page=2 HTTP/1.1\r\nHost: "
        "localhost\r\n\r\n");

    assert(parseStartLine(ctx) == 0);

    RequestStartLine *line = &ctx->request->startLine;
    assert(stringEquals(line->targetPath, "/search"));
    assert(line->paramsCount == 3);
    assert(line->params);
    assert(pairEquals(line->params[0], "q", "http"));
    assert(pairEquals(line->params[1], "lang", "en"));
    assert(pairEquals(line->params[2], "page", "2"));
    assert(stringEquals(line->protocol, "HTTP/1.1"));

    freeContext(ctx);
}

void testParseStartLine_ParamsWithoutValue(void) {
    ParserContext *ctx =
        contextForParsing("GET /items?verbose&filter=&page=2 HTTP/1.1\r\n\r\n");

    assert(parseStartLine(ctx) == 0);

    RequestStartLine *line = &ctx->request->startLine;
    assert(stringEquals(line->targetPath, "/items"));
    assert(line->paramsCount == 3);
    assert(line->params);
    assert(pairEquals(line->params[0], "verbose", ""));
    assert(pairEquals(line->params[1], "filter", ""));
    assert(pairEquals(line->params[2], "page", "2"));
    assert(stringEquals(line->protocol, "HTTP/1.1"));

    freeContext(ctx);
}

void testParseStartLine_EmptyQuery(void) {
    ParserContext *ctx = contextForParsing("GET /items? HTTP/1.1\r\n\r\n");

    assert(parseStartLine(ctx) == 0);

    RequestStartLine *line = &ctx->request->startLine;
    assert(stringEquals(line->targetPath, "/items"));
    assert(line->params == NULL);
    assert(line->paramsCount == 0);
    assert(stringEquals(line->protocol, "HTTP/1.1"));

    freeContext(ctx);
}

void testParseStartLine_QueryStaysInsideStartLine(void) {
    // the '&' in the header must not be treated as a parameter separator
    ParserContext *ctx = contextForParsing(
        "GET /items?a=1 HTTP/1.1\r\nCookie: x=1&y=2\r\n\r\n");

    assert(parseStartLine(ctx) == 0);

    RequestStartLine *line = &ctx->request->startLine;
    assert(stringEquals(line->targetPath, "/items"));
    assert(line->paramsCount == 1);
    assert(line->params);
    assert(pairEquals(line->params[0], "a", "1"));
    assert(stringEquals(line->protocol, "HTTP/1.1"));

    freeContext(ctx);
}

// ---------------------------------------------------------------------------
// parseHeaders
// NOTE: parseHeaders is still a TODO. Apart from testParseHeaders_NoRequest
// these tests describe the expected behavior (RFC 9112, section 5) and fail
// until it is implemented.
// ---------------------------------------------------------------------------

static void assertParseHeadersFails(const char *data) {
    ParserContext *ctx = contextForParsing(data);
    assert(parseHeaders(ctx) == -1);
    freeContext(ctx);
}

void testParseHeaders_NoRequest(void) {
    ParserContext *ctx =
        contextAfterHeaders("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");
    assert(ctx->request == NULL);

    assert(parseHeaders(ctx) == -1);
    assert(ctx->state == BadRequestError);

    freeContext(ctx);
}

void testParseHeaders_SingleHeader(void) {
    ParserContext *ctx =
        contextForParsing("GET / HTTP/1.1\r\nHost: localhost\r\n\r\n");

    assert(parseHeaders(ctx) == 0);
    assert(ctx->request->headersCount == 1);
    assert(ctx->request->headers);
    assert(pairEquals(ctx->request->headers[0], "Host", "localhost"));

    freeContext(ctx);
}

void testParseHeaders_MultipleHeaders_KeepOrder(void) {
    ParserContext *ctx = contextForParsing(
        "GET / HTTP/1.1\r\nHost: localhost\r\nAccept: */*\r\nConnection: "
        "close\r\n\r\n");

    assert(parseHeaders(ctx) == 0);
    assert(ctx->request->headersCount == 3);
    assert(ctx->request->headers);
    assert(pairEquals(ctx->request->headers[0], "Host", "localhost"));
    assert(pairEquals(ctx->request->headers[1], "Accept", "*/*"));
    assert(pairEquals(ctx->request->headers[2], "Connection", "close"));

    freeContext(ctx);
}

void testParseHeaders_ValueWhitespaceTrimmed(void) {
    ParserContext *ctx = contextForParsing(
        "GET / HTTP/1.1\r\nHost:localhost\r\nX-Padded: \t value \t\r\n\r\n");

    assert(parseHeaders(ctx) == 0);
    assert(ctx->request->headersCount == 2);
    assert(ctx->request->headers);
    assert(pairEquals(ctx->request->headers[0], "Host", "localhost"));
    assert(pairEquals(ctx->request->headers[1], "X-Padded", "value"));

    freeContext(ctx);
}

void testParseHeaders_ValueContainsColon(void) {
    ParserContext *ctx =
        contextForParsing("GET / HTTP/1.1\r\nHost: localhost:8080\r\n\r\n");

    assert(parseHeaders(ctx) == 0);
    assert(ctx->request->headersCount == 1);
    assert(ctx->request->headers);
    assert(pairEquals(ctx->request->headers[0], "Host", "localhost:8080"));

    freeContext(ctx);
}

void testParseHeaders_EmptyValue(void) {
    ParserContext *ctx = contextForParsing(
        "GET / HTTP/1.1\r\nX-Empty:\r\nHost: localhost\r\n\r\n");

    assert(parseHeaders(ctx) == 0);
    assert(ctx->request->headersCount == 2);
    assert(ctx->request->headers);
    assert(pairEquals(ctx->request->headers[0], "X-Empty", ""));
    assert(pairEquals(ctx->request->headers[1], "Host", "localhost"));

    freeContext(ctx);
}

void testParseHeaders_IgnoresBody(void) {
    ParserContext *ctx = contextForParsing(
        "POST / HTTP/1.1\r\nContent-Length: 12\r\n\r\nNot: aheader");

    assert(parseHeaders(ctx) == 0);
    assert(ctx->request->headersCount == 1);
    assert(ctx->request->headers);
    assert(pairEquals(ctx->request->headers[0], "Content-Length", "12"));

    freeContext(ctx);
}

void testParseHeaders_MissingColon(void) {
    assertParseHeadersFails("GET / HTTP/1.1\r\nHost localhost\r\n\r\n");
}

void testParseHeaders_WhitespaceBeforeColon(void) {
    assertParseHeadersFails("GET / HTTP/1.1\r\nHost : localhost\r\n\r\n");
}

void testParseHeaders_EmptyName(void) {
    assertParseHeadersFails("GET / HTTP/1.1\r\n: localhost\r\n\r\n");
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void) {
    RUN(testFindStartLine_EmptyBuffer);
    RUN(testFindStartLine_NoSeparator);
    RUN(testFindStartLine_StartLineOnly);
    RUN(testFindStartLine_StopsAtFirstSeparator);
    RUN(testFindStartLine_DataArrivesInChunks);
    RUN(testFindStartLine_LeadingEmptyLine_RangesStayInBuffer);

    RUN(testFindHeaders_EmptyBuffer);
    RUN(testFindHeaders_StartLineNotFound);
    RUN(testFindHeaders_HeadersStartNotSet);
    RUN(testFindHeaders_NothingAfterStartLine);
    RUN(testFindHeaders_NoHeaders);
    RUN(testFindHeaders_SingleHeader);
    RUN(testFindHeaders_MultipleHeaders);
    RUN(testFindHeaders_StopsAtFirstEmptyLine);
    RUN(testFindHeaders_DataArrivesInChunks);
    RUN(testFindHeaders_BufferFull);
    RUN(testFindHeaders_BufferFullWithCompleteHeaders);

    RUN(testParseStartLine_NoRequest);
    RUN(testParseStartLine_EmptyRange);
    RUN(testParseStartLine_Simple);
    RUN(testParseStartLine_WithHeaders);
    RUN(testParseStartLine_AllMethods);
    RUN(testParseStartLine_UnknownMethod);
    RUN(testParseStartLine_MissingTargetAndProtocol);
    RUN(testParseStartLine_MissingProtocol);
    RUN(testParseStartLine_SingleParam);
    RUN(testParseStartLine_MultipleParams);
    RUN(testParseStartLine_ParamsWithoutValue);
    RUN(testParseStartLine_EmptyQuery);
    RUN(testParseStartLine_QueryStaysInsideStartLine);

    RUN(testParseHeaders_NoRequest);
    RUN(testParseHeaders_SingleHeader);
    RUN(testParseHeaders_MultipleHeaders_KeepOrder);
    RUN(testParseHeaders_ValueWhitespaceTrimmed);
    RUN(testParseHeaders_ValueContainsColon);
    RUN(testParseHeaders_EmptyValue);
    RUN(testParseHeaders_IgnoresBody);
    RUN(testParseHeaders_MissingColon);
    RUN(testParseHeaders_WhitespaceBeforeColon);
    RUN(testParseHeaders_EmptyName);

    printf("\nAll tests passed.\n");
    return 0;
}
