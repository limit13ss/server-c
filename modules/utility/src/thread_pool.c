#include "thread_pool.h"
#include <stdlib.h>

bool ThreadPool_Create(ThreadPool_t *outPool, uint8_t workersCount) {
    if (workersCount == 0 || outPool == NULL) {
        return false;
    }

    RingBuffer_pthread_t *workers = malloc(sizeof(RingBuffer_pthread_t));
    if (!workers) {
        return false;
    }

    Queue_t *queue = malloc(sizeof(Queue_t));
    if (!queue) {
        free(workers);
        return false;
    }

    *workers = RingBuffer_pthread_Create(workersCount);
    *queue   = Queue_Create(threadTaskDeallocator);

    *outPool = (ThreadPool_t){ .workers = workers, .tasks = queue };
    return true;
}

bool ThreadPool_Free(ThreadPool_t *pool) {
    if (pool == NULL || pool->workers == NULL || pool->tasks == NULL) {
        return false;
    }

    pthread_t pThread;
    while (RingBuffer_pthread_PopFirst(pool->workers, &pThread)) {
        pthread_cancel(pThread);
        pthread_join(pThread, NULL);
    }

    RingBuffer_pthread_Free(pool->workers);
    free(pool->workers);
    pool->workers = NULL;

    Queue_Free(pool->tasks);
    free(pool->tasks);
    pool->tasks = NULL;

    return true;
}
