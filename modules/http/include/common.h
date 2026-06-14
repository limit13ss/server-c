#ifndef NAZARK_COMMON_H
#define NAZARK_COMMON_H

#include <stdint.h>

#define ERROR_SERVER_INVALID_SOCKET_DESCRIPTOR 1001

#define KiB 1024
#define MiB 1048576
#define GiB 1073741824

typedef struct {
    uint8_t *data;
    uint64_t len;
} NKString;

typedef struct {
    uint8_t *values;
    uint32_t capacity;
    uint32_t length;
} NKBuffer;

NKString NKString_Empty(void);
int8_t NKString_IsEmpty(NKString value);

void NKString_Free(NKString str);

#endif // NAZARK_COMMON_H
