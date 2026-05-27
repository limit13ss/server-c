#include "macros.h"

#include <stdint.h>
#include <stdlib.h>

#ifndef SET_TYPE
#define SET_TYPE int32_t
#define SET_TYPE_NAME i32
#endif

#define SET_FUNC(name) CONCAT(Set_, CONCAT(SET_TYPE_NAME, _##name))
#define SET_STRUCT CONCAT(Set_, CONCAT(SET_TYPE_NAME, _t))

#define INITIAL_CAPACITY 10

typedef struct {
    SET_TYPE *arr;
    uint64_t count, capacity;
} SET_STRUCT;

static inline SET_STRUCT *SET_FUNC(Create)(void) {
    SET_STRUCT *set = malloc(sizeof(SET_STRUCT));
    if (set == NULL) {
        return NULL;
    }

    SET_TYPE *arr = (SET_TYPE *)(malloc(sizeof(SET_TYPE) * INITIAL_CAPACITY));
    if (arr == NULL) {
        free(set);
        return NULL;
    }

    *set = (SET_STRUCT){ .arr = arr, .count = 0, .capacity = INITIAL_CAPACITY };

    return set;
}

static inline void SET_FUNC(Free)(SET_STRUCT *set) {
    if (set == NULL) {
        return;
    }
}

#undef SET_FUNC
#undef SET_STRUCT
