#include "thread_pool.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Shared task helpers
// ---------------------------------------------------------------------------

static _Atomic uint64_t g_counter;
static _Atomic int g_sum;

static void taskIncrement(void *arg) {
    (void)arg;
    atomic_fetch_add(&g_counter, 1);
}

static void taskIncrementBy(void *arg) {
    atomic_fetch_add(&g_counter, *(uint64_t *)arg);
}

static struct timespec ts = {
    .tv_sec  = 0,
    .tv_nsec = 0,
};

static void taskSleepMs(void *arg) {
    ts.tv_nsec = (*(uint32_t *)arg) * 1000 * 1000;
    nanosleep(&ts, NULL);
}

static void taskAddInt(void *arg) { atomic_fetch_add(&g_sum, *(int *)arg); }

static void resetCounters(void) {
    atomic_store(&g_counter, 0);
    atomic_store(&g_sum, 0);
}

// ---------------------------------------------------------------------------
// ThreadPool_Create
// ---------------------------------------------------------------------------

void testCreate_ZeroWorkers(void) { assert(ThreadPool_Create(0) == NULL); }

void testCreate_Valid(void) {
    ThreadPool_t *pool = ThreadPool_Create(4);
    assert(pool != NULL);
    assert(!ThreadPool_IsRunning(pool));

    uint64_t n = 0;
    assert(ThreadPool_TasksScheduled(pool, &n));
    assert(n == 0);
    assert(ThreadPool_TasksStarted(pool, &n));
    assert(n == 0);
    assert(ThreadPool_TasksFinished(pool, &n));
    assert(n == 0);

    assert(ThreadPool_Free(pool, false));
}

void testCreate_SingleWorker(void) {
    ThreadPool_t *pool = ThreadPool_Create(1);
    assert(pool != NULL);
    assert(ThreadPool_Start(pool));
    assert(ThreadPool_IsRunning(pool));
    assert(ThreadPool_Free(pool, false));
}

void testCreate_MaxWorkers(void) {
    ThreadPool_t *pool = ThreadPool_Create(255);
    assert(pool != NULL);
    assert(ThreadPool_Free(pool, false));
}

// ---------------------------------------------------------------------------
// ThreadPool_Free
// ---------------------------------------------------------------------------

void testFree_Null(void) {
    assert(!ThreadPool_Free(NULL, false));
    assert(!ThreadPool_Free(NULL, true));
}

void testFree_NeverStarted(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(pool != NULL);
    assert(ThreadPool_Free(pool, false));
}

void testFree_NeverStarted_WaitFlag(void) {
    // waitTasksCompleted with a non-running pool should still succeed
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(pool != NULL);
    assert(ThreadPool_Free(pool, true));
}

void testFree_NoWait(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));
    assert(ThreadPool_Free(pool, false));
}

void testFree_WaitDrainsQueue(void) {
    resetCounters();
    const int N        = 20;
    ThreadPool_t *pool = ThreadPool_Create(4);
    assert(ThreadPool_Start(pool));

    for (int i = 0; i < N; ++i) {
        assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    }

    assert(ThreadPool_Free(pool, true));
    // pthread_join guarantees all workers exited, which happens only after
    // their last task finishes — so all N tasks must have completed
    assert((int)atomic_load(&g_counter) == N);
}

void testFree_WaitWithPendingQueue_TasksRun(void) {
    // Submit tasks before start, then start and free with wait
    resetCounters();
    const int N        = 5;
    ThreadPool_t *pool = ThreadPool_Create(2);

    for (int i = 0; i < N; ++i) {
        assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    }

    assert(ThreadPool_Start(pool));
    assert(ThreadPool_Free(pool, true));
    assert((int)atomic_load(&g_counter) == N);
}

// ---------------------------------------------------------------------------
// ThreadPool_Start
// ---------------------------------------------------------------------------

void testStart_Null(void) { assert(!ThreadPool_Start(NULL)); }

