#ifndef NAZARK_QUEUE_H
#define NAZARK_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Node Node_t;

typedef struct Queue Queue_t;

Queue_t* Queue_Create(void (*deallocator)(void *data));
bool Queue_Free(Queue_t *q);
bool Queue_Push(Queue_t *q, void *data);
bool Queue_Pop(Queue_t *q, void **outData);
bool Queue_Peek(Queue_t *q, void **outData);

bool Queue_Length(Queue_t *q, uint64_t *out);

#endif // NAZARK_QUEUE_H
