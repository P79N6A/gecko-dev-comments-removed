






































#ifndef _SINGLY_LINK_LIST_H
#define _SINGLY_LINK_LIST_H

typedef enum {
    SLL_MATCH_FOUND,
    SLL_MATCH_NOT_FOUND
} sll_match_e;






typedef sll_match_e(*sll_find_callback_t)(void *find_by_p, void *data_p);

typedef void *sll_handle_t;

typedef enum {
    SLL_RET_SUCCESS,
    SLL_RET_INVALID_ARGS,
    SLL_RET_MALLOC_FAILURE,
    SLL_RET_NODE_NOT_FOUND,
    SLL_RET_LIST_NOT_EMPTY,
    SLL_RET_OTHER_FAILURE
} sll_return_e;










extern sll_handle_t sll_create(sll_find_callback_t find_fp);












extern sll_return_e sll_destroy(sll_handle_t list_handle);













extern sll_return_e sll_append(sll_handle_t list_handle, void *data_p);













extern sll_return_e sll_remove(sll_handle_t list_handle, void *data_p);









extern void *sll_find(sll_handle_t list_handle, void *find_by_p);













extern void *sll_next(sll_handle_t list_handle, void *data_p);









extern unsigned int sll_count(sll_handle_t list_handle);

#endif