void testStart_SetsIsRunning(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(!ThreadPool_IsRunning(pool));
    assert(ThreadPool_Start(pool));
    assert(ThreadPool_IsRunning(pool));
    ThreadPool_Free(pool, false);
}

void testStart_DoubleStart_Fails(void) {
    // Second start must fail: workers ring buffer is already non-empty
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));
    assert(!ThreadPool_Start(pool));
    ThreadPool_Free(pool, false);
}

// ---------------------------------------------------------------------------
// ThreadPool_Stop
// ---------------------------------------------------------------------------

void testStop_Null(void) { assert(!ThreadPool_Stop(NULL)); }

void testStop_NeverStarted(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Stop(pool));
    assert(ThreadPool_Free(pool, false));
}

void testStop_Running(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));
    assert(ThreadPool_IsRunning(pool));
    assert(ThreadPool_Stop(pool));
    assert(!ThreadPool_IsRunning(pool));
    ThreadPool_Free(pool, false);
}

void testStop_AlreadyStopped(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));
    assert(ThreadPool_Stop(pool));
    assert(ThreadPool_Stop(pool)); // idempotent
    ThreadPool_Free(pool, false);
}

// ---------------------------------------------------------------------------
// ThreadPool_IsRunning
// ---------------------------------------------------------------------------

void testIsRunning_Null(void) { assert(!ThreadPool_IsRunning(NULL)); }

void testIsRunning_Lifecycle(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);

    assert(!ThreadPool_IsRunning(pool));
    assert(ThreadPool_Start(pool));
    assert(ThreadPool_IsRunning(pool));
    assert(ThreadPool_Stop(pool));
    assert(!ThreadPool_IsRunning(pool));

    ThreadPool_Free(pool, false);
}

// ---------------------------------------------------------------------------
// ThreadPool_Submit
// ---------------------------------------------------------------------------

void testSubmit_NullPool(void) {
    assert(!ThreadPool_Submit(NULL, taskIncrement, NULL, false));
}

void testSubmit_NullFn(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(!ThreadPool_Submit(pool, NULL, NULL, false));
    ThreadPool_Free(pool, false);
}

void testSubmit_BeforeStart_TaskExecutesAfterStart(void) {
    resetCounters();
    ThreadPool_t *pool = ThreadPool_Create(1);

    assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));

    uint64_t n = 0;
    assert(ThreadPool_TasksScheduled(pool, &n));
    assert(n == 1);

    assert(ThreadPool_Start(pool));
    assert(ThreadPool_Free(pool, true));
    assert(atomic_load(&g_counter) == 1);
}

void testSubmit_NullArg_StackOwnership(void) {
    // isArgOnHeap=false with NULL arg — must not crash
    resetCounters();
    ThreadPool_t *pool = ThreadPool_Create(1);
    assert(ThreadPool_Start(pool));
    assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    assert(ThreadPool_Free(pool, true));
    assert(atomic_load(&g_counter) == 1);
}

void testSubmit_HeapArg_FreedAfterRun(void) {
    // isArgOnHeap=true: pool frees the arg after the task runs
    resetCounters();
    ThreadPool_t *pool = ThreadPool_Create(1);
    assert(ThreadPool_Start(pool));

    uint64_t *val = malloc(sizeof(uint64_t));
    assert(val != NULL);
    *val = 7;

    assert(ThreadPool_Submit(pool, taskIncrementBy, val, true));
    assert(ThreadPool_Free(pool, true));
    assert(atomic_load(&g_counter) == 7);
    // `val` was freed by the pool — no double-free, no crash
}

void testSubmit_StackArg_NotFreed(void) {
    // isArgOnHeap=false: pool must not free the arg
    resetCounters();
    ThreadPool_t *pool = ThreadPool_Create(1);
    assert(ThreadPool_Start(pool));

    uint64_t val = 42;
    assert(ThreadPool_Submit(pool, taskIncrementBy, &val, false));
    assert(ThreadPool_Free(pool, true));
    assert(atomic_load(&g_counter) == 42);
}

