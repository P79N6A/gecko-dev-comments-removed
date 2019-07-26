






































#ifndef _CPR_WIN_LOCKS_H_
#define _CPR_WIN_LOCKS_H_




typedef struct {
	
	int noWaiters; 
	
	
	CRITICAL_SECTION noWaitersLock; 

	
	HANDLE sema; 

} cpr_condition_t;

#endif

