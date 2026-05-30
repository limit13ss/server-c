#include "linked_list.h"

#include <stdlib.h>

struct ListNode {
    void *data;
    ListNode_t *next;
    ListNode_t *prev;

    LinkedList_t *holderList;
};

/// Law 1.
/// If head == NULL;
///     Then tail == NULL && length == 0.
///     (and vice versa)
struct LinkedList {
    ListNode_t *head;
    ListNode_t *tail;
    uint64_t length;
    void (*deallocator)(void *data);
};

LinkedList_t *LinkedList_Create(void (*deallocator)(void *data)) {
    LinkedList_t *ll = malloc(sizeof(LinkedList_t));
    if (ll == NULL) {
        return NULL;
    }

    *ll = (LinkedList_t){
        .head = NULL, .tail = NULL, .length = 0, .deallocator = deallocator
    };
    return ll;
}

int32_t LinkedList_Free(LinkedList_t *ll) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    while (ll->head != NULL) {
        if (ll->deallocator != NULL) {
            ll->deallocator(ll->head->data);
        }

        ListNode_t *next = ll->head->next;

        free(ll->head);
        ll->head = next;
    }
    free(ll);

    return 0;
}

int32_t LinkedList_Length(LinkedList_t *ll, uint64_t *out) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    *out = ll->length;
    return 0;
}

int32_t LinkedList_AddStart(LinkedList_t *ll, void *data) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (data == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }

    ListNode_t *newNode = malloc(sizeof(ListNode_t));
    if (newNode == NULL) {
        return LL_ERROR_MALLOC_FAILURE;
    }
    newNode->holderList = ll;
    newNode->data       = data;
    newNode->prev       = NULL;
    newNode->next       = ll->head;

    ll->head->prev = newNode;
    ll->head       = newNode;

    if (ll->length == 0) {
        ll->tail = newNode;
    }

    ++ll->length;
    return 0;
}

int32_t LinkedList_AddEnd(LinkedList_t *ll, void *data) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (data == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }

    ListNode_t *newNode = malloc(sizeof(ListNode_t));
    if (newNode == NULL) {
        return LL_ERROR_MALLOC_FAILURE;
    }
    newNode->holderList = ll;
    newNode->data       = data;
    newNode->prev       = ll->tail;
    newNode->next       = NULL;

    if (ll->length == 0) {
        ll->head = newNode;
    } else {
        ll->tail->next = newNode;
    }
    ll->tail = newNode;

    ++ll->length;
    return 0;
}

int32_t LinkedList_AddAt(LinkedList_t *ll, uint64_t idx, void *data) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (data == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (idx > ll->length) {
        return LL_ERROR_INDEX_OUT_OF_BOUNDS;
    }

    if (idx == 0) {
        return LinkedList_AddStart(ll, data);
    }
    if (idx == ll->length) {
        return LinkedList_AddEnd(ll, data);
    }

    ListNode_t *newNode = malloc(sizeof(ListNode_t));
    if (newNode == NULL) {
        return LL_ERROR_MALLOC_FAILURE;
    }
    newNode->holderList = ll;
    newNode->data       = data;

    ListNode_t *prev = ll->head;
    while (idx != 0) {
        prev = prev->next;
        --idx;
    }

    newNode->prev = prev;
    newNode->next = prev->next;
    prev->next    = newNode;

    ++ll->length;
    return 0;
}

int32_t LinkedList_PopStart(LinkedList_t *ll, void **outData) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (outData == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (ll->length == 0) {
        return LL_ERROR_READ_FROM_EMPTY;
    }

    *outData             = ll->head->data;
    ListNode_t *nextHead = ll->head->next;

    free(ll->head);
    --ll->length;

    if (nextHead == NULL) {
        ll->head = NULL;
        ll->tail = NULL;
    } else {
        nextHead->prev = NULL;
        ll->head       = nextHead;
    }

    return 0;
}

