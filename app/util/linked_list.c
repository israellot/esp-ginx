#include "mem.h"
#include "linked_list.h"
#include "osapi.h"

/** create anode
 */
static node* list_create_node (void* item) {
    node* n = (node*) os_malloc (sizeof(node));
    n->item = item;
    n->prev = NULL;
    n->next = NULL;
    return n;
}

/** add item to head
 */
void list_add_first (linked_list* _this,void* item) {
    node* newNode = list_create_node(item);
    node* head = _this->head;
    // list is empty
    if (head == NULL)
        _this->head = newNode;
    else { // has item(s)
        node* last = _this->tail;
        if (last == NULL) // only head node
            last = head;
        newNode->next = head;
        head->prev = newNode;
        _this->head = newNode;
        _this->tail = last;
    }

    _this->size++;
}

/** add item to tail
 */
void list_add_last (linked_list* _this, void* item) {
   node* newNode = list_create_node(item);
   node* head = _this->head;
   node* tail = _this->tail;
    // list is empty
    if (head == NULL)
        _this->head = newNode;
    else { // has item(s)
       node* lastNode = tail;
        if (tail == NULL) // only head node
            lastNode = head;
        lastNode->next = newNode;
        newNode->prev = lastNode;
        _this->tail = newNode;
    }
    _this->size++;
}

/** add item to any position
 */
void list_add (linked_list* _this, void* item, int position) {
     // index out of list size
     if (position > _this->size) {
       return;
    }
    // add to head
    if (position == 0) {
        list_add_first(_this, item);
    } else if (position == _this->size) {
        // add to tail
        list_add_last(_this, item);
    } else {
        // insert between head and tail

       node* n = _this->head;
        int i = 0;
        // loop until the position
        while (i < position) {
            n = n->next;
            i++;
        }
        // insert new node to position
        node* newNode = list_create_node(item);
        list_insert_before(_this, n, newNode);
        _this->size++;
    }
}


/** insert one node before another,
 * newNdoe, node and node->prev should not be null.
 */
void list_insert_before (linked_list* _this,node* n,node* newNode) {
    node* prev = n->prev;

    n->prev = newNode;
    newNode->next = n;
    prev->next = newNode;
    newNode->prev = prev;
}

/** get item from specific position
 */
void* list_get (linked_list* _this, int position) {
    // list is empty
    if (_this->size == 0) {
        //NODE_DBG("LinkedList#get: The list is empty.");
       return NULL;
    } else if (position >= _this->size) {
        // out of bound
        //NODE_DBG("LinkedList#get: Index out of bound");
        return NULL;
    }
    // get head item
    if (position == 0) {
        return list_get_first(_this);
    } else if (position+1 == _this->size) {
        // get tail item
        return list_get_last(_this);
    } else {
       node* node = _this->head;
        int i = 0;
        // loop until position
        while (i < position) {
            node = node->next;
            i++;
        }
        return node->item;
    }
}
/** get item from head
 */
void* list_get_first (linked_list* _this) {
    // list is empty
    if (_this->size == 0) {
        //NODE_DBG("LinkedList#getFirst: The list is empty.");
        return NULL;
    }
    return _this->head->item;
}

/** get item from tail
 */
void* list_get_last (linked_list* _this) {
    // list is empty
    if (_this->size == 0) {
        //NODE_DBG("LinkedList#getLast: The list is empty.");
        return NULL;
    }
    // only head node
    if (_this->size == 1) {
        return list_get_first(_this);
    }
    return _this->tail->item;
}


void* list_remove_node(linked_list* _this,node *n){

     // list is empty
    if (_this->size == 0) {
        //NODE_DBG("LinkedList#_remove: The list is empty.");
        return NULL;
    }

    if(n->prev!=NULL)
        n->prev->next = n->next;
    else
        _this->head=n->next;
    if(n->next!=NULL)
        n->next->prev = n->prev;
    else
        _this->tail = n->prev;

    void* r = n->item;

    os_free(n);

    return r;
}

/** get item and remove it from any position
 */
void* list_remove (linked_list* _this, int position) {
    // list is empty
    if (_this->size == 0) {
        //NODE_DBG("LinkedList#_remove: The list is empty.");
        return NULL;
    } else if (position >= _this->size) {
        // out of bound
        //NODE_DBG("LinkedList#_remove: Index out of bound");
        return NULL;
    }

    // remove from head
    if (position == 0) {
        return list_remove_first(_this);
    } else if (position+1 == _this->size) {
        // remove from tail
        return list_remove_last(_this);
    } else {
       node* n = _this->head;
       node* prev;
       node* next;
        int i = 0;
        void* item;
        // loop until position
        while (i < position) {
            n = n->next;
            i++;
        }
        item = n->item;
        // remove node from list
        prev = n->prev;
        next = n->next;
        prev->next = next;
        next->prev = prev;
        os_free(n);
        _this->size--;
        return item ;
    }
}
/** get and remove item from head
 */
void* list_remove_first (linked_list* _this) {
   node* head = _this->head;
   node* next;
    void* item;
    // list is empty
    if (head == NULL) {
        //NODE_DBG("LinkedList#_removeFirst: The list is empty.");
        return NULL;
    }
    item = head->item;
    next = head->next;
    _this->head = next;
    if (next != NULL) // has next item
        next->prev = NULL;
    os_free(head);
    _this->size--;
    if (_this->size <= 1) // empty or only head node
        _this->tail = NULL;
    return item;
}

/** get and remove item from tail
 */
void* list_remove_last (linked_list* _this) {
    // list is empty
    if (_this->size == 0) {
        //NODE_DBG("LinkedList#_removeLast: The list is empty.");
        return NULL;
    }
    if (_this->size == 1) { // only head node
        return list_remove_first(_this);
    } else {
       node* tail = _this->tail;
       node* prev = tail->prev;
        void* item = tail->item;
        prev->next = NULL;
        os_free(tail);
        if (_this->size > 1)
            _this->tail = prev;
        _this->size--;
        if (_this->size <= 1) // empty or only head node
            _this->tail = NULL;
        return item;
    }
}

/** create a LinkedList
 */
linked_list* create_linked_list () {

    linked_list *list = (linked_list *)os_malloc(sizeof(linked_list));
    init_linked_list(list);
    return list;
}

void init_linked_list(linked_list *list){
    
    os_memset(list,0,sizeof(linked_list));
    list->head = NULL;
    list->tail = NULL;
    
    list->size=0;
}
