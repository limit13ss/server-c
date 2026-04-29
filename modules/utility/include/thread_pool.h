#ifndef NAZARK_THREAD_POOL_H
#define NAZARK_THREAD_POOL_H

#include <pthread.h>
#include <stdint.h>

#ifndef NKRYLOV_RING_BUFFER_PTHREAD_H
#define NKRYLOV_RING_BUFFER_PTHREAD_H
#define RB_TYPE pthread_t
#define RB_TYPE_NAME pthread
#include "generic_ring_buffer.h"
#undef RB_TYPE
#undef RB_TYPE_NAME
#endif // NKRYLOV_RING_BUFFER_PTHREAD_H

typedef struct ThreadPool ThreadPool_t;

typedef struct ThreadTask {
    uint64_t id;
    void (*fn)(void *arg);
    void *arg;
} ThreadTask_t;

ThreadPool_t *ThreadPool_Create(uint8_t workersCount);
bool ThreadPool_Start(ThreadPool_t *pool);
bool ThreadPool_Stop(ThreadPool_t *pool);
void ThreadPool_Submit(ThreadPool_t *pool, void (*fn)(void *arg), void *args);

#define THREAD_POOL_SUBMIT_TASK(pool, fn, type, ...)                                                                   \
    do {                                                                                                               \
        type *_a = malloc(sizeof(type));                                                                               \
        *_a      = (type){ __VA_ARGS__ };                                                                              \
        ThreadPool_Submit(pool, fn, _a);                                                                               \
    } while (0)

bool ThreadPool_Free(ThreadPool_t *pool);

#endif // NAZARK_THREAD_POOL_H
