




































#ifndef prtrace_h___
#define prtrace_h___
























































#include "prtypes.h"
#include "prthread.h"
#include "prtime.h"

PR_BEGIN_EXTERN_C






typedef void *  PRTraceHandle;






typedef struct PRTraceEntry
{
    PRThread        *thread;        
    PRTraceHandle   handle;         
    PRTime          time;           
    PRUint32        userData[8];    
} PRTraceEntry;






typedef enum PRTraceOption
{
    PRTraceBufSize,
    PRTraceEnable,              
    PRTraceDisable,
    PRTraceSuspend,
    PRTraceResume,
    PRTraceSuspendRecording,
    PRTraceResumeRecording,
    PRTraceLockHandles,
    PRTraceUnLockHandles,
    PRTraceStopRecording
} PRTraceOption;








#define PR_DEFINE_TRACE(name) PRTraceHandle name









#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_INIT_TRACE_HANDLE(handle,value)\
    (handle) = (PRCounterHandle)(value)
#else
#define PR_INIT_TRACE_HANDLE(handle,value)
#endif


































#define PRTRACE_NAME_MAX 31
#define PRTRACE_DESC_MAX 255

#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_CREATE_TRACE(handle,qName,rName,description)\
    (handle) = PR_CreateTrace((qName),(rName),(description))
#else
#define PR_CREATE_TRACE(handle,qName,rName,description)
#endif

NSPR_API(PRTraceHandle)
	PR_CreateTrace( 
    	const char *qName,          
	    const char *rName,          
	    const char *description     
);





















#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_DESTROY_TRACE(handle)\
    PR_DestroyTrace((handle))
#else
#define PR_DESTROY_TRACE(handle)
#endif

NSPR_API(void) 
	PR_DestroyTrace( 
		PRTraceHandle handle    
);






























#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_TRACE(handle,ud0,ud1,ud2,ud3,ud4,ud5,ud6,ud7)\
    PR_Trace((handle),(ud0),(ud1),(ud2),(ud3),(ud4),(ud5),(ud6),(ud7))
#else
#define PR_TRACE(handle,ud0,ud1,ud2,ud3,ud4,ud5,ud6,ud7)
#endif

NSPR_API(void) 
	PR_Trace( 
    	PRTraceHandle handle,       
	    PRUint32    userData0,      
	    PRUint32    userData1,      
	    PRUint32    userData2,      
	    PRUint32    userData3,      
	    PRUint32    userData4,      
	    PRUint32    userData5,      
	    PRUint32    userData6,      
	    PRUint32    userData7       
);




































































#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_SET_TRACE_OPTION(command,value)\
    PR_SetTraceOption((command),(value))
#else
#define PR_SET_TRACE_OPTION(command,value)
#endif

NSPR_API(void) 
	PR_SetTraceOption( 
	    PRTraceOption command,  
	    void *value             
);


























#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_GET_TRACE_OPTION(command,value)\
    PR_GetTraceOption((command),(value))
#else
#define PR_GET_TRACE_OPTION(command,value)
#endif

NSPR_API(void) 
	PR_GetTraceOption( 
    	PRTraceOption command,  
	    void *value             
);
























#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_GET_TRACE_HANDLE_FROM_NAME(handle,qName,rName)\
    (handle) = PR_GetTraceHandleFromName((qName),(rName))
#else
#define PR_GET_TRACE_HANDLE_FROM_NAME(handle,qName,rName)
#endif

NSPR_API(PRTraceHandle) 
	PR_GetTraceHandleFromName( 
    	const char *qName,      
        const char *rName       
);




















#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_GET_TRACE_NAME_FROM_HANDLE(handle,qName,rName,description)\
    PR_GetTraceNameFromHandle((handle),(qName),(rName),(description))
#else
#define PR_GET_TRACE_NAME_FROM_HANDLE(handle,qName,rName,description)
#endif

NSPR_API(void) 
	PR_GetTraceNameFromHandle( 
    	PRTraceHandle handle,       
	    const char **qName,         
	    const char **rName,         
    	const char **description    
);




























#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_FIND_NEXT_TRACE_QNAME(next,handle)\
    (next) = PR_FindNextTraceQname((handle))
#else
#define PR_FIND_NEXT_TRACE_QNAME(next,handle)
#endif

NSPR_API(PRTraceHandle) 
	PR_FindNextTraceQname( 
        PRTraceHandle handle
);































#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_FIND_NEXT_TRACE_RNAME(next,rhandle,qhandle)\
    (next) = PR_FindNextTraceRname((rhandle),(qhandle))
#else
#define PR_FIND_NEXT_TRACE_RNAME(next,rhandle,qhandle)
#endif

NSPR_API(PRTraceHandle) 
	PR_FindNextTraceRname( 
        PRTraceHandle rhandle,
        PRTraceHandle qhandle
);












































#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_RECORD_TRACE_ENTRIES()\
	PR_RecordTraceEntries()
#else
#define PR_RECORD_TRACE_ENTRIES()
#endif
    
NSPR_API(void)
	PR_RecordTraceEntries(
        void 
);









































#if defined (DEBUG) || defined (FORCE_NSPR_TRACE)
#define PR_GET_TRACE_ENTRIES(buffer,count,found)\
        PR_GetTraceEntries((buffer),(count),(found))
#else
#define PR_GET_TRACE_ENTRIES(buffer,count,found)
#endif

NSPR_API(PRIntn)
    PR_GetTraceEntries(
        PRTraceEntry    *buffer,    
        PRInt32         count,      
        PRInt32         *found      
);

PR_END_EXTERN_C

#endif 