int32_t LinkedList_PopEnd(LinkedList_t *ll, void **outData) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (outData == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (ll->length == 0) {
        return LL_ERROR_READ_FROM_EMPTY;
    }

    *outData             = ll->tail->data;
    ListNode_t *nextTail = ll->tail->prev;

    free(ll->tail);
    --ll->length;

    if (nextTail == NULL) {
        ll->head = NULL;
        ll->tail = NULL;
    } else {
        nextTail->next = NULL;
        ll->tail       = nextTail;
    }

    return 0;
}

int32_t LinkedList_PopAt(LinkedList_t *ll, uint64_t idx, void **outData) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (outData == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (idx >= ll->length) {
        return LL_ERROR_INDEX_OUT_OF_BOUNDS;
    }

    if (idx == 0) {
        return LinkedList_PopStart(ll, outData);
    }
    if (idx == ll->length - 1) {
        return LinkedList_PopEnd(ll, outData);
    }

    ListNode_t *theNode;
    int32_t error = !LinkedList_NodeAt(ll, idx, &theNode);
    if (error) {
        return error;
    }

    theNode->prev->next = theNode->next;
    theNode->next->prev = theNode->prev;

    *outData = theNode->data;

    free(theNode);
    --ll->length;

    return 0;
}

int32_t LinkedList_PeekStart(LinkedList_t *ll, void **outData) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (outData == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (ll->length == 0) {
        return LL_ERROR_READ_FROM_EMPTY;
    }

    *outData = ll->head->data;
    return 0;
}

int32_t LinkedList_PeekEnd(LinkedList_t *ll, void **outData) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (outData == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (ll->length == 0) {
        return LL_ERROR_READ_FROM_EMPTY;
    }

    *outData = ll->tail->data;
    return 0;
}

int32_t LinkedList_PeekAt(LinkedList_t *ll, uint64_t idx, void **outData) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (outData == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (idx >= ll->length) {
        return LL_ERROR_INDEX_OUT_OF_BOUNDS;
    }

    ListNode_t *theNode;
    int32_t error = LinkedList_NodeAt(ll, idx, &theNode);
    if (error) {
        return error;
    }

    *outData = theNode->data;
    return 0;
}

int32_t LinkedList_NodeAt(LinkedList_t *ll, uint64_t idx,
                          ListNode_t **outNode) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (outNode == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (idx >= ll->length) {
        return LL_ERROR_INDEX_OUT_OF_BOUNDS;
    }

    if (idx == 0) {
        *outNode = ll->head;
        return 0;
    }
    if (idx == ll->length - 1) {
        *outNode = ll->tail;
        return 0;
    }

    ListNode_t *theNode;
    if (idx > ll->length / 2) {
        uint64_t nodeIdx = ll->length - 1;

        theNode = ll->tail;
        while (idx != nodeIdx) {
            theNode = theNode->prev;
            ++idx;
        }
    } else {
        uint64_t nodeIdx = 0;

        theNode = ll->head;
        while (idx != nodeIdx) {
            theNode = theNode->next;
            ++nodeIdx;
        }
    }

    *outNode = theNode;
    return 0;
}

int32_t LinkedList_RemoveNode(LinkedList_t *ll, ListNode_t *theNode) {
    if (ll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }
    if (theNode == NULL) {
        return LL_ERROR_NULL_POINTER_ARG;
    }
    if (theNode->holderList != ll) {
        return LL_ERROR_ALIEN_NODE;
    }

    if (ll->length == 1) {
        ll->head = NULL;
        ll->tail = NULL;
    } else if (ll->head == theNode) {
        ll->head       = theNode->next;
        ll->head->prev = NULL;
    } else if (ll->tail == theNode) {
        ll->tail       = theNode->prev;
        ll->tail->next = NULL;
    } else {
        theNode->prev->next = theNode->next;
        theNode->next->prev = theNode->prev;
    }

    if (ll->deallocator != NULL) {
        ll->deallocator(theNode->data);
    }

    free(theNode);
    theNode = NULL;
    --ll->length;

    return 0;
}
