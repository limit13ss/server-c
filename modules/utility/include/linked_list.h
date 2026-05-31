#ifndef NAZARK_LINKED_LIST_H
#define NAZARK_LINKED_LIST_H

#include <stdint.h>

typedef struct ListNode ListNode_t;

typedef struct LinkedList LinkedList_t;

LinkedList_t* LinkedList_Create(void (*deallocator)(void *data));
int32_t LinkedList_Free(LinkedList_t *ll);
int32_t LinkedList_Length(LinkedList_t *ll, uint64_t *out);

int32_t LinkedList_AddStart(LinkedList_t *ll, void *data);
int32_t LinkedList_AddEnd(LinkedList_t *ll, void *data);
int32_t LinkedList_AddAt(LinkedList_t *ll, uint64_t idx, void *data);

int32_t LinkedList_PopStart(LinkedList_t *ll, void **outData);
int32_t LinkedList_PopEnd(LinkedList_t *ll, void **outData);
int32_t LinkedList_PopAt(LinkedList_t *ll, uint64_t idx, void **outData);

int32_t LinkedList_PeekStart(LinkedList_t *ll, void **outData);
int32_t LinkedList_PeekEnd(LinkedList_t *ll, void **outData);
int32_t LinkedList_PeekAt(LinkedList_t *ll, uint64_t idx, void **outData);

int32_t LinkedList_NodeAt(LinkedList_t *ll, uint64_t idx, ListNode_t **outNode);
int32_t LinkedList_RemoveNode(LinkedList_t *ll, ListNode_t *theNode);

int32_t LinkedList_Next(LinkedList_t *ll, ListNode_t **outNode, void **outData);

#define LL_ERROR_NULL_POINTER_LIST -1
#define LL_ERROR_NULL_POINTER_ARG -2
#define LL_ERROR_INDEX_OUT_OF_BOUNDS -3
#define LL_ERROR_READ_FROM_EMPTY -4
#define LL_ERROR_ALIEN_NODE -5
#define LL_ERROR_MALLOC_FAILURE -6

#endif // NAZARK_LINKED_LIST_H
