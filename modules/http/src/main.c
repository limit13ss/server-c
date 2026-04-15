#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "server.h"
#include "thread_pool.h"

int32_t main(void) {
    int32_t mainListenerSocketFD = server_initMainListener();

    printf("Socket fd=%d\n", mainListenerSocketFD);
    getchar();

    thread_task_t task = {
        .id = 12345, .fn = NULL, .arg = NULL, .next = NULL
    };

    printf("Thread task id=%lu\n", task.id);

    close(mainListenerSocketFD);
    return 0;
}
