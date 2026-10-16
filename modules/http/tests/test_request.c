#include "request.h"
#include "test_common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Heap-allocated, non-empty string (independent of NKString_CopyFrom).
static NKString makeString(const char *str) {
    size_t length    = strlen(str);
    uint8_t *data = malloc(length);
    assert(data);
    memcpy(data, str, length);
    return (NKString){ .data = data, .length = length };
}

static HttpMethod methodOf(const char *str) {
    return MethodFromString(str, (uint16_t)strlen(str));
}

static void assertStartLineIsInitial(const RequestStartLine *rsl) {
    assert(rsl->method == UNKNOWN);
    assert(rsl->targetPath.data == NULL);
    assert(rsl->targetPath.length == 0);
    assert(rsl->params == NULL);
    assert(rsl->paramsCount == 0);
    assert(rsl->protocol.data == NULL);
    assert(rsl->protocol.length == 0);
}

static void assertRequestIsInitial(const HttpRequest *req) {
    assertStartLineIsInitial(&req->startLine);
    assert(req->headers == NULL);
    assert(req->headersCount == 0);
    assert(req->body.values == NULL);
    assert(req->body.capacity == 0);
    assert(req->body.length == 0);
}

static RequestStartLine makePopulatedStartLine(void) {
    RequestStartLine rsl = RequestStartLine_Init();
    rsl.method           = GET;
    rsl.targetPath       = makeString("/search");
    rsl.protocol         = makeString("HTTP/1.1");

    rsl.paramsCount = 2;
    rsl.params      = calloc(rsl.paramsCount, sizeof(NKStringPair));
    assert(rsl.params);
    rsl.params[0][0] = makeString("q");
    rsl.params[0][1] = makeString("http");
    rsl.params[1][0] = makeString("flag");
    rsl.params[1][1] = NKString_Empty();

    return rsl;
}

// ---------------------------------------------------------------------------
// NKStringPair_Free
// (it has no observable effect besides freeing, run under valgrind or
//  -fsanitize=address to catch leaks / invalid frees)
// ---------------------------------------------------------------------------

void testNKStringPairFree_BothAllocated(void) {
    NKStringPair pair = { makeString("Host"), makeString("localhost") };
    NKStringPair_Free(pair);
}

void testNKStringPairFree_BothEmpty(void) {
    NKStringPair pair = { NKString_Empty(), NKString_Empty() };
    NKStringPair_Free(pair);
}

void testNKStringPairFree_KeyOnly(void) {
    NKStringPair pair = { makeString("flag"), NKString_Empty() };
    NKStringPair_Free(pair);
}

// ---------------------------------------------------------------------------
// RequestStartLine_Init / RequestStartLine_Free
// ---------------------------------------------------------------------------

void testRequestStartLineInit(void) {
    RequestStartLine rsl = RequestStartLine_Init();
    assertStartLineIsInitial(&rsl);
}

void testRequestStartLineFree_Null(void) { RequestStartLine_Free(NULL); }

void testRequestStartLineFree_Initial(void) {
    RequestStartLine rsl = RequestStartLine_Init();
    RequestStartLine_Free(&rsl);
    assertStartLineIsInitial(&rsl);
}

void testRequestStartLineFree_Populated_ResetsToInitial(void) {
    RequestStartLine rsl = makePopulatedStartLine();
    RequestStartLine_Free(&rsl);
    assertStartLineIsInitial(&rsl);
}

void testRequestStartLineFree_ParamsWithoutCount(void) {
    RequestStartLine rsl = RequestStartLine_Init();
    rsl.params           = calloc(1, sizeof(NKStringPair));
    assert(rsl.params);
    rsl.paramsCount = 0;

    RequestStartLine_Free(&rsl);
    assertStartLineIsInitial(&rsl);
}

// ---------------------------------------------------------------------------
// HttpRequest_Init / HttpRequest_Free
// ---------------------------------------------------------------------------

void testHttpRequestInit(void) {
    HttpRequest req = HttpRequest_Init();
    assertRequestIsInitial(&req);
}

void testHttpRequestFree_Null(void) { HttpRequest_Free(NULL); }

