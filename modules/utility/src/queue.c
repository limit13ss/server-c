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

    q->head = NULL;
    q->tail = NULL;
    q->length = 0;
}