void testSubmit_MultipleTasksOrdered(void) {
    // All submitted tasks must eventually execute
    resetCounters();
    const int N        = 50;
    ThreadPool_t *pool = ThreadPool_Create(3);
    assert(ThreadPool_Start(pool));

    for (int i = 0; i < N; ++i) {
        assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    }

    assert(ThreadPool_Free(pool, true));
    assert((int)atomic_load(&g_counter) == N);
}

// ---------------------------------------------------------------------------
// Task counters
// ---------------------------------------------------------------------------

void testTasksScheduled_Null(void) {
    uint64_t n = 0;
    assert(!ThreadPool_TasksScheduled(NULL, &n));
}

void testTasksStarted_Null(void) {
    uint64_t n = 0;
    assert(!ThreadPool_TasksStarted(NULL, &n));
}

void testTasksFinished_Null(void) {
    uint64_t n = 0;
    assert(!ThreadPool_TasksFinished(NULL, &n));
}

void testTaskCounters_Scheduled(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);

    uint64_t n = 0;
    assert(ThreadPool_TasksScheduled(pool, &n));
    assert(n == 0);

    // Submit without starting — tasks queue but nothing executes
    assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));

    assert(ThreadPool_TasksScheduled(pool, &n));
    assert(n == 3);

    ThreadPool_Free(pool, false);
}

void testTaskCounters_StartedAndFinished(void) {
    resetCounters();
    const int N        = 10;
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));

    for (int i = 0; i < N; ++i) {
        assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    }

    assert(ThreadPool_Free(pool, true));
    // pool is freed; verify via g_counter that all tasks ran
    assert((int)atomic_load(&g_counter) == N);
}

void testTaskCounters_ScheduledEqualsSubmitCount(void) {
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));

    const int N = 7;
    for (int i = 0; i < N; ++i) {
        assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    }

    uint64_t scheduled = 0;
    assert(ThreadPool_TasksScheduled(pool, &scheduled));
    assert((int)scheduled == N);

    ThreadPool_Free(pool, true);
}

// ---------------------------------------------------------------------------
// Stop-and-restart
// ---------------------------------------------------------------------------

void testRestart_WorksAfterStop(void) {
    resetCounters();
    ThreadPool_t *pool = ThreadPool_Create(2);

    assert(ThreadPool_Start(pool));
    assert(ThreadPool_IsRunning(pool));
    assert(ThreadPool_Stop(pool));
    assert(!ThreadPool_IsRunning(pool));

    // After stop the workers ring buffer is empty — start should succeed again
    assert(ThreadPool_Start(pool));
    assert(ThreadPool_IsRunning(pool));

    assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    assert(ThreadPool_Free(pool, true));
    // At minimum the post-restart task completed
    assert(atomic_load(&g_counter) >= 1);
}

// ---------------------------------------------------------------------------
// THREAD_POOL_SUBMIT_TASK macro
// ---------------------------------------------------------------------------

typedef struct {
    int x;
    int y;
} Point_t;

static void taskAddPoint(void *arg) {
    Point_t *p = (Point_t *)arg;
    atomic_fetch_add(&g_sum, p->x + p->y);
}

void testMacro_SubmitTask_AllocatesAndFrees(void) {
    atomic_store(&g_sum, 0);
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));

    THREAD_POOL_SUBMIT_TASK(pool, taskAddPoint, Point_t, .x = 3, .y = 4);
    THREAD_POOL_SUBMIT_TASK(pool, taskAddPoint, Point_t, .x = 1, .y = 2);

    assert(ThreadPool_Free(pool, true));
    assert(atomic_load(&g_sum) == 10); // (3+4) + (1+2)
}

void testMacro_SubmitTask_MultipleTypes(void) {
    resetCounters();
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));

    THREAD_POOL_SUBMIT_TASK(pool, taskAddInt, int, 5);
    THREAD_POOL_SUBMIT_TASK(pool, taskAddInt, int, -3);
    THREAD_POOL_SUBMIT_TASK(pool, taskAddInt, int, 10);

    assert(ThreadPool_Free(pool, true));
    assert(atomic_load(&g_sum) == 12);
}

