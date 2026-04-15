#include "macros.h"

#ifndef RB_TYPE
#define RB_TYPE int
#define RB_TYPE_NAME default
#endif

#define RB_FUNC(name) CONCAT(RingBuffer_, CONCAT(RB_TYPE_NAME, _ ## name))
#define RB_STRUCT CONCAT(RingBuffer_, CONCAT(RB_TYPE_NAME, _t))

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    RB_TYPE *arr;
    uint32_t head, tail, count, capacity;
} RB_STRUCT;

bool RB_FUNC(Append)(RB_STRUCT *rb, RB_TYPE value);
bool RB_FUNC(Prepend)(RB_STRUCT *rb, RB_TYPE value);
bool RB_FUNC(PeekFirst)(RB_STRUCT *rb, RB_TYPE *out);
bool RB_FUNC(PeekLast)(RB_STRUCT *rb, RB_TYPE *out);
bool RB_FUNC(PopFirst)(RB_STRUCT *rb, RB_TYPE *out);
bool RB_FUNC(PopLast)(RB_STRUCT *rb, RB_TYPE *out);
bool RB_FUNC(IsEmpty)(RB_STRUCT *rb);
bool RB_FUNC(GetAt)(RB_STRUCT *rb, RB_TYPE index, RB_TYPE *out);
uint32_t RB_FUNC(Length)(RB_STRUCT *rb);
uint32_t RB_FUNC(Capacity)(RB_STRUCT *rb);
