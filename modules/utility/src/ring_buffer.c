#include "ring_buffer.h"

#include <stdlib.h>

RingBuffer_t RingBuffer_Create(int32_t capacity) {
    return (RingBuffer_t){
        .arr = malloc(sizeof(int32_t) * (unsigned long)capacity), .head = 0, .tail = 0, .count = 0, .capacity = capacity
    };
}

void RingBuffer_Free(RingBuffer_t *rb) { free(rb->arr); }

bool RingBuffer_Append(RingBuffer_t *rb, int32_t value) {
    if (rb->count == rb->capacity) {
        return false;
    }

    rb->arr[rb->tail] = value;
    if (++rb->tail == rb->capacity) {
        rb->tail = 0;
    }
    ++rb->count;

    return true;
}

bool RingBuffer_Prepend(RingBuffer_t *rb, int32_t value) {
    if (rb->count == rb->capacity) {
        return false;
    }

    if (--rb->head < 0) {
        rb->head = rb->capacity - 1;
    }
    rb->arr[rb->head] = value;

    ++rb->count;
    return true;
}

bool RingBuffer_PeekFirst(RingBuffer_t *rb, int32_t *out) {
    if (rb->count == 0) {
        return false;
    }
    *out = rb->arr[rb->head];
    return true;
}

bool RingBuffer_PeekLast(RingBuffer_t *rb, int32_t *out) {
    if (rb->count == 0) {
        return false;
    }

    int32_t idx = rb->tail - 1;
    if (idx < 0) {
        idx = rb->capacity - 1;
    }
    *out = rb->arr[idx];

    return true;
}

bool RingBuffer_PopFirst(RingBuffer_t *rb, int32_t *out) {
    if (rb->count == 0) {
        return false;
    }

    *out = rb->arr[rb->head];
    if (++rb->head == rb->capacity) {
        rb->head = 0;
    }
    --rb->count;

    return true;
}

bool RingBuffer_PopLast(RingBuffer_t *rb, int32_t *out) {
    if (rb->count == 0) {
        return false;
    }

    if (--rb->tail < 0) {
        rb->tail = rb->capacity - 1;
    }
    *out = rb->arr[rb->tail];
    --rb->count;

    return true;
}

bool RingBuffer_GetAt(RingBuffer_t *rb, int32_t index, int32_t *out) {
    if (index < 0 || index >= rb->count) {
        return false;
    }

    index += rb->head;
    if (index > rb->capacity) {
        index -= rb->capacity;
    }
    *out = rb->arr[index];

    return true;
}

bool RingBuffer_IsEmpty(RingBuffer_t *rb) { return rb->count == 0; }

int32_t RingBuffer_Length(RingBuffer_t *rb) { return rb->count; }

int32_t RingBuffer_Capacity(RingBuffer_t *rb) { return rb->capacity; }
