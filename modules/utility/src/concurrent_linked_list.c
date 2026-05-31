#include "concurrent_linked_list.h"
#include "linked_list.h"

#include <pthread.h>
#include <stdlib.h>

struct ConcurrentLinkedList {
    LinkedList_t *innerList;
    pthread_mutex_t mutex;
};

ConcurrentLinkedList_t *
ConcurrentLinkedList_Create(void (*deallocator)(void *data)) {
    ConcurrentLinkedList_t *cll = malloc(sizeof(ConcurrentLinkedList_t));
    if (cll == NULL) {
        return NULL;
    }

    LinkedList_t *ll = LinkedList_Create(deallocator);
    if (ll == NULL) {
        free(cll);
        return NULL;
    }

    pthread_mutex_t mutex;
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        LinkedList_Free(ll);
        free(cll);
        return NULL;
    }

    *cll = (ConcurrentLinkedList_t){ .innerList = ll, .mutex = mutex };

    return cll;
}

int32_t ConcurrentLinkedList_Free(ConcurrentLinkedList_t *cll) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_Free(cll->innerList);
    if (rCode) {
        pthread_mutex_unlock(&cll->mutex);
        return rCode;
    }

    pthread_mutex_unlock(&cll->mutex);
    pthread_mutex_destroy(&cll->mutex);
    free(cll);

    return 0;
}

int32_t ConcurrentLinkedList_Length(ConcurrentLinkedList_t *cll,
                                    uint64_t *out) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_Length(cll->innerList, out);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_AddStart(ConcurrentLinkedList_t *cll, void *data) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_AddStart(cll->innerList, data);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_AddEnd(ConcurrentLinkedList_t *cll, void *data) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_AddEnd(cll->innerList, data);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_AddAt(ConcurrentLinkedList_t *cll, uint64_t idx, void *data) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_AddAt(cll->innerList, idx, data);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_PopStart(ConcurrentLinkedList_t *cll, void **outData) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_PopStart(cll->innerList, outData);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_PopEnd(ConcurrentLinkedList_t *cll, void **outData) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_PopEnd(cll->innerList, outData);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_PopAt(ConcurrentLinkedList_t *cll, uint64_t idx, void **outData) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_PopAt(cll->innerList, idx, outData);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_PeekStart(ConcurrentLinkedList_t *cll, void **outData) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_PeekStart(cll->innerList, outData);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_PeekEnd(ConcurrentLinkedList_t *cll, void **outData) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_PeekEnd(cll->innerList, outData);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_PeekAt(ConcurrentLinkedList_t *cll, uint64_t idx, void **outData) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_PeekAt(cll->innerList, idx, outData);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_NodeAt(ConcurrentLinkedList_t *cll, uint64_t idx, ListNode_t **outNode) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_NodeAt(cll->innerList, idx, outNode);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_RemoveNode(ConcurrentLinkedList_t *cll, ListNode_t *theNode) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_RemoveNode(cll->innerList, theNode);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}

int32_t ConcurrentLinkedList_Next(ConcurrentLinkedList_t *cll, ListNode_t **outNode, void **outData) {
    if (cll == NULL) {
        return LL_ERROR_NULL_POINTER_LIST;
    }

    pthread_mutex_lock(&cll->mutex);
    int32_t rCode = LinkedList_Next(cll->innerList, outNode, outData);
    pthread_mutex_unlock(&cll->mutex);

    return rCode;
}
