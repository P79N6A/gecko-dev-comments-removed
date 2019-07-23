










































#ifdef GC_LEAK_DETECTOR


#include <stdio.h>


#include "generic_threads.h"
#include "prthread.h"
#include "prlock.h"


#ifdef XP_MAC
#include "pprthred.h"
#else
#include "private/pprthred.h"
#endif

#include "nsLeakDetector.h"

extern FILE *GC_stdout, *GC_stderr;

extern void GC_gcollect(void);
extern void GC_clear_roots(void);

static PRStatus PR_CALLBACK scanner(PRThread* t, void** baseAddr, PRUword count, void* closure)
{
	char* begin = (char*)baseAddr;
	char* end = (char*)(baseAddr + count);
	GC_mark_range_proc marker = (GC_mark_range_proc) closure;
	marker(begin, end);
	return PR_SUCCESS;
}

static void mark_all_stacks(GC_mark_range_proc marker)
{
	
	PR_ScanStackPointers(&scanner, (void *)marker);
}

static void locker(void* mutex)
{
	PR_Lock(mutex);
}

static void unlocker(void* mutex)
{
	PR_Unlock(mutex);
}

static void stopper(void* unused)
{
	PR_SuspendAll();
}

static void starter(void* unused)
{
	PR_ResumeAll();
}

nsresult NS_InitGarbageCollector()
{
	PRLock* mutex;
	
	
	GC_stderr = fopen("StartupLeaks", "w");

	mutex = PR_NewLock();
	if (mutex == NULL)
		return NS_ERROR_FAILURE;
		
	GC_generic_init_threads(&mark_all_stacks, mutex,
						 	&locker, &unlocker,
							&stopper, &starter);
	
	return NS_OK;
}

nsresult NS_ShutdownGarbageCollector()
{
	
	return NS_OK;
}

#endif 