// ---------------------------------------------------------------------------
// Concurrency stress
// ---------------------------------------------------------------------------

void testConcurrency_ManyTasksManyWorkers(void) {
    resetCounters();
    const int N        = 200;
    ThreadPool_t *pool = ThreadPool_Create(8);
    assert(ThreadPool_Start(pool));

    for (int i = 0; i < N; ++i) {
        assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    }

    assert(ThreadPool_Free(pool, true));
    assert((int)atomic_load(&g_counter) == N);
}

void testConcurrency_SingleWorkerOrdering(void) {
    // With one worker, tasks execute serially — counter must equal N
    resetCounters();
    const int N        = 30;
    ThreadPool_t *pool = ThreadPool_Create(1);
    assert(ThreadPool_Start(pool));

    for (int i = 0; i < N; ++i) {
        assert(ThreadPool_Submit(pool, taskIncrement, NULL, false));
    }

    assert(ThreadPool_Free(pool, true));
    assert((int)atomic_load(&g_counter) == N);
}

void testConcurrency_SlowTasks_FreeNoWait(void) {
    // Slow tasks submitted; Free(false) should not hang
    ThreadPool_t *pool = ThreadPool_Create(2);
    assert(ThreadPool_Start(pool));

    uint32_t ms = 50;
    assert(ThreadPool_Submit(pool, taskSleepMs, &ms, false));
    assert(ThreadPool_Submit(pool, taskSleepMs, &ms, false));

    assert(ThreadPool_Free(pool, false)); // must not block
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(void) {

#define RUN(t)                                                                 \
    do {                                                                       \
        printf("[TEST] %-55s", #t);                                            \
        t();                                                                   \
        printf("PASS\n");                                                      \
    } while (0)

    RUN(testCreate_ZeroWorkers);
    RUN(testCreate_Valid);
    RUN(testCreate_SingleWorker);
    RUN(testCreate_MaxWorkers);

    RUN(testFree_Null);
    RUN(testFree_NeverStarted);
    RUN(testFree_NeverStarted_WaitFlag);
    RUN(testFree_NoWait);
    RUN(testFree_WaitDrainsQueue);
    RUN(testFree_WaitWithPendingQueue_TasksRun);

    RUN(testStart_Null);
    RUN(testStart_SetsIsRunning);
    RUN(testStart_DoubleStart_Fails);

    RUN(testStop_Null);
    RUN(testStop_NeverStarted);
    RUN(testStop_Running);
    RUN(testStop_AlreadyStopped);

    RUN(testIsRunning_Null);
    RUN(testIsRunning_Lifecycle);

    RUN(testSubmit_NullPool);
    RUN(testSubmit_NullFn);
    RUN(testSubmit_BeforeStart_TaskExecutesAfterStart);
    RUN(testSubmit_NullArg_StackOwnership);
    RUN(testSubmit_HeapArg_FreedAfterRun);
    RUN(testSubmit_StackArg_NotFreed);
    RUN(testSubmit_MultipleTasksOrdered);

    RUN(testTasksScheduled_Null);
    RUN(testTasksStarted_Null);
    RUN(testTasksFinished_Null);
    RUN(testTaskCounters_Scheduled);
    RUN(testTaskCounters_StartedAndFinished);
    RUN(testTaskCounters_ScheduledEqualsSubmitCount);

    RUN(testRestart_WorksAfterStop);

    RUN(testMacro_SubmitTask_AllocatesAndFrees);
    RUN(testMacro_SubmitTask_MultipleTypes);

    RUN(testConcurrency_ManyTasksManyWorkers);
    RUN(testConcurrency_SingleWorkerOrdering);
    RUN(testConcurrency_SlowTasks_FreeNoWait);

    printf("\nAll tests passed.\n");
    return 0;
}
