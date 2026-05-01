#include "thread_pool.h"
#include "queue.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct WorkerArg {
    Queue_t *tasksQueue;
    pthread_mutex_t *mutexQueue;
    pthread_cond_t *condQueue;
    pthread_cond_t *condFree;
    bool _Atomic *isRunning;
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

    pthread_mutex_t *mutexQueue;
    pthread_mutex_t *mutexPool;
    pthread_cond_t *condQueue;
    pthread_cond_t *condFree;

    bool _Atomic isRunning;
    uint64_t currentTaskId;
};

static inline void threadTaskDeallocator(void *task) {
    ThreadTask_t *pTask = (ThreadTask_t *)(task);

    if (pTask->arg && pTask->isArgOnHeap) {
        free(pTask->arg);
    }
    free(pTask);
}

void *threadFunction(void *arg) {
    if (!arg) {
        return NULL;
    }

    WorkerArg_t *wfArg          = (WorkerArg_t *)(arg);
    Queue_t *tasks              = wfArg->tasksQueue;
    pthread_mutex_t *mutexQueue = wfArg->mutexQueue;
    pthread_cond_t *condQueue   = wfArg->condQueue;
    pthread_cond_t *condFree    = wfArg->condFree;
    bool _Atomic *isRunning     = wfArg->isRunning;

    if (!tasks || !mutexQueue || !condQueue) {
        return NULL;
    }

    ThreadTask_t *taskPtr = NULL;
    while (1) {
        pthread_mutex_lock(mutexQueue);
        while (*isRunning && !Queue_Peek(tasks, (void **)(&taskPtr))) {
            pthread_cond_wait(condQueue, mutexQueue);
        }

        if (!*isRunning) {
            pthread_mutex_unlock(mutexQueue);
            break;
        }
        if (!Queue_Pop(tasks, (void **)(&taskPtr))) {
            pthread_mutex_unlock(mutexQueue);
            continue;
        }
        pthread_cond_signal(condFree);
        pthread_mutex_unlock(mutexQueue);

        printf("[INF] ThreadTask (id=%lu) -- START\n", taskPtr->id);
        taskPtr->fn(taskPtr->arg);
        printf("[INF] ThreadTask (id=%lu) -- END\n", taskPtr->id);
    }

    return NULL;
}

ThreadPool_t *ThreadPool_Create(uint8_t workersCount) {
    if (workersCount == 0) {
        return NULL;
    }

    ThreadPool_t *pool = malloc(sizeof(ThreadPool_t));
    if (!pool) {
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

    pthread_mutex_t *mutexQueue = malloc(sizeof(pthread_mutex_t));
    if (!mutexQueue) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        Queue_Free(queue);
        free(pool);
        return NULL;
    }
    pthread_mutex_t *mutexPool = malloc(sizeof(pthread_mutex_t));
    if (!mutexPool) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        Queue_Free(queue);
        free(mutexQueue);
        free(pool);
        return NULL;
    }
    pthread_cond_t *condQueue = malloc(sizeof(pthread_cond_t));
    if (!condQueue) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        Queue_Free(queue);
        free(mutexQueue);
        free(mutexPool);
        free(pool);
        return NULL;
    }
    pthread_cond_t *condFree = malloc(sizeof(pthread_cond_t));
    if (!condFree) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        Queue_Free(queue);
        free(mutexQueue);
        free(mutexPool);
        free(condQueue);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(mutexQueue, NULL)) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        Queue_Free(queue);
        free(mutexQueue);
        free(mutexPool);
        free(condQueue);
        free(condFree);
        free(pool);
        return NULL;
    }
    if (pthread_mutex_init(mutexPool, NULL)) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        Queue_Free(queue);
        free(mutexQueue);
        free(mutexPool);
        free(condQueue);
        free(condFree);
        pthread_mutex_destroy(mutexQueue);
        free(pool);
        return NULL;
    }
    if (pthread_cond_init(condQueue, NULL)) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        Queue_Free(queue);
        free(mutexQueue);
        free(mutexPool);
        free(condQueue);
        free(condFree);
        pthread_mutex_destroy(mutexQueue);
        pthread_mutex_destroy(mutexPool);
        free(pool);
        return NULL;
    }
    if (pthread_cond_init(condFree, NULL)) {
        RingBuffer_pthread_Free(workers);
        RingBuffer_WorkerArg_Free(workerArgs);
        Queue_Free(queue);
        free(mutexQueue);
        free(mutexPool);
        free(condQueue);
        free(condFree);
        pthread_mutex_destroy(mutexQueue);
        pthread_mutex_destroy(mutexPool);
        pthread_cond_destroy(condQueue);
        free(pool);
        return NULL;
    }

    *pool = (ThreadPool_t){ .workers       = workers,
                            .workerArgs    = workerArgs,
                            .tasks         = queue,
                            .mutexQueue    = mutexQueue,
                            .mutexPool     = mutexPool,
                            .condQueue     = condQueue,
                            .condFree      = condFree,
                            .isRunning     = false,
                            .currentTaskId = 0 };

    return pool;
}

