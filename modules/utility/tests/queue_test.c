#include "queue.h"

#include <assert.h>
#include <stdio.h>

void dataDeallocator(void *data) {
    uint32_t *p = (uint32_t *)(data);
    printf("dataDeallocator is called with %u:%p\n", *p, data);
}

void testCreateFree(void) {
    Queue_t queue = Queue_Create(NULL);

    assert(queue.head == NULL);
    assert(queue.tail == NULL);
    assert(queue.deallocator == NULL);
    assert(queue.length == 0);

    assert(!Queue_Free(NULL));
    assert(!Queue_Free(&queue));

    queue = Queue_Create(dataDeallocator);

    assert(queue.deallocator == dataDeallocator);
    assert(!Queue_Free(&queue));
}

void testPush(void) {
    Queue_t queue = Queue_Create(dataDeallocator);

    uint32_t u1 = 1;
    assert(Queue_Push(&queue, &u1));

    uint64_t length = 0;
    assert(Queue_Length(&queue, &length));
    assert(length == 1);
    assert(queue.head == queue.tail);

    uint32_t u2 = 2;
    assert(Queue_Push(&queue, &u2));

    assert(Queue_Length(&queue, &length));
    assert(length == 2);
    assert(queue.head->data == &u1);
    assert(queue.tail->data == &u2);

    uint32_t u3 = 3;
    assert(Queue_Push(&queue, &u3));

    assert(Queue_Length(&queue, &length));
    assert(length == 3);

    Queue_Free(&queue);
}

void testPop(void) {
    Queue_t queue = Queue_Create(dataDeallocator);

    uint32_t *popped = NULL;
    uint64_t length  = 0;

    assert(!Queue_Pop(&queue, (void **)(&popped)));

    uint32_t u1 = 1, u2 = 2, u3 = 3;
    assert(Queue_Push(&queue, &u1));
    assert(Queue_Push(&queue, &u2));
    assert(Queue_Push(&queue, &u3));

    assert(Queue_Length(&queue, &length));
    assert(length == 3);

    assert(Queue_Pop(&queue, (void **)(&popped)));
    assert(*popped == u1);
    assert(Queue_Length(&queue, &length));
    assert(length == 2);

    assert(Queue_Pop(&queue, (void **)(&popped)));
    assert(*popped == u2);
    assert(Queue_Length(&queue, &length));
    assert(length == 1);

    assert(Queue_Pop(&queue, (void **)(&popped)));
    assert(*popped == u3);
    assert(Queue_Length(&queue, &length));
    assert(length == 0);

    assert(!Queue_Pop(&queue, (void **)(&popped)));

    assert(queue.head == NULL);
    assert(queue.tail == NULL);

    u1 = 123456;
    assert(Queue_Push(&queue, &u1));

    Queue_Free(&queue);
}

int main(void) {

    printf("[TEST:CreateFree -- START]\n");
    testCreateFree();
    printf("[TEST:CreateFree -- END]\n");

    printf("[TEST:Pop -- START]\n");
    testPush();
    printf("[TEST:Pop -- END] \n");

    printf("[TEST:Pop -- START]\n");
    testPop();
    printf("[TEST:Pop -- END]\n");

    printf("All tests passed!\n");
    return 0;
}
