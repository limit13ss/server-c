#include "thread_pool.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

struct ThreadPool {
    RingBuffer_pthread_t *workers;
    Queue_t *tasks;
    pthread_mutex_t mutexQueue;
    pthread_cond_t condQueue;
};

static inline void threadTaskDeallocator(void *task) {
    ThreadTask_t *pTask = (ThreadTask_t *)(task);

    free(pTask->arg);
    free(pTask);
}

void threadFunction(Queue_t *tasks, ThreadTask_t *taskPtr, pthread_mutex_t *mutex, pthread_cond_t *cond) {
    if (tasks == NULL || taskPtr == NULL || mutex == NULL || cond == NULL) {
        return;
    }

    while (1) {
        pthread_mutex_lock(mutex);
        while (!Queue_Pop(tasks, (void *)(&taskPtr))) {
            printf("[INF] ThreadPool queue is empty...\n");
            pthread_cond_wait(cond, mutex);
        }
        pthread_mutex_unlock(mutex);

        printf("[INF] ThreadTask (id=%lu) -- START\n", taskPtr->id);
        taskPtr->fn(taskPtr->arg);
        printf("[INF] ThreadTask (id=%lu) -- END\n", taskPtr->id);
    }
}

ThreadPool_t *ThreadPool_Create(uint8_t workersCount) {
    if (workersCount == 0) {
        return false;
    }

    ThreadPool_t *pool = malloc(sizeof(ThreadPool_t));
    if (pool == NULL) {
        return NULL;
    };

    RingBuffer_pthread_t *workers = RingBuffer_pthread_Create(workersCount);
    if (!workers) {
        free(pool);
        return NULL;
    }

    Queue_t *queue = Queue_Create(threadTaskDeallocator);
    if (!queue) {
        RingBuffer_pthread_Free(workers);
        free(pool);
        return NULL;
    }

    pthread_mutex_t mutexQueue;
    pthread_cond_t condQueue;

    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);

    *pool = (ThreadPool_t){ .workers = workers, .tasks = queue, .mutexQueue = mutexQueue, .condQueue = condQueue };

    return pool;
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
    pool->workers = NULL;

    Queue_Free(pool->tasks);
    pool->tasks = NULL;

    pthread_mutex_destroy(&pool->mutexQueue);
    pthread_cond_destroy(&pool->condQueue);

    free(pool);

    return true;
}

bool ThreadPool_Start(ThreadPool_t *pool) {
    if (pool == NULL || pool->workers == NULL || pool->tasks == NULL) {
        return false;
    }

    return true;
}
