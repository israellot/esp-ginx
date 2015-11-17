/* 
 * File:   LinkedList.h
 * Author: me_000
 *
 * Created on 6 de Julho de 2015, 15:21
 */

#ifndef LINKEDLIST_H
#define	LINKEDLIST_H

#ifdef	__cplusplus
extern "C" {
#endif

    
/**
 * The Node struct,
 * contains item and the pointers that point to previous node/next node.
 */
typedef struct node {
    void* item;
    // previous node
    struct node* prev;
    // next node
    struct node* next;
} node;



/**
 * The LinkedList struct, contains the pointers that
 * point to first node and last node, the size of the LinkedList,
 * and the function pointers.
 */
typedef struct linked_list {
    node* head;
    node* tail;
    // size of this LinkedList
    int size;
} linked_list;

void list_add_first (linked_list* _this, void* item);
void list_add_last (linked_list* _this, void* item);
void list_add (linked_list* _this, void* item, int position);
void list_insert_before (linked_list* _this, node* previous_node, node* new_node);
void* list_get (linked_list* _this, int position);
void* list_get_first (linked_list* _this);
void* list_get_last (linked_list* _this);
void* list_remove_node(linked_list* _this, node *node_to_remove);
void* list_remove (linked_list* _this, int position);
void* list_remove_first (linked_list* _this);
void* list_remove_last (linked_list* _this);
linked_list* create_linked_list ();
void init_linked_list(linked_list *list);


#ifdef	__cplusplus
}
#endif

#endif	/* LINKEDLIST_H */

