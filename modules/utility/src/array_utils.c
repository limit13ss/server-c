#include "array_utils.h"

int64_t indexOf(const uint8_t *arr, uint32_t arrLen, uint8_t targetCh) {
    if (!arr || arrLen == 0) {
        return -1;
    }

    for (uint32_t i = 0; i < arrLen; ++i) {
        if (arr[i] == targetCh) {
            return i;
        }
    }

    return -1;
}

int64_t indexOfSeq(const uint8_t *arr, uint32_t arrLen,
                   const uint8_t *targetSeq, uint32_t targetLen) {
    if (!arr || !targetSeq || targetLen == 0 || targetLen > arrLen) {
        return -1;
    }

    uint32_t maxStartPos = arrLen - targetLen;
    for (uint32_t i = 0; i <= maxStartPos; ++i) {
        uint32_t ii = 0;

        while (ii < targetLen && arr[i + ii] == targetSeq[ii]) {
            ++ii;
        }

        if (ii == targetLen) {
            return i;
        }
    }

    return -1;
}

int64_t indexOfSeqOff(const uint8_t *arr, uint32_t arrLen,
                      const uint8_t *targetSeq, uint32_t targetLen,
                      uint32_t arrOffset) {
    if (!arr || !targetSeq || targetLen == 0 || arrOffset >= arrLen ||
        targetLen > arrLen - arrOffset) {
        return -1;
    }

    int64_t relIndex =
        indexOfSeq(arr + arrOffset, arrLen - arrOffset, targetSeq, targetLen);
    if (relIndex < 0) {
        return relIndex;
    }

    return arrOffset + relIndex;
}
