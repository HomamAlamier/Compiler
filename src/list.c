#include <include/list.h>

list_t* init_list() {
    list_t* list = calloc(1, sizeof(struct list));
    list->top = NULL;
    list->bottom = NULL;
    list->size = 0;
    return list;
}

void list_push(list_t* list, void* data) {
    list_node_t* node = calloc(1, sizeof(struct list_node));
    node->data = data;
    node->next = NULL;
    if (list->top == NULL) {
        list->top = node;
        list->bottom = node;
    } else {
        list->bottom->next = node;
        list->bottom = node;
    }
    list->size++;
}

void list_free(list_t** list, bool free_data) {
    list_node_t* node = (*list)->top;
    while(node) {
        list_node_t* tmp = node;
        node = node->next;
        if (free_data)
            free(tmp->data);
        free(tmp);
    }
    free(*list);
    *list = NULL;
}

void* list_index_data(list_t* list, size_t index) {
    list_node_t* node = list->top;
    for(size_t i = 0; i < list->size; ++i) {
        if (i == index)
            return node->data;
        node = node->next;
    }
    return NULL;
}

void list_append_list(list_t* list, list_t* list2) {
    if (list == NULL || list2 == NULL)
        return;
    LIST_FOREACH(list2, {
        list_push(list, node->data);
    })
}
