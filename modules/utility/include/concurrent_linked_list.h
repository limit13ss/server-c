#ifndef NAZARK_CONCURRENT_LINKED_LIST_H
#define NAZARK_CONCURRENT_LINKED_LIST_H

#include "linked_list.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct ConcurrentLinkedList ConcurrentLinkedList_t;

ConcurrentLinkedList_t *
ConcurrentLinkedList_Create(void (*deallocator)(void *data));
int32_t ConcurrentLinkedList_Free(ConcurrentLinkedList_t *cll);
int32_t ConcurrentLinkedList_Length(ConcurrentLinkedList_t *cll, uint64_t *out);

int32_t ConcurrentLinkedList_AddStart(ConcurrentLinkedList_t *cll, void *data);
int32_t ConcurrentLinkedList_AddEnd(ConcurrentLinkedList_t *cll, void *data);
int32_t ConcurrentLinkedList_AddAt(ConcurrentLinkedList_t *cll, uint64_t idx,
                                void *data);

int32_t ConcurrentLinkedList_PopStart(ConcurrentLinkedList_t *cll, void **outData);
int32_t ConcurrentLinkedList_PopEnd(ConcurrentLinkedList_t *cll, void **outData);
int32_t ConcurrentLinkedList_PopAt(ConcurrentLinkedList_t *cll, uint64_t idx,
                                void **outData);

int32_t ConcurrentLinkedList_PeekStart(ConcurrentLinkedList_t *cll, void **outData);
int32_t ConcurrentLinkedList_PeekEnd(ConcurrentLinkedList_t *cll, void **outData);
int32_t ConcurrentLinkedList_PeekAt(ConcurrentLinkedList_t *cll, uint64_t idx,
                                 void **outData);

int32_t ConcurrentLinkedList_NodeAt(ConcurrentLinkedList_t *cll, uint64_t idx, ListNode_t **outNode);
int32_t ConcurrentLinkedList_RemoveNode(ConcurrentLinkedList_t *cll, ListNode_t *theNode);

int32_t ConcurrentLinkedList_Next(ConcurrentLinkedList_t *cll, ListNode_t **outNode, void **outData);

#endif // NAZARK_CONCURRENT_LINKED_LIST_H
