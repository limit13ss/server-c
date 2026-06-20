#ifndef NAZARK_ARRAY_UTILS_H
#define NAZARK_ARRAY_UTILS_H

#include <stdint.h>

int64_t indexOf(const uint8_t *arr, uint32_t arrLen, uint8_t targetCh);

/// ignores first skipCount chars in arr, but returns targetCh index
/// counting from the beginning of the arr
int64_t indexOfSkip(const uint8_t *arr, uint32_t arrLen, uint8_t targetCh,
                    uint32_t skipCount);
int64_t indexOfSeq(const uint8_t *arr, uint32_t arrLen,
                   const uint8_t *targetSeq, uint32_t targetLen);

/// ignores first skipCount chars in arr, but returns targetSeq index
/// counting from the beginning of the arr
int64_t indexOfSeqSkip(const uint8_t *arr, uint32_t arrLen,
                       const uint8_t *targetSeq, uint32_t targetLen,
                       uint32_t skipCount);

#endif // NAZARK_ARRAY_UTILS_H
