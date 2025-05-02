///////////////////////////////////////////////////////////////////
//
// ADT List
//
///////////////////////////////////////////////////////////////////


typedef void* Pointer;
#define LIST_BOF (ListNode)0
#define LIST_EOF (ListNode)0
typedef struct list* List;
typedef struct list_node* ListNode;
typedef void (*DestroyFunc)(Pointer value);

// create a list and pass a destroy function 
List list_create(DestroyFunc destroy_value);

// return list's size
int list_size(List list);

// insert a node after the given node, if we want to put it in the beginning just use node = LIST_BOF
void list_insert_next(List list, ListNode node, Pointer value);


// removes the next node compared to the given one if we want to remove the first one just use node = LIST_BOF 
void list_remove_next(List list, ListNode node);

// destroys list
void list_destroy(List list);




// retutns the first and last node accordingly
ListNode list_first(List list);
ListNode list_last(List list);

// returns the next node 
ListNode list_next(List list, ListNode node);

// returns the value of the node
Pointer list_node_value(List list, ListNode node);

