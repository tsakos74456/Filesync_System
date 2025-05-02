#include "ADTQueue.h"

struct queue {
    int size;   //size of the queue
    Node dump;  // dummy node even if the queue is empty it exists and the first node is the next of it
    Node last;  // last node
    DestroyFunc destroy_value;	// Συνάρτηση που καταστρέφει ένα στοιχείο της λίστας.

};  

struct node {
	Node next;		// pointer to the next node
	Pointer value;		// the value of the node
};

Queue queue_create(DestroyFunc destroy_value) {
    Queue queue = malloc(sizeof(struct queue));
    if (queue == NULL) {
        return NULL;
    }
    queue->size = 0;
    queue->destroy_value = destroy_value;
    queue->dump = malloc(sizeof(struct node));
    if (queue->dump == NULL) {
        free(queue);
        return NULL;
    }
    queue->dump->next = QUEUE_EOF;
    queue->last = QUEUE_EOF;
    return queue;
};

void queue_insert_back(Queue queue, Pointer value) {
    Node node = malloc(sizeof(struct node));
    if (node == NULL) {
        return;
    }
    node->value = value;
    node->next = QUEUE_EOF;
    if (queue->size == 0) {
        queue->dump->next = node;
        queue->last = node;
    } else {
        queue->last->next = node;
        queue->last = node;
    }
    queue->size++;
};

void queue_remove_front(Queue queue) {
    if (queue->size == 0) {
        return;
    }
    Node node = queue->dump->next;
    queue->dump->next = node->next;
    if (queue->size == 1) {
        queue->last = QUEUE_EOF;
    }
    if (queue->destroy_value != NULL) {
        queue->destroy_value(node->value);
    }
    free(node);
    queue->size--;
};


int queue_size(Queue queue) {
    return queue->size;
};

Pointer queue_front(Queue queue) {
    if (queue->size == 0) {
        return QUEUE_BOF;
    }
    return queue->dump->next->value;
};

void queue_destroy(Queue queue) {
    if (queue == NULL) {
        return;
    }
    Node node = queue->dump->next;
    while (node != QUEUE_EOF) {
        Node next = node->next;
        if (queue->destroy_value != NULL) {
            queue->destroy_value(node->value);
        }
        free(node);
        node = next;
    }
    free(queue->dump);
    free(queue);
};





Node queue_first(Queue queue) {
    if (queue->size == 0) {
        return QUEUE_BOF;
    }
    return queue->dump->next;
};

Node queue_last(Queue queue) {
    if (queue->size == 0) {
        return QUEUE_EOF;
    }
    return queue->last->value;
};

Node queue_next(Queue queue, Node node) {
    (void)queue;
    if (node == QUEUE_EOF) {
        return QUEUE_EOF;
    }
    return node->next;
};

Pointer queue_node_value(Queue queue, Node node) {
    (void)queue;

    if (node == QUEUE_EOF) {
        return QUEUE_EOF;
    }
    return node->value;
};
