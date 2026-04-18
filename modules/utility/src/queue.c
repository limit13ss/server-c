#include "queue.h"

#include <stdlib.h>

Queue_t Queue_Create(void) {
    return (Queue_t){
        .head   = NULL,
        .tail   = NULL,
        .length = 0,
    };
}

void Queue_Free(Queue_t *q) {
    if (q == NULL) {
        return;
    }

    Node_t *cur = q->head;
    while (cur != NULL) {
        free(cur);
        cur = cur->next;
    }

    q->head   = NULL;
    q->tail   = NULL;
    q->length = 0;
}

bool Queue_Push(Queue_t *q, void *data) {
    if (q == NULL || data == NULL) {
        return false;
    }

    Node_t *newNode = malloc(sizeof(Node_t));
    newNode->data   = data;
    newNode->next   = NULL;

    if (q->head == NULL) {
        q->head = newNode;
    }

    if (q->tail == NULL) {
        q->tail = newNode;
    } else {
        q->tail->next = newNode;
        q->tail       = newNode;
    }

    ++q->length;
    return true;
}

bool Queue_Pop(Queue_t *q, void **outData) {
    if (q == NULL || q->length == 0 || q->head == NULL) {
        return false;
    }

    *outData = q->head;
    q->head  = q->head->next;

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

    *outData = q->head;
    return true;
}

uint64_t Queue_Length(Queue_t *q) {
    return q->length;
}

