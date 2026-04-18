#ifndef NAZARK_QUEUE_H
#define NAZARK_QUEUE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Node {
    void *data;
    struct Node *next;
} Node_t;

typedef struct Queue {
    Node_t *head;
    Node_t *tail;
    uint64_t length;
    void (*deallocator)(void *data);
} Queue_t;

Queue_t Queue_Create(void (*deallocator)(void *data));
void Queue_Free(Queue_t *q);
bool Queue_Push(Queue_t *q, void *data);
bool Queue_Pop(Queue_t *q, void **outData);
bool Queue_Peek(Queue_t *q, void **outData);

uint64_t Queue_Length(Queue_t *q);

#endif // NAZARK_QUEUE_H
