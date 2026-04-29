#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#ifndef NKRYLOV_RING_BUFFER_I32_H
#define NKRYLOV_RING_BUFFER_I32_H
#define RB_TYPE int32_t
#define RB_TYPE_NAME i32
#include "generic_ring_buffer.h"
#undef RB_TYPE
#undef RB_TYPE_NAME
#endif // NKRYLOV_RING_BUFFER_I32_H

void testCreation(void) {
    uint64_t cap         = 100;
    RingBuffer_i32_t *rb = RingBuffer_i32_Create(cap);

    uint64_t realCap = 0;
    bool isEmpty     = false;

    assert(RingBuffer_i32_Capacity(rb, &realCap));
    assert(realCap == cap);
    assert(RingBuffer_i32_IsEmpty(rb, &isEmpty));
    assert(isEmpty);

    RingBuffer_i32_Free(rb);
}

void testAppend(void) {
    uint64_t cap         = 3;
    RingBuffer_i32_t *rb = RingBuffer_i32_Create(cap);

    assert(RingBuffer_i32_Append(rb, 1));

    uint64_t realLength = 0;
    bool isEmpty        = false;

    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == 1);

    RingBuffer_i32_Append(rb, 2);
    RingBuffer_i32_Append(rb, 3);

    assert(!RingBuffer_i32_Append(rb, 0));
    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == cap);

    assert(RingBuffer_i32_IsEmpty(rb, &isEmpty));
    assert(!isEmpty);

    for (uint64_t i = 0; i < cap; ++i) {
        int32_t v = 0;
        assert(RingBuffer_i32_GetAt(rb, i, &v));
        assert(v == (int32_t)(i + 1));
    }

    RingBuffer_i32_Free(rb);
}

void testPrepend(void) {
    uint64_t cap         = 3;
    RingBuffer_i32_t *rb = RingBuffer_i32_Create(cap);

    uint64_t realLength = 0;
    bool isEmpty        = false;

    assert(RingBuffer_i32_Prepend(rb, 1));
    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == 1);

    RingBuffer_i32_Prepend(rb, 2);
    RingBuffer_i32_Prepend(rb, 3);

    assert(!RingBuffer_i32_Prepend(rb, 0));
    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == cap);
    assert(RingBuffer_i32_IsEmpty(rb, &isEmpty));
    assert(!isEmpty);

    int32_t v = 0;
    for (uint64_t i = 0; i < cap; ++i) {
        assert(RingBuffer_i32_GetAt(rb, i, &v));
        assert(v == (int32_t)(cap - i));
    }

    RingBuffer_i32_Free(rb);
}

void testPopFirst(void) {
    uint64_t cap         = 5;
    RingBuffer_i32_t *rb = RingBuffer_i32_Create(cap);

    for (uint64_t i = 0; i < cap; ++i) {
        RingBuffer_i32_Append(rb, (int32_t)(i + 1));
    }

    uint64_t realLength = 0;
    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == cap);

    int32_t v = 0;
    for (uint64_t i = 0; i < cap; ++i) {
        assert(RingBuffer_i32_PopFirst(rb, &v));
        assert(v == (int32_t)(i + 1));
    }

    bool isEmpty = false;
    assert(RingBuffer_i32_IsEmpty(rb, &isEmpty));
    assert(isEmpty);
    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == 0);

    RingBuffer_i32_Free(rb);
}

void testPopLast(void) {
    uint64_t cap         = 5;
    RingBuffer_i32_t *rb = RingBuffer_i32_Create(cap);

    for (uint64_t i = 0; i < cap; ++i) {
        RingBuffer_i32_Append(rb, (int32_t)(i + 1));
    }

    uint64_t realLength = 0;
    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == cap);

    int32_t v = 0;
    for (uint64_t i = 0; i < cap; ++i) {
        assert(RingBuffer_i32_PopLast(rb, &v));
        assert(v == (int32_t)(cap - i));
    }

    bool isEmpty = false;
    assert(RingBuffer_i32_IsEmpty(rb, &isEmpty));
    assert(isEmpty);
    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == 0);

    RingBuffer_i32_Free(rb);
}

void testGetAt(void) {
    uint64_t cap         = 5;
    RingBuffer_i32_t *rb = RingBuffer_i32_Create(cap);

    for (uint64_t i = 0; i < cap; ++i) {
        RingBuffer_i32_Append(rb, (int32_t)(i + 1));
    }

    uint64_t realLength = 0;
    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == cap);

    int32_t v = 0;

    assert(RingBuffer_i32_GetAt(rb, 3, &v));
    assert(v == 4);

    assert(RingBuffer_i32_GetAt(rb, 0, &v));
    assert(v == 1);

    assert(!RingBuffer_i32_GetAt(rb, 5, &v));

    assert(RingBuffer_i32_Length(rb, &realLength));
    assert(realLength == cap);

    RingBuffer_i32_Free(rb);
}

int main(void) {

    testCreation();
    testAppend();
    testPrepend();
    testPopFirst();
    testPopLast();
    testGetAt();

    printf("All tests passed!\n");

    return 0;
}
