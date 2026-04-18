#include "queue.h"

#include <stddef.h>	// for NULL

Queue_t Queue_Create(void) {
	return (Queue_t) {
		.head = NULL,
	 	.tail = NULL,
	 	.length = 0,
	};
}

void Queue_Free(Queue_t* q) {
}
