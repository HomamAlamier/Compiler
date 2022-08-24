#ifndef LIST_H
#define LIST_H
#include <stdlib.h>
#include <include/types.h>

typedef struct list_node
{
    void* data;
    struct list_node* next;
} list_node_t;

typedef struct list
{
    list_node_t* top;
    list_node_t* bottom;
    size_t size;
} list_t;

#define LIST_FOREACH(LIST, FUNC)        \
    if (LIST) {                         \
        list_node_t* node = LIST->top;  \
        while(node) {                   \
            FUNC                        \
            node = node->next;          \
        }                               \
    }

list_t* init_list();
void list_push(list_t* list, void* data);
void list_free(list_t** list, bool free_data);
void list_free_data(list_t* list);
void* list_index_data(list_t* list, size_t index);
void list_append_list(list_t* list, list_t* list2);

#endif // LIST_H
