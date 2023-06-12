#include "queue.h"
#include <memory.h>
#include <stdlib.h>

bool queue_init(Queue* q, int size, int length)
{
    /* Power of two test */
    if ((length & (length - 1)) != 0)
        return false;
        
    q->items = malloc(size * length);

    if (!q)
        return false;

    memset(q->items, 0, size * length);
    q->head   = 0;
    q->tail   = 0;
    q->count  = 0;
    q->len    = length;
    q->size   = size;

    return true;
}

void queue_reset(Queue* q)
{
    memset(q->items, 0, q->size * q->len);
    q->head = 0;
    q->tail = 0;
    q->count  = 0;
}

void queue_free(Queue* q)
{
    if (q->items)
        free(q->items);
}

bool queue_empty(Queue* q)
{
    return (q->count == 0);
}

bool queue_full(Queue* q)
{
    return (q->count == q->len);
}

void* enqueue(Queue* q)
{
    /* Queue full */
    if (queue_full(q))
        return NULL;
    
    void* data = (char*)q->items + (q->head * q->size);

    /* memmove(
        (char*)q->items + (q->head * q->size), 
        data, 
        q->size
    ); */

    q->head = ((q->head + 1) & (q->len - 1));
    ++q->count;

    return data;
}

void* queue_peek(Queue* q)
{
    if (queue_empty(q))
        return NULL;

    return (char*)q->items + (q->tail * q->size);
}

void* dequeue(Queue* q)
{
    if (queue_empty(q))
        return NULL;
    
    /* memmove(
        buf, 
        (char*)q->items + (q->tail * q->size), 
        q->size
    ); */
    void* data = (char*)q->items + (q->tail * q->size);

    q->tail = ((q->tail + 1) & (q->len - 1));
    --q->count;

    return data;
}