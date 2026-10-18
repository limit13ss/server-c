#include "common.h"

#include <stdlib.h>
#include <string.h>

NKBuffer NKBuffer_Empty(void) {
    return (NKBuffer){ .values = NULL, .capacity = 0, .length = 0 };
}
int8_t NKBuffer_IsValidAndEmpty(NKBuffer buf) {
    return buf.values != NULL && buf.length == 0;
}

void NKBuffer_Free(NKBuffer buf) {
    if (buf.values != NULL) {
        free(buf.values);
    }

    buf.values   = NULL;
    buf.capacity = 0;
    buf.length   = 0;

    return;
}

NKString NKString_Empty(void) { return (NKString){ .data = NULL, .length = 0 }; }
int8_t NKString_IsEmpty(NKString value) { return !value.data || !value.length; }

NKString NKString_CopyFrom(const void *str, uint64_t length) {
    if (!length) {
        return NKString_Empty();
    }

    uint8_t *ptr = calloc(length, sizeof(uint8_t));
    if (!ptr) {
        return NKString_Empty();
    }

    strncpy((char *)ptr, str, length);

    return (NKString){ .data = ptr, .length = length };
}

void NKString_Free(NKString str) {
    if (str.data) {
        free(str.data);
    }
    str.data   = NULL;
    str.length = 0;
}
