









































#ifdef GC_LEAK_DETECTOR


#include <stdio.h>


#include "generic_threads.h"
#include "primpl.h"

extern FILE *GC_stdout, *GC_stderr;

extern void GC_gcollect(void);
extern void GC_clear_roots(void);

static PRStatus PR_CALLBACK scanner(PRThread* t, void** baseAddr,
                                    PRUword count, void* closure)
{
    if (count) {
        char* begin = (char*)baseAddr;
        char* end = (char*)(baseAddr + count);
        GC_mark_range_proc marker = (GC_mark_range_proc) closure;
        marker(begin, end);
    }
    return PR_SUCCESS;
}

static void mark_all_stacks(GC_mark_range_proc marker)
{
    PR_ScanStackPointers(&scanner, (void *)marker);
}

#if defined(_PR_PTHREADS)
#define _PR_MD_CURRENT_CPU() 1
#endif

static void locker(void* mutex)
{
    if (_PR_MD_CURRENT_CPU())
        PR_EnterMonitor(mutex);
}

static void unlocker(void* mutex)
{
    if (_PR_MD_CURRENT_CPU())
        PR_ExitMonitor(mutex);
}

static void stopper(void* unused)
{
    if (_PR_MD_CURRENT_CPU())
        PR_SuspendAll();
}

static void starter(void* unused)
{
    if (_PR_MD_CURRENT_CPU())
        PR_ResumeAll();
}

void _PR_InitGarbageCollector()
{
    void* mutex;

    
    GC_stderr = fopen("StartupLeaks", "w");

    mutex = PR_NewMonitor();
    PR_ASSERT(mutex != NULL);

    GC_generic_init_threads(&mark_all_stacks, mutex,
            &locker, &unlocker,
            &stopper, &starter);
}

void _PR_ShutdownGarbageCollector()
{
    
}

#endif 
