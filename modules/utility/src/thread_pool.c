#include "thread_pool.h"

ThreadPool_t ThreadPool_Create(uint8_t workersCount) {
    RingBuffer_pthread_t *workers = malloc(sizeof(RingBuffer_pthread_t));
    Queue_t *queue                = malloc(sizeof(Queue_t));

    *workers = RingBuffer_pthread_Create(workersCount);
    *queue   = Queue_Create(threadTaskDeallocator);

    return (ThreadPool_t){ .workers = workers, .tasks = queue };
}

bool ThreadPool_Free(ThreadPool_t *pool) {
    if (pool == NULL || pool->workers == NULL || pool->tasks == NULL) {
        return false;
    }

    RingBuffer_pthread_Free(pool->workers);
    free(pool->workers);
    pool->workers = NULL;

    Queue_Free(pool->tasks);
    free(pool->tasks);
    pool->tasks = NULL;

    return true;
}
