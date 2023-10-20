#include <stdlib.h>
#include <stdio.h>
#include "list.h"

node* node_create(void* data, free_func f) {
    node* next = (node*) malloc(sizeof(node));
    next->next = NULL;
    next->data = data;
    next->free = f;
    return next;
}

node* node_append(node* head, node* n) {
    if (n == NULL) {
        return head;
    }

    if (head == NULL) {
        return n;
    }

    node* cur;
    for (cur = head; cur->next != NULL; cur = cur->next);
    cur->next = n;
    return head;
}

#ifdef DEBUG
typedef struct {
    int type;
    char * str;
} temp;
void node_print(node* ptr) {
    temp* v = (temp*)ptr->data;
    printf("freeing %s\n", v->str);
}
#endif

void node_remove(node *n) {
    if (n == NULL) {
        return;
    }

    #ifdef DEBUG
    node_print(n);
    #endif

    node* next = n->next;
    n->free(n->data);
    free(n);
    node_remove(next);
}