void testHttpRequestFree_Initial(void) {
    HttpRequest req = HttpRequest_Init();
    HttpRequest_Free(&req);
    assertRequestIsInitial(&req);
}

void testHttpRequestFree_Populated_ResetsToInitial(void) {
    HttpRequest req = HttpRequest_Init();
    req.startLine   = makePopulatedStartLine();

    req.headersCount = 2;
    req.headers      = calloc(req.headersCount, sizeof(NKStringPair));
    assert(req.headers);
    req.headers[0][0] = makeString("Host");
    req.headers[0][1] = makeString("localhost");
    req.headers[1][0] = makeString("Accept");
    req.headers[1][1] = makeString("*/*");

    req.body.values = calloc(16, sizeof(uint8_t));
    assert(req.body.values);
    req.body.capacity = 16;
    memcpy(req.body.values, "body", 4);
    req.body.length = 4;

    HttpRequest_Free(&req);
    assertRequestIsInitial(&req);
}

// ---------------------------------------------------------------------------
// MethodFromString
// ---------------------------------------------------------------------------

void testMethodFromString_AllMethods(void) {
    assert(methodOf("GET") == GET);
    assert(methodOf("POST") == POST);
    assert(methodOf("PUT") == PUT);
    assert(methodOf("PATCH") == PATCH);
    assert(methodOf("DELETE") == DELETE);
    assert(methodOf("CONNECT") == CONNECT);
    assert(methodOf("HEAD") == HEAD);
    assert(methodOf("OPTIONS") == OPTIONS);
    assert(methodOf("TRACE") == TRACE);
}

void testMethodFromString_ZeroLength(void) {
    assert(MethodFromString("GET", 0) == UNKNOWN);
    assert(MethodFromString("", 0) == UNKNOWN);
    assert(MethodFromString(NULL, 0) == UNKNOWN);
}

void testMethodFromString_Unknown(void) {
    assert(methodOf("FETCH") == UNKNOWN);
    assert(methodOf("GETS") == UNKNOWN);
    assert(methodOf("X") == UNKNOWN);
    assert(methodOf("CONNECTION") == UNKNOWN);
}

void testMethodFromString_CaseSensitive(void) {
    assert(methodOf("get") == UNKNOWN);
    assert(methodOf("Post") == UNKNOWN);
    assert(methodOf("dElEtE") == UNKNOWN);
}

void testMethodFromString_UsesOnlyStrLen(void) {
    // input is a slice of a larger buffer, not a null-terminated string
    assert(MethodFromString("GET /index.html HTTP/1.1", 3) == GET);
    assert(MethodFromString("POSTMAN", 4) == POST);
    assert(MethodFromString("HEADERS", 4) == HEAD);
}

void testMethodFromString_Prefix(void) {
    assert(MethodFromString("GET", 2) == UNKNOWN);
    assert(MethodFromString("DELETE", 3) == UNKNOWN);
    assert(MethodFromString("OPTIONS", 6) == UNKNOWN);
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void) {
    RUN(testNKStringPairFree_BothAllocated);
    RUN(testNKStringPairFree_BothEmpty);
    RUN(testNKStringPairFree_KeyOnly);

    RUN(testRequestStartLineInit);
    RUN(testRequestStartLineFree_Null);
    RUN(testRequestStartLineFree_Initial);
    RUN(testRequestStartLineFree_Populated_ResetsToInitial);
    RUN(testRequestStartLineFree_ParamsWithoutCount);

    RUN(testHttpRequestInit);
    RUN(testHttpRequestFree_Null);
    RUN(testHttpRequestFree_Initial);
    RUN(testHttpRequestFree_Populated_ResetsToInitial);

    RUN(testMethodFromString_AllMethods);
    RUN(testMethodFromString_ZeroLength);
    RUN(testMethodFromString_Unknown);
    RUN(testMethodFromString_CaseSensitive);
    RUN(testMethodFromString_UsesOnlyStrLen);
    RUN(testMethodFromString_Prefix);

    printf("\nAll tests passed.\n");
    return 0;
}
