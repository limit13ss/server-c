#ifndef NAZARK_RING_BUFFER_H
#define NAZARK_RING_BUFFER_H 1

#include <stdbool.h>
#include <stdint.h>

typedef struct RingBuffer {
    int32_t *arr;
    int32_t head;
    int32_t tail;
    int32_t count;
    int32_t capacity;
} RingBuffer_t;

RingBuffer_t RingBuffer_Create(int32_t capacity);
void RingBuffer_Free(RingBuffer_t *rb);

bool RingBuffer_Append(RingBuffer_t *rb, int32_t value);
bool RingBuffer_Prepend(RingBuffer_t *rb, int32_t value);
bool RingBuffer_PeekFirst(RingBuffer_t *rb, int32_t *out);
bool RingBuffer_PeekLast(RingBuffer_t *rb, int32_t *out);
bool RingBuffer_PopFirst(RingBuffer_t *rb, int32_t *out);
bool RingBuffer_PopLast(RingBuffer_t *rb, int32_t *out);
bool RingBuffer_IsEmpty(RingBuffer_t *rb);
bool RingBuffer_GetAt(RingBuffer_t *rb, int32_t index, int32_t *out);
int32_t RingBuffer_Length(RingBuffer_t *rb);
int32_t RingBuffer_Capacity(RingBuffer_t *rb);

#endif // NAZARK_RING_BUFFER_H
