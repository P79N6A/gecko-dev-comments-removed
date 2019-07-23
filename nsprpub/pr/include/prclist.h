




































#ifndef prclist_h___
#define prclist_h___

#include "prtypes.h"

typedef struct PRCListStr PRCList;




struct PRCListStr {
    PRCList	*next;
    PRCList	*prev;
};




#define PR_INSERT_BEFORE(_e,_l)	 \
    PR_BEGIN_MACRO		 \
	(_e)->next = (_l);	 \
	(_e)->prev = (_l)->prev; \
	(_l)->prev->next = (_e); \
	(_l)->prev = (_e);	 \
    PR_END_MACRO




#define PR_INSERT_AFTER(_e,_l)	 \
    PR_BEGIN_MACRO		 \
	(_e)->next = (_l)->next; \
	(_e)->prev = (_l);	 \
	(_l)->next->prev = (_e); \
	(_l)->next = (_e);	 \
    PR_END_MACRO




#define PR_NEXT_LINK(_e)	 \
    	((_e)->next)



#define PR_PREV_LINK(_e)	 \
    	((_e)->prev)




#define PR_APPEND_LINK(_e,_l) PR_INSERT_BEFORE(_e,_l)




#define PR_INSERT_LINK(_e,_l) PR_INSERT_AFTER(_e,_l)


#define PR_LIST_HEAD(_l) (_l)->next
#define PR_LIST_TAIL(_l) (_l)->prev




#define PR_REMOVE_LINK(_e)	       \
    PR_BEGIN_MACRO		       \
	(_e)->prev->next = (_e)->next; \
	(_e)->next->prev = (_e)->prev; \
    PR_END_MACRO





#define PR_REMOVE_AND_INIT_LINK(_e)    \
    PR_BEGIN_MACRO		       \
	(_e)->prev->next = (_e)->next; \
	(_e)->next->prev = (_e)->prev; \
	(_e)->next = (_e);	       \
	(_e)->prev = (_e);	       \
    PR_END_MACRO





#define PR_CLIST_IS_EMPTY(_l) \
    ((_l)->next == (_l))




#define PR_INIT_CLIST(_l)  \
    PR_BEGIN_MACRO	   \
	(_l)->next = (_l); \
	(_l)->prev = (_l); \
    PR_END_MACRO

#define PR_INIT_STATIC_CLIST(_l) \
    {(_l), (_l)}

#endif 
