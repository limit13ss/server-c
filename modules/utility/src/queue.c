#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

Queue_t Queue_Create(void (*deallocator)(void *data)) {
    return (Queue_t){ .head = NULL, .tail = NULL, .length = 0, .deallocator = deallocator };
}

bool Queue_Free(Queue_t *q) {
    if (q == NULL || q->head == NULL) {
        return false;
    }

    Node_t *cur  = q->head;
    Node_t *next = cur->next;

    while (cur != NULL) {
        next = cur->next;
        if (q->deallocator != NULL) {
            q->deallocator(cur->data);
        }

        free(cur);
        cur = next;
    }

    q->head   = NULL;
    q->tail   = NULL;
    q->length = 0;

    return true;
}

bool Queue_Push(Queue_t *q, void *data) {
    if (q == NULL || data == NULL) {
        return false;
    }

    Node_t *newNode = malloc(sizeof(Node_t));
    newNode->data   = data;
    newNode->next   = NULL;

    if (q->tail == NULL) {
        q->tail = newNode;
    } else {
        q->tail->next = newNode;
        q->tail       = newNode;
    }

    if (q->head == NULL) {
        q->head = newNode;
    }

    ++q->length;
    return true;
}

bool Queue_Pop(Queue_t *q, void **outData) {
    if (q == NULL || q->length == 0 || q->head == NULL) {
        return false;
    }

    *outData     = q->head->data;
    Node_t *next = q->head->next;

    free(q->head);
    q->head = next;

    --q->length;
    if (q->length == 0) {
        q->head = NULL;
        q->tail = NULL;
    }

    return true;
}

bool Queue_Peek(Queue_t *q, void **outData) {
    if (q == NULL || q->length == 0 || q->head == NULL) {
        return false;
    }

    *outData = q->head->data;
    return true;
}

bool Queue_Length(Queue_t *q, uint64_t *out) {
    if (q == NULL) {
        return false;
    }

    *out = q->length;
    return true;
}
