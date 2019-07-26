






































#include "cpr_stdio.h"
#include "cpr_stdlib.h"
#include "singly_link_list.h"





typedef struct slink_list_node_t {
    struct slink_list_node_t *next_p; 
    void                     *data_p; 
} slink_list_node_t;

typedef struct {
    slink_list_node_t  *first_p; 
    slink_list_node_t  *last_p;  
    unsigned int        count;   
    sll_find_callback_t find_fp; 
} slink_list_t;



















sll_handle_t
sll_create (sll_find_callback_t find_fp)
{
    slink_list_t *link_list_p;

    link_list_p = (slink_list_t *) cpr_malloc(sizeof(slink_list_t));
    if (link_list_p == NULL) {
        return NULL;
    }
    link_list_p->first_p = NULL;
    link_list_p->last_p = NULL;
    link_list_p->count = 0;
    link_list_p->find_fp = find_fp;

    return (sll_handle_t) link_list_p;
}










sll_return_e
sll_destroy (sll_handle_t list_handle)
{
    slink_list_t *list_p = (slink_list_t *) list_handle;

    


    if (list_p == NULL) {
        return SLL_RET_INVALID_ARGS;
    }

    


    if (list_p->count != 0) {
        return SLL_RET_LIST_NOT_EMPTY;
    }

    


    cpr_free(list_p);
    return SLL_RET_SUCCESS;
}











sll_return_e
sll_append (sll_handle_t list_handle, void *data_p)
{
    slink_list_t *list_p = (slink_list_t *) list_handle;
    slink_list_node_t *list_node_p;

    


    if ((list_p == NULL) || (data_p == NULL)) {
        return SLL_RET_INVALID_ARGS;
    }

    


    list_node_p = (slink_list_node_t *) cpr_malloc(sizeof(slink_list_node_t));
    if (list_node_p == NULL) {
        return SLL_RET_MALLOC_FAILURE;
    }
    list_node_p->next_p = NULL;
    list_node_p->data_p = data_p;

    


    if (list_p->first_p == NULL) { 
        list_p->first_p = list_p->last_p = list_node_p;
    } else {
        list_p->last_p->next_p = list_node_p;
        list_p->last_p = list_node_p;
    }
    list_p->count = list_p->count + 1;

    return SLL_RET_SUCCESS;
}











sll_return_e
sll_remove (sll_handle_t list_handle, void *data_p)
{
    slink_list_t *list_p = (slink_list_t *) list_handle;
    slink_list_node_t *list_node_p;
    slink_list_node_t *prev_node_p;

    


    if ((list_p == NULL) || (data_p == NULL)) {
        return SLL_RET_INVALID_ARGS;
    }

    


    prev_node_p = NULL;
    list_node_p = list_p->first_p;
    while (list_node_p) {
        if (list_node_p->data_p == data_p) {
            break;
        }
        prev_node_p = list_node_p;
        list_node_p = list_node_p->next_p;
    }
    if (list_node_p == NULL) {
        return SLL_RET_NODE_NOT_FOUND;
    }

    


    if (prev_node_p == NULL) { 
        list_p->first_p = list_node_p->next_p;
        if (list_p->last_p == list_node_p) { 
            list_p->last_p = list_p->first_p;
        }
    } else {
        prev_node_p->next_p = list_node_p->next_p;
        if (list_p->last_p == list_node_p) { 
            list_p->last_p = prev_node_p;
        }
    }
    cpr_free(list_node_p);
    list_p->count = list_p->count - 1;

    return SLL_RET_SUCCESS;
}










void *
sll_find (sll_handle_t list_handle, void *find_by_p)
{
    slink_list_t *list_p = (slink_list_t *) list_handle;
    slink_list_node_t *list_node_p;
    sll_match_e match;

    


    if ((list_p == NULL) || (find_by_p == NULL) || (list_p->find_fp == NULL)) {
        return NULL;
    }

    list_node_p = list_p->first_p;
    while (list_node_p) {
        match = (*(list_p->find_fp))(find_by_p, list_node_p->data_p);
        if (match == SLL_MATCH_FOUND) {
            break;
        }
        list_node_p = list_node_p->next_p;
    }

    if (list_node_p == NULL) {
        return NULL;
    }

    return list_node_p->data_p;
}











void *
sll_next (sll_handle_t list_handle, void *data_p)
{
    slink_list_t *list_p = (slink_list_t *) list_handle;
    slink_list_node_t *list_node_p;

    


    if (list_p == NULL) {
        return NULL;
    }

    if (data_p == NULL) { 
        if (list_p->first_p == NULL) {
            return NULL;
        }
        return list_p->first_p->data_p;
    }

    list_node_p = list_p->first_p;
    while (list_node_p) {
        if (list_node_p->data_p == data_p) {
            break;
        }
        list_node_p = list_node_p->next_p;
    }

    if (list_node_p == NULL) {
        return NULL;
    }

    if (list_node_p->next_p == NULL) {
        return NULL;
    }

    return list_node_p->next_p->data_p;
}









unsigned int
sll_count (sll_handle_t list_handle)
{
    slink_list_t *list_p = (slink_list_t *) list_handle;

    return (list_p->count);
}


