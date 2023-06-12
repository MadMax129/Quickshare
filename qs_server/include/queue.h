#ifndef QS_QUEUE
#define QS_QUEUE

#include <stdbool.h>

typedef struct {
    void* items;
    int head, 
        tail,
        len,
        size,
        count;
} Queue;

bool queue_init(Queue* q, int size, int length);
void queue_free(Queue* q);
void queue_reset(Queue* q);
bool queue_empty(Queue* q);
void* queue_peek(Queue* q);
void* enqueue(Queue* q);
void* dequeue(Queue* q);

#endif /* QS_QUEUE */