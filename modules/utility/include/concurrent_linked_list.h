#ifndef NAZARK_CONCURRENT_LINKED_LIST_H
#define NAZARK_CONCURRENT_LINKED_LIST_H

#include "linked_list.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct ConcurrentLinkedList ConcurrentLinkedList_t;

ConcurrentLinkedList_t *
ConcurrentLinkedList_Create(void (*deallocator)(void *data));
bool ConcurrentLinkedList_Free(ConcurrentLinkedList_t *ll);
bool ConcurrentLinkedList_Length(ConcurrentLinkedList_t *ll, uint64_t *out);

bool ConcurrentLinkedList_AddStart(ConcurrentLinkedList_t *ll, void *data);
bool ConcurrentLinkedList_AddEnd(ConcurrentLinkedList_t *ll, void *data);
bool ConcurrentLinkedList_AddAt(ConcurrentLinkedList_t *ll, uint64_t idx,
                                void *data);

bool ConcurrentLinkedList_PopStart(ConcurrentLinkedList_t *ll, void **outData);
bool ConcurrentLinkedList_PopEnd(ConcurrentLinkedList_t *ll, void **outData);
bool ConcurrentLinkedList_PopAt(ConcurrentLinkedList_t *ll, uint64_t idx,
                                void **outData);

bool ConcurrentLinkedList_PeekStart(ConcurrentLinkedList_t *ll, void **outData);
bool ConcurrentLinkedList_PeekEnd(ConcurrentLinkedList_t *ll, void **outData);
bool ConcurrentLinkedList_PeekAt(ConcurrentLinkedList_t *ll, uint64_t idx,
                                 void **outData);

bool ConcurrentLinkedList_NodeAt(ConcurrentLinkedList_t *ll, uint64_t idx, ListNode_t *outNode);
bool ConcurrentLinkedList_RemoveNode(ConcurrentLinkedList_t *ll, ListNode_t *Node);

#endif // NAZARK_CONCURRENT_LINKED_LIST_H
