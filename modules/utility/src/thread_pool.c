#include "thread_pool.h"
#include "queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct WorkerArg {
    Queue_t *tasksQueue;
    ThreadTask_t *taskPtr;
    pthread_mutex_t *mutexQueue;
    pthread_cond_t *condQueue;
} WorkerArg_t;

#ifndef NKRYLOV_RING_BUFFER_WORKER_ARG
#define NKRYLOV_RING_BUFFER_WORKER_ARG
#define RB_TYPE WorkerArg_t
#define RB_TYPE_NAME WorkerArg
#include "generic_ring_buffer.h"
#undef RB_TYPE
#undef RB_TYPE_NAME
#endif // NKRYLOV_RING_BUFFER_WORKER_ARG

struct ThreadPool {
    RingBuffer_pthread_t *workers;
    RingBuffer_WorkerArg_t *workerArgs;
    Queue_t *tasks;
    pthread_mutex_t mutexQueue;
    pthread_cond_t condQueue;
};

static inline void threadTaskDeallocator(void *task) {
    ThreadTask_t *pTask = (ThreadTask_t *)(task);

    free(pTask->arg);
    free(pTask);
}

void *threadFunction(void *arg) {
    if (arg == NULL) {
        return NULL;
    }

    WorkerArg_t *wfArg     = (WorkerArg_t *)(arg);
    Queue_t *tasks         = wfArg->tasksQueue;
    ThreadTask_t *taskPtr  = wfArg->taskPtr;
    pthread_mutex_t *mutex = wfArg->mutexQueue;
    pthread_cond_t *cond   = wfArg->condQueue;

    if (tasks == NULL || taskPtr == NULL || mutex == NULL || cond == NULL) {
        return NULL;
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

    RingBuffer_WorkerArg_t *workerArgs =
        RingBuffer_WorkerArg_Create(workersCount);
    if (!workerArgs) {
        RingBuffer_pthread_Free(workers);
        free(pool);
        return NULL;
    }

    Queue_t *queue = Queue_Create(threadTaskDeallocator);
    if (!queue) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        free(pool);
        return NULL;
    }

    pthread_mutex_t mutexQueue;
    pthread_cond_t condQueue;

    pthread_mutex_init(&mutexQueue, NULL);
    pthread_cond_init(&condQueue, NULL);

    *pool = (ThreadPool_t){ .workers    = workers,
                            .workerArgs = workerArgs,
                            .tasks      = queue,
                            .mutexQueue = mutexQueue,
                            .condQueue  = condQueue };

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

    WorkerArg_t arg;
    while (RingBuffer_WorkerArg_PopFirst(pool->workerArgs, &arg)) {
        free(arg.taskPtr);
    }

    RingBuffer_pthread_Free(pool->workers);
    pool->workers = NULL;

    RingBuffer_WorkerArg_Free(pool->workerArgs);
    pool->workerArgs = NULL;

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

    uint64_t workersCount = 0, workersCapacity = 0;
    if (!RingBuffer_pthread_Capacity(pool->workers, &workersCapacity)) {
        return false;
    }

    if (!RingBuffer_pthread_Length(pool->workers, &workersCount) ||
        workersCount != 0) {
        return false;
    }

    for (size_t i = workersCount; i < workersCapacity; ++i) {
        pthread_t thread;
        ThreadTask_t *threadTask = malloc(sizeof(ThreadTask_t));
        WorkerArg_t arg          = { .tasksQueue = pool->tasks,
                                     .mutexQueue = &pool->mutexQueue,
                                     .condQueue  = &pool->condQueue,
                                     .taskPtr    = threadTask };

        pthread_create(&thread, NULL, &threadFunction, (void *)(&arg));

        RingBuffer_pthread_Append(pool->workers, thread);
        RingBuffer_WorkerArg_Append(pool->workerArgs, arg);
    }

    return true;
}
