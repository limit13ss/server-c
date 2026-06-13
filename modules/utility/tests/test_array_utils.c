#include "array_utils.h"
#include "test_common.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#define ALPHABET "abcdefghijklmnopqrstuvwxyz"
#define ALPHABET_LENGTH 26

void testIndexOf_NullString(void) {
    char *arr = NULL;

    assert(indexOf((uint8_t *)arr, 123456, '1') == -1);
}

void testIndexOf_EmptyString(void) {
    char *arr = "";

    assert(indexOf((uint8_t *)arr, 0, '1') == -1);
}

void testIndexOf_NonExistentChar(void) {
    char *arr = ALPHABET;

    assert(indexOf((uint8_t *)arr, ALPHABET_LENGTH, 'A') == -1);
}

void testIndexOf_ExistingChar(void) {
    char *arr = ALPHABET;

    for (size_t i = 0; i < ALPHABET_LENGTH; ++i) {
        assert(indexOf((uint8_t *)arr, ALPHABET_LENGTH, (uint8_t)arr[i]) ==
               (int64_t)i);
    }
}

void testIndexOfSeq_NullString(void) {
    char *arr       = NULL;
    char *targetSeq = "1";

    assert(indexOfSeq((uint8_t *)arr, 123456, (uint8_t *)targetSeq, 1) == -1);

    arr       = "123";
    targetSeq = NULL;
    assert(indexOfSeq((uint8_t *)arr, 3, (uint8_t *)targetSeq, 123456) == -1);
}

void testIndexOfSeq_EmptyString(void) {
    char *arr       = "";
    char *targetSeq = "1";

    assert(indexOfSeq((uint8_t *)arr, 0, (uint8_t *)targetSeq, 1) == -1);

    arr       = "1";
    targetSeq = "";
    assert(indexOfSeq((uint8_t *)arr, 1, (uint8_t *)targetSeq, 0) == -1);
}

void testIndexOfSeq_StringShorterThanTarget(void) {
    char *arr       = "123";
    char *targetSeq = "1234";

    assert(indexOfSeq((uint8_t *)arr, 3, (uint8_t *)targetSeq, 4) == -1);
}

void testIndexOfSeq_NonExistentTarget(void) {
    char *arr       = "aabbccdd";
    char *targetSeq = "abc";

    assert(indexOfSeq((uint8_t *)arr, 8, (uint8_t *)targetSeq, 3) == -1);

    targetSeq = "bcd";
    assert(indexOfSeq((uint8_t *)arr, 8, (uint8_t *)targetSeq, 3) == -1);

    targetSeq = "bbb";
    assert(indexOfSeq((uint8_t *)arr, 8, (uint8_t *)targetSeq, 3) == -1);
}

void testIndexOfSeq_ExistingTarget(void) {
    char *arr       = "aabbccddee";
    char *targetSeq = "ab";

    assert(indexOfSeq((uint8_t *)arr, 10, (uint8_t *)targetSeq, 2) == 1);

    targetSeq = "bccd";
    assert(indexOfSeq((uint8_t *)arr, 10, (uint8_t *)targetSeq, 4) == 3);

    targetSeq = "e";
    assert(indexOfSeq((uint8_t *)arr, 10, (uint8_t *)targetSeq, 1) == 8);

    targetSeq = "ee";
    assert(indexOfSeq((uint8_t *)arr, 10, (uint8_t *)targetSeq, 2) == 8);

    targetSeq = "aabbccddee";
    assert(indexOfSeq((uint8_t *)arr, 10, (uint8_t *)targetSeq, 10) == 0);

    targetSeq = "abbccddee";
    assert(indexOfSeq((uint8_t *)arr, 10, (uint8_t *)targetSeq, 9) == 1);
}

void testIndexOfSeqOff_NullString(void) {
    char *arr       = NULL;
    char *targetSeq = "1";

    assert(indexOfSeqOff((uint8_t *)arr, 123456, (uint8_t *)targetSeq, 1, 0) ==
           -1);

    arr       = "123";
    targetSeq = NULL;
    assert(indexOfSeqOff((uint8_t *)arr, 3, (uint8_t *)targetSeq, 123456, 0) ==
           -1);
}

void testIndexOfSeqOff_EmptyString(void) {
    char *arr       = "";
    char *targetSeq = "1";

    assert(indexOfSeqOff((uint8_t *)arr, 0, (uint8_t *)targetSeq, 1, 0) == -1);

    arr       = "123";
    targetSeq = "";
    assert(indexOfSeqOff((uint8_t *)arr, 3, (uint8_t *)targetSeq, 0, 0) == -1);
}

void testIndexOfSeqOff_OffsetBiggerThanStringLength(void) {
    char *arr       = "12345";
    char *targetSeq = "111";

    assert(indexOfSeqOff((uint8_t *)arr, 5, (uint8_t *)targetSeq, 3, 1234567) ==
           -1);
    assert(indexOfSeqOff((uint8_t *)arr, 5, (uint8_t *)targetSeq, 3, 3) == -1);
}

void testIndexOfSeqOff_TargetIsBeforeOffset(void) {
    char *arr       = "12345";
    char *targetSeq = "23";

    assert(indexOfSeqOff((uint8_t *)arr, 5, (uint8_t *)targetSeq, 2, 3) == -1);
}

void testIndexOfSeqOff_ValidCases(void) {
    char *arr       = ALPHABET;
    char *targetSeq = "de";

    assert(indexOfSeqOff((uint8_t *)arr, ALPHABET_LENGTH, (uint8_t *)targetSeq,
                         2, 0) == 3);
    assert(indexOfSeqOff((uint8_t *)arr, ALPHABET_LENGTH, (uint8_t *)targetSeq,
                         2, 2) == 3);
    assert(indexOfSeqOff((uint8_t *)arr, ALPHABET_LENGTH, (uint8_t *)targetSeq,
                         2, 3) == 3);

    targetSeq = "xyz";
    assert(indexOfSeqOff((uint8_t *)arr, ALPHABET_LENGTH, (uint8_t *)targetSeq,
                         2, 22) == 23);
    assert(indexOfSeqOff((uint8_t *)arr, ALPHABET_LENGTH, (uint8_t *)targetSeq,
                         2, 23) == 23);
}

int main(void) {
    RUN(testIndexOf_NullString);
    RUN(testIndexOf_EmptyString);
    RUN(testIndexOf_NonExistentChar);
    RUN(testIndexOf_ExistingChar);

    RUN(testIndexOfSeq_NullString);
    RUN(testIndexOfSeq_EmptyString);
    RUN(testIndexOfSeq_StringShorterThanTarget);
    RUN(testIndexOfSeq_NonExistentTarget);
    RUN(testIndexOfSeq_ExistingTarget);

    RUN(testIndexOfSeqOff_NullString);
    RUN(testIndexOfSeqOff_EmptyString);
    RUN(testIndexOfSeqOff_EmptyString);
    RUN(testIndexOfSeqOff_TargetIsBeforeOffset);
    RUN(testIndexOfSeqOff_ValidCases);

    return 0;
}
