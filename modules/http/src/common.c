#include "common.h"

#include <stdlib.h>
#include <string.h>

NKBuffer NKBuffer_Empty(void) {
    return (NKBuffer){ .values = NULL, .capacity = 0, .length = 0 };
}
int8_t NKBuffer_IsValidAndEmpty(NKBuffer buf) {
    return buf.values && !buf.length;
}

void NKBuffer_Free(NKBuffer buf) {
    if (!buf.values) {
        return;
    }
    free(buf.values);
    buf.capacity = 0;
    buf.length   = 0;

    return;
}

NKString NKString_Empty(void) { return (NKString){ .data = NULL, .len = 0 }; }
int8_t NKString_IsEmpty(NKString value) { return !value.data || !value.len; }

NKString NKString_CopyFrom(const char *str, uint64_t len) {
    if (!len) {
        return NKString_Empty();
    }

    uint8_t *ptr = calloc(len, sizeof(uint8_t));
    if (!ptr) {
        return NKString_Empty();
    }

    if (strncpy((char *)ptr, str, len) != 0) {
        free(ptr);
        return NKString_Empty();
    }

    return (NKString){ .data = ptr, .len = len };
}

void NKString_Free(NKString str) {
    if (str.data) {
        free(str.data);
    }
}
