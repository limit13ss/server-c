#include "common.h"

#include <stdlib.h>

NKString NKString_Empty(void) { return (NKString){ .data = NULL, .len = 0 }; }
int8_t NKString_IsEmpty(NKString value) { return !value.data || !value.len; }

void NKString_Free(NKString str) {
    if (str.data) {
        free(str.data);
    }
}
