#include <stdio.h>
#include <stdlib.h>

typedef struct node* Node;
typedef void* Pointer;
typedef struct queue* Queue;
typedef void (*DestroyFunc)(Pointer value);
#define QUEUE_BOF (Node)0
#define QUEUE_EOF (Node)0

// Create a queue FIFO return NULL if it fails
Queue queue_create(DestroyFunc destroy_value);

// return queue size
int queue_size(Queue queue);

// return the first element of the queue, if queue is empty return NULL 
Pointer queue_front(Queue queue);

// insert a value at the back of the queue
void queue_insert_back(Queue queue, Pointer value);

// remove the first element of the queue
void queue_remove_front(Queue queue);

// destroy the queue and all its elements witout leaks
void queue_destroy(Queue queue);

// for the nodes of the queue 


// return the first/last node of the queue if the queue is empty return QUEUE_BOF/QUEUE_EOF
Node queue_first(Queue queue);
Node queue_last(Queue queue);

// return the next node in the queue if is the last return QUEUE_EOF
Node queue_next(Queue list, Node node);

// return the value of the node
Pointer queue_node_value(Queue queue, Node node);