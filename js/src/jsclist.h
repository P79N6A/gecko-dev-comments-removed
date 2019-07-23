





































#ifndef jsclist_h___
#define jsclist_h___

#include "jstypes.h"




typedef struct JSCListStr {
    struct JSCListStr *next;
    struct JSCListStr *prev;
} JSCList;




#define JS_INSERT_BEFORE(_e,_l)  \
    JS_BEGIN_MACRO               \
        (_e)->next = (_l);       \
        (_e)->prev = (_l)->prev; \
        (_l)->prev->next = (_e); \
        (_l)->prev = (_e);       \
    JS_END_MACRO




#define JS_INSERT_AFTER(_e,_l)   \
    JS_BEGIN_MACRO               \
        (_e)->next = (_l)->next; \
        (_e)->prev = (_l);       \
        (_l)->next->prev = (_e); \
        (_l)->next = (_e);       \
    JS_END_MACRO




#define JS_NEXT_LINK(_e)         \
        ((_e)->next)



#define JS_PREV_LINK(_e)         \
        ((_e)->prev)




#define JS_APPEND_LINK(_e,_l) JS_INSERT_BEFORE(_e,_l)




#define JS_INSERT_LINK(_e,_l) JS_INSERT_AFTER(_e,_l)


#define JS_LIST_HEAD(_l) (_l)->next
#define JS_LIST_TAIL(_l) (_l)->prev




#define JS_REMOVE_LINK(_e)             \
    JS_BEGIN_MACRO                     \
        (_e)->prev->next = (_e)->next; \
        (_e)->next->prev = (_e)->prev; \
    JS_END_MACRO





#define JS_REMOVE_AND_INIT_LINK(_e)    \
    JS_BEGIN_MACRO                     \
        (_e)->prev->next = (_e)->next; \
        (_e)->next->prev = (_e)->prev; \
        (_e)->next = (_e);             \
        (_e)->prev = (_e);             \
    JS_END_MACRO





#define JS_CLIST_IS_EMPTY(_l) \
    ((_l)->next == (_l))




#define JS_INIT_CLIST(_l)  \
    JS_BEGIN_MACRO         \
        (_l)->next = (_l); \
        (_l)->prev = (_l); \
    JS_END_MACRO

#define JS_INIT_STATIC_CLIST(_l) \
    {(_l), (_l)}

#endif 
