#ifndef NAZARK_QUEUE_H
#define NAZARK_QUEUE_H

#include <stdint.h>

typedef struct Node {
	void *data;
	struct Node* next;
} Node_t;

typedef struct Queue {
	Node_t *head;
	Node_t *tail;
	uint64_t size;
} Queue_t;

#endif // NAZARK_QUEUE_H
