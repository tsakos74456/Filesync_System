#include <stdio.h>
#include "ADTList.h"
#include <stdlib.h>


// Ενα List είναι pointer σε αυτό το struct
struct list {
	ListNode dummy;				// carbage node so even if the list is empty we have a node
	ListNode last;				//  pointer to the last node
	int size;					// size
	DestroyFunc destroy_value;	
};

struct list_node {
	ListNode next;		// pointer to the next node
	Pointer value;		// value's node
};

List list_create(DestroyFunc destroy_value){

    List list = malloc(sizeof(*list));
    if (list == NULL) {
        return NULL;
    }

    list->destroy_value = destroy_value;
    list->size = 0;

    list->dummy = malloc(sizeof(*list->dummy));
    list->dummy->next = NULL;
    list->last = list->dummy;
    return list;
};

int list_size(List list){
    return list->size;
};

void list_insert_next(List list, ListNode node, Pointer value){
    if (node == NULL) {
        node = list->dummy;
    }
    
    ListNode new_node = malloc(sizeof(*new_node));
    if (new_node == NULL) {
        return;
    }

    new_node->value = value;
    new_node->next = node->next;

    node->next = new_node;

    
    list->size++;
    if(list->last == node){
        list->last = new_node;
    }
};

void list_remove_next(List list, ListNode node){
    if (node == NULL) {
        node = list->dummy;
    }
    
    if (node->next == NULL) {
        return;
    }

    ListNode to_remove = node->next;
    node->next = to_remove->next;

    if (list->destroy_value != NULL)
        list->destroy_value(to_remove->value);
        
    free(to_remove);

    list->size--;
    if (list->last == to_remove)
        list->last = node;
};

void list_destroy(List list) {
	
	ListNode node = list->dummy;
	while (node != NULL) {				
		ListNode next = node->next;		

		if (node != list->dummy && list->destroy_value != NULL)
			list->destroy_value(node->value);

		free(node);
		node = next;
	}

	free(list);
};

ListNode list_first(List list){
    if (list->size == 0) {
        return LIST_EOF;
    }
    return list->dummy->next;
};

ListNode list_last(List list){
    if (list->size == 0) {
        return LIST_EOF;
    }
    return list->last;  
};

ListNode list_next(List list, ListNode node){
    (void)list;
    if (node == NULL) {
        return LIST_EOF;
    }
    return node->next;
};

Pointer list_node_value(List list, ListNode node){
    (void)list;
    if (node == NULL) {
        return NULL;
    }
    return node->value;
}


