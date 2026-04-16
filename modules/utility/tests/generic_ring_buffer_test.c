#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define RB_TYPE int32_t
#define RB_TYPE_NAME i32
#include "generic_ring_buffer.h"
#undef RB_TYPE
#undef RB_TYPE_NAME

void testCreation(void) {
    uint32_t cap        = 100;
    RingBuffer_i32_t rb = RingBuffer_i32_Create(cap);

    assert(RingBuffer_i32_Capacity(&rb) == cap);
    assert(RingBuffer_i32_IsEmpty(&rb));

    RingBuffer_i32_Free(&rb);
}

void testAppend(void) {
    uint32_t cap        = 3;
    RingBuffer_i32_t rb = RingBuffer_i32_Create(cap);

    assert(RingBuffer_i32_Append(&rb, 1));
    assert(RingBuffer_i32_Length(&rb) == 1);

    RingBuffer_i32_Append(&rb, 2);
    RingBuffer_i32_Append(&rb, 3);

    assert(!RingBuffer_i32_Append(&rb, 0));
    assert(RingBuffer_i32_Length(&rb) == cap);
    assert(!RingBuffer_i32_IsEmpty(&rb));

    for (uint32_t i = 0; i < cap; ++i) {
        int32_t v = 0;
        assert(RingBuffer_i32_GetAt(&rb, i, &v));
        assert(v == (int32_t)(i + 1));
    }

    RingBuffer_i32_Free(&rb);
}

void testPrepend(void) {
    uint32_t cap        = 3;
    RingBuffer_i32_t rb = RingBuffer_i32_Create(cap);

    assert(RingBuffer_i32_Prepend(&rb, 1));
    assert(RingBuffer_i32_Length(&rb) == 1);

    RingBuffer_i32_Prepend(&rb, 2);
    RingBuffer_i32_Prepend(&rb, 3);

    assert(!RingBuffer_i32_Prepend(&rb, 0));
    assert(RingBuffer_i32_Length(&rb) == cap);
    assert(!RingBuffer_i32_IsEmpty(&rb));

    int32_t v = 0;
    for (uint32_t i = 0; i < cap; ++i) {
        assert(RingBuffer_i32_GetAt(&rb, i, &v));
        assert(v == (int32_t)(cap - i));
    }

    RingBuffer_i32_Free(&rb);
}

void testPopFirst(void) {
    uint32_t cap        = 5;
    RingBuffer_i32_t rb = RingBuffer_i32_Create(cap);

    for (uint32_t i = 0; i < cap; ++i) {
        RingBuffer_i32_Append(&rb, (int32_t)(i + 1));
    }

    assert(RingBuffer_i32_Length(&rb) == cap);

    int32_t v = 0;
    for (uint32_t i = 0; i < cap; ++i) {
        assert(RingBuffer_i32_PopFirst(&rb, &v));
        assert(v == (int32_t)(i + 1));
    }

    assert(RingBuffer_i32_IsEmpty(&rb));
    assert(RingBuffer_i32_Length(&rb) == 0);

    RingBuffer_i32_Free(&rb);
}

void testPopLast(void) {
    uint32_t cap        = 5;
    RingBuffer_i32_t rb = RingBuffer_i32_Create(cap);

    for (uint32_t i = 0; i < cap; ++i) {
        RingBuffer_i32_Append(&rb, (int32_t)(i + 1));
    }

    assert(RingBuffer_i32_Length(&rb) == cap);

    int32_t v = 0;
    for (uint32_t i = 0; i < cap; ++i) {
        assert(RingBuffer_i32_PopLast(&rb, &v));
        assert(v == (int32_t)(cap - i));
    }

    assert(RingBuffer_i32_IsEmpty(&rb));
    assert(RingBuffer_i32_Length(&rb) == 0);

    RingBuffer_i32_Free(&rb);
}

void testGetAt(void) {
    uint32_t cap        = 5;
    RingBuffer_i32_t rb = RingBuffer_i32_Create(cap);

    for (uint32_t i = 0; i < cap; ++i) {
        RingBuffer_i32_Append(&rb, (int32_t)(i + 1));
    }

    assert(RingBuffer_i32_Length(&rb) == cap);

    int32_t v = 0;

    assert(RingBuffer_i32_GetAt(&rb, 3, &v));
    assert(v == 4);

    assert(RingBuffer_i32_GetAt(&rb, 0, &v));
    assert(v == 1);

    assert(!RingBuffer_i32_GetAt(&rb, 5, &v));

    assert(RingBuffer_i32_Length(&rb) == cap);

    RingBuffer_i32_Free(&rb);
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