bool ThreadPool_Free(ThreadPool_t *pool, bool waitTasksCompleted) {
    if (!pool) {
        return false;
    }

    if (pthread_mutex_trylock(pool->mutexPool) != 0) {
        return false;
    }

    if (waitTasksCompleted && pool->isRunning) {
        uint64_t queueLength = 0;
        pthread_mutex_lock(pool->mutexQueue);
        while (Queue_Length(pool->tasks, &queueLength) && queueLength != 0) {
            pthread_cond_wait(pool->condFree, pool->mutexQueue);
        }
        pthread_mutex_unlock(pool->mutexQueue);
    }

    pool->isRunning = false;

    pthread_cond_broadcast(pool->condQueue);
    pthread_t pThread;
    while (RingBuffer_pthread_PopFirst(pool->workers, &pThread)) {
        pthread_join(pThread, NULL);
    }

    WorkerArg_t arg;
    while (RingBuffer_WorkerArg_PopFirst(pool->workerArgs, &arg))
        ;

    RingBuffer_pthread_Free(pool->workers);
    pool->workers = NULL;

    RingBuffer_WorkerArg_Free(pool->workerArgs);
    pool->workerArgs = NULL;

    Queue_Free(pool->tasks);
    pool->tasks = NULL;

    pthread_mutex_destroy(pool->mutexQueue);
    free(pool->mutexQueue);
    pthread_cond_destroy(pool->condQueue);
    free(pool->condQueue);
    pthread_cond_destroy(pool->condFree);
    free(pool->condFree);

    pthread_mutex_t *mutexPool = pool->mutexPool;
    free(pool);

    pthread_mutex_unlock(mutexPool);
    pthread_mutex_destroy(mutexPool);
    free(mutexPool);

    return true;
}

bool ThreadPool_Start(ThreadPool_t *pool) {
    if (!pool) {
        return false;
    }

    if (pthread_mutex_trylock(pool->mutexPool) != 0) {
        return false;
    }

    uint64_t workersCapacity = 0;
    if (!RingBuffer_pthread_Capacity(pool->workers, &workersCapacity)) {
        goto unlockMutexReturnFalse;
    }

    uint64_t workersCount = 0;
    if (!RingBuffer_pthread_Length(pool->workers, &workersCount) ||
        workersCount != 0) {
        goto unlockMutexReturnFalse;
    }

    uint64_t createdThreadsCount = 0;
    bool needRollback            = false;
    pthread_t *threads           = malloc(sizeof(pthread_t) * workersCapacity);
    if (!threads) {
        goto unlockMutexReturnFalse;
    }

    pool->isRunning = true;
    for (size_t i = 0; i < workersCapacity; ++i) {
        pthread_t thread;

        WorkerArg_t arg = {
            .tasksQueue = pool->tasks,
            .mutexQueue = pool->mutexQueue,
            .condQueue  = pool->condQueue,
            .condFree   = pool->condFree,
            .isRunning  = &pool->isRunning,
        };
        WorkerArg_t *argPtr;

        if (!RingBuffer_WorkerArg_Append(pool->workerArgs, arg)) {
            needRollback = true;
            break;
        }
        if (!RingBuffer_WorkerArg_PeekLast(pool->workerArgs, &argPtr)) {
            needRollback = true;
            break;
        }

        if (pthread_create(&thread, NULL, &threadFunction, (void *)(argPtr)) !=
            0) {
            needRollback = true;
            break;
        }

        threads[createdThreadsCount] = thread;
        ++createdThreadsCount;

        if (!RingBuffer_pthread_Append(pool->workers, thread)) {
            needRollback = true;
            break;
        }
    }

    if (needRollback) {
        pool->isRunning = false;
        pthread_cond_broadcast(pool->condQueue);
        for (size_t i = 0; i < createdThreadsCount; ++i) {
            pthread_join(threads[i], NULL);
        }

        pthread_t thread;
        while (RingBuffer_pthread_PopFirst(pool->workers, &thread))
            ;

        WorkerArg_t arg;
        while (RingBuffer_WorkerArg_PopFirst(pool->workerArgs, &arg))
            ;
    }

    free(threads);
    pthread_mutex_unlock(pool->mutexPool);

    return !needRollback;

unlockMutexReturnFalse:
    pthread_mutex_unlock(pool->mutexPool);
    return false;
}

bool ThreadPool_Stop(ThreadPool_t *pool) {
    if (!pool) {
        return false;
    }

    if (pthread_mutex_trylock(pool->mutexPool) != 0) {
        return false;
    }

    pool->isRunning = false;

    pthread_cond_broadcast(pool->condQueue);
    pthread_t pThread;
    while (RingBuffer_pthread_PopFirst(pool->workers, &pThread)) {
        pthread_join(pThread, NULL);
    }

    WorkerArg_t arg;
    while (RingBuffer_WorkerArg_PopFirst(pool->workerArgs, &arg))
        ;

    pthread_mutex_unlock(pool->mutexPool);
    return true;
}

bool ThreadPool_Submit(ThreadPool_t *pool, void (*fn)(void *arg), void *args,
                       bool isArgOnHeap) {
    if (!pool || !fn) {
        return false;
    }

    ThreadTask_t *task = malloc(sizeof(ThreadTask_t));
    if (!task) {
        perror("[ERR][ThreadPool] Failed to allocate memory for ThreadTask\n");
        return false;
    }

    pthread_mutex_lock(pool->mutexQueue);
    *task = (ThreadTask_t){ .id          = pool->currentTaskId++,
                            .fn          = fn,
                            .arg         = args,
                            .isArgOnHeap = isArgOnHeap };

    bool isSuccess = Queue_Push(pool->tasks, task);
    if (isSuccess) {
        pthread_cond_signal(pool->condQueue);
    } else {
        free(task);
    }

    pthread_mutex_unlock(pool->mutexQueue);

    return isSuccess;
}
