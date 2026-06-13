#ifndef NAZARK_TEST_COMMON_H
#define NAZARK_TEST_COMMON_H

#include <stdio.h>

#define RUN(t)                                                                 \
    do {                                                                       \
        printf("[TEST] %-55s", #t);                                            \
        t();                                                                   \
        printf("PASS\n");                                                      \
    } while (0)

#endif // NAZARK_TEST_COMMON_H
