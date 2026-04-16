#include "macros.h"

#ifndef RB_TYPE
#define RB_TYPE int
#define RB_TYPE_NAME default
#endif

#define RB_FUNC(name) CONCAT(RingBuffer_, CONCAT(RB_TYPE_NAME, _##name))
#define RB_STRUCT CONCAT(RingBuffer_, CONCAT(RB_TYPE_NAME, _t))

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    RB_TYPE *arr;
    uint32_t head, tail, count, capacity;
} RB_STRUCT;

RB_STRUCT RB_FUNC(Create)(uint32_t capacity) {
    return (RB_STRUCT){ 
        .arr = malloc(sizeof(RB_TYPE) * capacity),
        .head = 0,
        .tail = 0,
        .count = 0,
        .capacity = capacity 
    };
}

void RB_FUNC(Free)(RB_STRUCT *rb) { free(rb->arr); }

bool RB_FUNC(Append)(RB_STRUCT *rb, RB_TYPE value) {
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

bool RB_FUNC(Prepend)(RB_STRUCT *rb, RB_TYPE value) {
    if (rb->count == rb->capacity) {
        return false;
    }

    if (rb->head == 0) {
        rb->head = rb->capacity - 1;
    } else {
        --rb->head;
    }
    rb->arr[rb->head] = value;

    ++rb->count;
    return true;
}

bool RB_FUNC(PeekFirst)(RB_STRUCT *rb, RB_TYPE *out) {
    if (rb->count == 0) {
        return false;
    }
    *out = rb->arr[rb->head];
    return true;
}

bool RB_FUNC(PeekLast)(RB_STRUCT *rb, RB_TYPE *out) {
    if (rb->count == 0) {
        return false;
    }

    uint32_t idx = 0;
    if (rb->tail == 0) {
        idx = rb->capacity - 1;
    } else {
        idx = rb->tail - 1;
    }
    *out = rb->arr[idx];

    return true;
}

bool RB_FUNC(PopFirst)(RB_STRUCT *rb, RB_TYPE *out) {
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

bool RB_FUNC(PopLast)(RB_STRUCT *rb, RB_TYPE *out) {
    if (rb->count == 0) {
        return false;
    }

    if (rb->tail == 0) {
        rb->tail = rb->capacity - 1;
    } else {
        --rb->tail;
    }
    *out = rb->arr[rb->tail];
    --rb->count;

    return true;
}

bool RB_FUNC(GetAt)(RB_STRUCT *rb, uint32_t index, RB_TYPE *out) {
    if (index >= rb->count) {
        return false;
    }

    if (index >= rb->capacity - rb->head) {
        index = index - (rb->capacity - rb->head);
    } else {
        index = index + rb->head;
    }
    *out = rb->arr[index];

    return true;
}

bool RB_FUNC(IsEmpty)(RB_STRUCT *rb) { return rb->count == 0; }

uint32_t RB_FUNC(Length)(RB_STRUCT *rb) { return rb->count; }

uint32_t RB_FUNC(Capacity)(RB_STRUCT *rb) { return rb->capacity; }

