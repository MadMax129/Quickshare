#include <memory.h>
#include <stdlib.h>

#include "queue.h"
#include "mem.h"

bool queue_init(Queue* q, int size, int length)
{
    /* Power of two test */
    if ((length & (length - 1)) != 0)
        return false;
        
    q->items = alloc((size_t)size * (size_t)length);

    if (!q->items)
        return false;

    queue_reset(q);
    q->len    = length;
    q->size   = size;

    return true;
}

void queue_reset(Queue* q)
{
    memset(
        q->items, 
        0, 
        (size_t)(q->size * q->len)
    );
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
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