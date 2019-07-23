




































#ifndef prcountr_h___
#define prcountr_h___



































































#include "prtypes.h"

PR_BEGIN_EXTERN_C






typedef void *  PRCounterHandle;

#define PRCOUNTER_NAME_MAX 31
#define PRCOUNTER_DESC_MAX 255










#define PR_DEFINE_COUNTER(name) PRCounterHandle name









#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_INIT_COUNTER_HANDLE(handle,value)\
    (handle) = (PRCounterHandle)(value)
#else
#define PR_INIT_COUNTER_HANDLE(handle,value)
#endif






























#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_CREATE_COUNTER(handle,qName,rName,description)\
   (handle) = PR_CreateCounter((qName),(rName),(description))
#else
#define PR_CREATE_COUNTER(handle,qName,rName,description)
#endif

NSPR_API(PRCounterHandle) 
	PR_CreateCounter( 
		const char *qName, 
    	const char *rName, 
        const char *description 
);


















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_DESTROY_COUNTER(handle) PR_DestroyCounter((handle))
#else
#define PR_DESTROY_COUNTER(handle)
#endif

NSPR_API(void) 
	PR_DestroyCounter( 
		PRCounterHandle handle 
);






















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_GET_COUNTER_HANDLE_FROM_NAME(handle,qName,rName)\
    (handle) = PR_GetCounterHandleFromName((qName),(rName))
#else
#define PR_GET_COUNTER_HANDLE_FROM_NAME(handle,qName,rName)
#endif

NSPR_API(PRCounterHandle) 
	PR_GetCounterHandleFromName( 
    	const char *qName, 
    	const char *rName 
);





















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_GET_COUNTER_NAME_FROM_HANDLE(handle,qName,rName,description)\
    PR_GetCounterNameFromHandle((handle),(qName),(rName),(description))
#else
#define PR_GET_COUNTER_NAME_FROM_HANDLE(handle,qName,rName,description )
#endif

NSPR_API(void) 
	PR_GetCounterNameFromHandle( 
    	PRCounterHandle handle,  
	    const char **qName, 
	    const char **rName, 
		const char **description 
);


















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_INCREMENT_COUNTER(handle) PR_IncrementCounter(handle)
#else
#define PR_INCREMENT_COUNTER(handle)
#endif

NSPR_API(void) 
	PR_IncrementCounter( 
		PRCounterHandle handle
);



















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_DECREMENT_COUNTER(handle) PR_DecrementCounter(handle)
#else
#define PR_DECREMENT_COUNTER(handle)
#endif

NSPR_API(void) 
	PR_DecrementCounter( 
		PRCounterHandle handle
);


















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_ADD_TO_COUNTER(handle,value)\
    PR_AddToCounter((handle),(value))
#else
#define PR_ADD_TO_COUNTER(handle,value)
#endif

NSPR_API(void) 
	PR_AddToCounter( 
    	PRCounterHandle handle, 
	    PRUint32 value 
);






















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_SUBTRACT_FROM_COUNTER(handle,value)\
    PR_SubtractFromCounter((handle),(value))
#else
#define PR_SUBTRACT_FROM_COUNTER(handle,value)
#endif

NSPR_API(void) 
	PR_SubtractFromCounter( 
    	PRCounterHandle handle, 
	    PRUint32 value 
);


















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_GET_COUNTER(counter,handle)\
    (counter) = PR_GetCounter((handle))
#else
#define PR_GET_COUNTER(counter,handle) 0
#endif

NSPR_API(PRUint32) 
	PR_GetCounter( 
		PRCounterHandle handle 
);





















#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_SET_COUNTER(handle,value) PR_SetCounter((handle),(value))
#else
#define PR_SET_COUNTER(handle,value)
#endif

NSPR_API(void) 
	PR_SetCounter( 
		PRCounterHandle handle, 
		PRUint32 value 
);































#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_FIND_NEXT_COUNTER_QNAME(next,handle)\
    (next) = PR_FindNextCounterQname((handle))
#else
#define PR_FIND_NEXT_COUNTER_QNAME(next,handle) NULL
#endif

NSPR_API(PRCounterHandle) 
	PR_FindNextCounterQname( 
        PRCounterHandle handle
);
































#if defined(DEBUG) || defined(FORCE_NSPR_COUNTERS)
#define PR_FIND_NEXT_COUNTER_RNAME(next,rhandle,qhandle)\
    (next) = PR_FindNextCounterRname((rhandle),(qhandle))
#else
#define PR_FIND_NEXT_COUNTER_RNAME(next,rhandle,qhandle)
#endif

NSPR_API(PRCounterHandle) 
	PR_FindNextCounterRname( 
        PRCounterHandle rhandle,
        PRCounterHandle qhandle
);

PR_END_EXTERN_C

#endif 
