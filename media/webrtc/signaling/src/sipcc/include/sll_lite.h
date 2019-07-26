



#ifndef _SLL_LITE_H
#define _SLL_LITE_H

#include "cpr_types.h"



typedef struct sll_lite_node_t_ {
    struct sll_lite_node_t_ *next_p;   
} sll_lite_node_t;




typedef struct sll_lite_list_t_ {
    uint16_t        count;     
    sll_lite_node_t *head_p;  
    sll_lite_node_t *tail_p;  
} sll_lite_list_t;

typedef enum {
    SLL_LITE_RET_SUCCESS,
    SLL_LITE_RET_INVALID_ARGS,
    SLL_LITE_RET_NODE_NOT_FOUND,
    SLL_LITE_RET_OTHER_FAILURE
} sll_lite_return_e;




#define SLL_LITE_LINK_HEAD(link) \
     (((sll_lite_list_t *)link)->head_p)

#define SLL_LITE_LINK_TAIL(link) \
     (((sll_lite_list_t *)link)->tail_p)

#define SLL_LITE_LINK_NEXT_NODE(node) \
     (((sll_lite_node_t *)node)->next_p)

#define SLL_LITE_NODE_COUNT(link) \
     (((sll_lite_list_t *)link)->count)












extern sll_lite_return_e
sll_lite_init(sll_lite_list_t *list);












 
extern sll_lite_return_e
sll_lite_link_head(sll_lite_list_t *list, sll_lite_node_t *node);













sll_lite_return_e
sll_lite_link_tail(sll_lite_list_t *list, sll_lite_node_t *node);












extern sll_lite_node_t *
sll_lite_unlink_head(sll_lite_list_t *list);












sll_lite_node_t *
sll_lite_unlink_tail(sll_lite_list_t *list);
















extern sll_lite_return_e
sll_lite_remove(sll_lite_list_t *list, sll_lite_node_t *node);

#endif
