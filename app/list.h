#ifndef LIST_H__
#define LIST_H__


typedef void (*free_func)(void*);

typedef struct node {
    struct node* next;
    free_func free;
    void* data;
} node;

node* node_create(void* data, free_func f);
node* node_append(node* head, node* n);
void node_remove(node* n);
#else
#endif /* LIST_H__ */
