
































































#include "cpr_types.h"
#include "sll_lite.h"






















sll_lite_return_e
sll_lite_init (sll_lite_list_t *list)
{
    if (list == NULL) { 
        return (SLL_LITE_RET_INVALID_ARGS);
    }
    list->count   = 0;
    list->head_p  = NULL; 
    list->tail_p  = NULL;
    return (SLL_LITE_RET_SUCCESS);
}













sll_lite_return_e
sll_lite_link_head (sll_lite_list_t *list, sll_lite_node_t *node)
{
    if ((list == NULL) || (node == NULL)) {
        return (SLL_LITE_RET_INVALID_ARGS);
    }

    if (list->head_p == NULL) {
        
        list->head_p = node;
        list->tail_p = node;
        node->next_p = NULL; 
    } else {
        
        node->next_p = list->head_p;
        list->head_p = node;
    }
    list->count++;

    return (SLL_LITE_RET_SUCCESS);
}













sll_lite_return_e
sll_lite_link_tail (sll_lite_list_t *list, sll_lite_node_t *node)
{
    if ((list == NULL) || (node == NULL)) {
        return (SLL_LITE_RET_INVALID_ARGS);
    }

    if (list->tail_p == NULL) {
        
        list->head_p = node;
        list->tail_p = node;
    } else {
        
        list->tail_p->next_p = node;
        list->tail_p = node;
    }
    node->next_p = NULL;
    list->count++;

    return (SLL_LITE_RET_SUCCESS);
}












sll_lite_node_t *
sll_lite_unlink_head (sll_lite_list_t *list)
{
    sll_lite_node_t *node_p = NULL;

    if (list == NULL) {
        return (NULL);
    }

    if (list->head_p !=  NULL) {
        node_p = list->head_p;
        list->head_p = list->head_p->next_p;
        if (list->tail_p == node_p) {
            
            list->tail_p = list->head_p;
        }
        list->count--;
        node_p->next_p = NULL;
    }
    return (node_p);
}












sll_lite_node_t *
sll_lite_unlink_tail (sll_lite_list_t *list)
{
    sll_lite_node_t *node_p;

    if ((list == NULL) || (list->tail_p == NULL)) {
        return (NULL);
    }

    
    node_p = list->tail_p;
    if (sll_lite_remove(list, node_p) != SLL_LITE_RET_SUCCESS) {
        
        return (NULL);
    }
    return (node_p);
}
















sll_lite_return_e
sll_lite_remove (sll_lite_list_t *list, sll_lite_node_t *node)
{
    sll_lite_node_t *this_p, *prev_p;

    if ((list == NULL) || (node == NULL)) {
        return (SLL_LITE_RET_INVALID_ARGS);
    }

    prev_p = NULL;
    this_p = list->head_p; 

    



    while (this_p != NULL) {
        if (this_p == node) {
            break;
        }
        prev_p = this_p;
        this_p = this_p->next_p;
    }

    if (this_p != NULL) {
       
       if (prev_p == NULL) {
           
           list->head_p = list->head_p->next_p;
           if (list->tail_p == this_p) {
               
               list->tail_p = list->head_p;
           }
       } else {
           
           prev_p->next_p = this_p->next_p;
           if (list->tail_p == this_p) {
               
               list->tail_p = prev_p;
           }
       }
       list->count--;
       node->next_p = NULL;
       return (SLL_LITE_RET_SUCCESS);
    }
    return (SLL_LITE_RET_NODE_NOT_FOUND);
}
