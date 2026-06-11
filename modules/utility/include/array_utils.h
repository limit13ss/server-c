#ifndef NAZARK_ARRAY_UTILS_H
#define NAZARK_ARRAY_UTILS_H

#include <stdint.h>

int64_t indexOf(const uint8_t *arr, uint32_t arrLen, uint8_t targetCh);
int64_t indexOfSeq(const uint8_t *arr, uint32_t arrLen,
                   const uint8_t *targetSeq, uint32_t targetLen);
int64_t indexOfSeqOff(const uint8_t *arr, uint32_t arrLen,
                      const uint8_t *targetSeq, uint32_t targetLen,
                      uint32_t arrOffset);

#endif // NAZARK_ARRAY_UTILS_H
