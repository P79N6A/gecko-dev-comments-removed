




































#include "prlog.h"
#include "prthread.h"
#include "private/pprthred.h"
#include "primpl.h"

PR_IMPLEMENT(PRWord *)
PR_GetGCRegisters(PRThread *t, int isCurrent, int *np)
{
    return _MD_HomeGCRegisters(t, isCurrent, np);
}

PR_IMPLEMENT(PRStatus)
PR_ThreadScanStackPointers(PRThread* t,
                           PRScanStackFun scanFun, void* scanClosure)
{
    PRThread* current = PR_GetCurrentThread();
    PRWord *sp, *esp, *p0;
    int n;
    void **ptd;
    PRStatus status;
    PRUint32 index;
    int stack_end;

    



    p0 = _MD_HomeGCRegisters(t, t == current, &n);
    status = scanFun(t, (void**)p0, n, scanClosure);
    if (status != PR_SUCCESS)
        return status;

    
#if defined(XP_PC) && defined(WIN16)
    





    if (t == current) {
        sp  = (PRWord*) &stack_end;
        esp = (PRWord*) _pr_top_of_task_stack;

        PR_ASSERT(sp <= esp);
    } else {
        sp  = (PRWord*) PR_GetSP(t);
        esp = (PRWord*) t->stack->stackTop;

        PR_ASSERT((t->stack->stackSize == 0) ||
                  ((sp >  (PRWord*)t->stack->stackBottom) &&
                   (sp <= (PRWord*)t->stack->stackTop)));
    }
#else   
#ifdef HAVE_STACK_GROWING_UP
    if (t == current) {
        esp = (PRWord*) &stack_end;
    } else {
        esp = (PRWord*) PR_GetSP(t);
    }
    sp = (PRWord*) t->stack->stackTop;
    if (t->stack->stackSize) {
        PR_ASSERT((esp > (PRWord*)t->stack->stackTop) &&
                  (esp < (PRWord*)t->stack->stackBottom));
    }
#else   
    if (t == current) {
        sp = (PRWord*) &stack_end;
    } else {
        sp = (PRWord*) PR_GetSP(t);
    }
    esp = (PRWord*) t->stack->stackTop;
    if (t->stack->stackSize) {
        PR_ASSERT((sp > (PRWord*)t->stack->stackBottom) &&
                  (sp < (PRWord*)t->stack->stackTop));
    }
#endif  
#endif  

#if defined(WIN16)
    {
        prword_t scan;
        prword_t limit;
        
        scan = (prword_t) sp;
        limit = (prword_t) esp;
        while (scan < limit) {
            prword_t *test;

            test = *((prword_t **)scan);
            status = scanFun(t, (void**)&test, 1, scanClosure);
            if (status != PR_SUCCESS)
                return status;
            scan += sizeof(char);
        }
    }
#else
    if (sp < esp) {
        status = scanFun(t, (void**)sp, esp - sp, scanClosure);
        if (status != PR_SUCCESS)
            return status;
    }
#endif

    





    status = scanFun(t, (void**)&t->environment, 1, scanClosure);
    if (status != PR_SUCCESS)
        return status;

#ifndef GC_LEAK_DETECTOR
    
    ptd = t->privateData;
    for (index = 0; index < t->tpdLength; index++, ptd++) {
        status = scanFun(t, (void**)ptd, 1, scanClosure);
        if (status != PR_SUCCESS)
            return status;
    }
#endif
    
    return PR_SUCCESS;
}


typedef struct PRScanStackData {
    PRScanStackFun      scanFun;
    void*               scanClosure;
} PRScanStackData;

static PRStatus PR_CALLBACK
pr_ScanStack(PRThread* t, int i, void* arg)
{
    PRScanStackData* data = (PRScanStackData*)arg;
    return PR_ThreadScanStackPointers(t, data->scanFun, data->scanClosure);
}

PR_IMPLEMENT(PRStatus)
PR_ScanStackPointers(PRScanStackFun scanFun, void* scanClosure)
{
    PRScanStackData data;
    data.scanFun = scanFun;
    data.scanClosure = scanClosure;
    return PR_EnumerateThreads(pr_ScanStack, &data);
}

PR_IMPLEMENT(PRUword)
PR_GetStackSpaceLeft(PRThread* t)
{
    PRThread *current = PR_GetCurrentThread();
    PRWord *sp, *esp;
    int stack_end;

#if defined(WIN16)
    





    if (t == current) {
        sp  = (PRWord*) &stack_end;
        esp = (PRWord*) _pr_top_of_task_stack;

        PR_ASSERT(sp <= esp);
    } else {
        sp  = (PRWord*) PR_GetSP(t);
        esp = (PRWord*) t->stack->stackTop;

	PR_ASSERT((t->stack->stackSize == 0) ||
                 ((sp >  (PRWord*)t->stack->stackBottom) &&
		  (sp <= (PRWord*)t->stack->stackTop)));
    }
#else   
#ifdef HAVE_STACK_GROWING_UP
    if (t == current) {
        esp = (PRWord*) &stack_end;
    } else {
        esp = (PRWord*) PR_GetSP(t);
    }
    sp = (PRWord*) t->stack->stackTop;
    if (t->stack->stackSize) {
        PR_ASSERT((esp > (PRWord*)t->stack->stackTop) &&
                  (esp < (PRWord*)t->stack->stackBottom));
    }
#else   
    if (t == current) {
        sp = (PRWord*) &stack_end;
    } else {
        sp = (PRWord*) PR_GetSP(t);
    }
    esp = (PRWord*) t->stack->stackTop;
    if (t->stack->stackSize) {
	PR_ASSERT((sp > (PRWord*)t->stack->stackBottom) &&
		  (sp < (PRWord*)t->stack->stackTop));
    }
#endif  
#endif  
    return (PRUword)t->stack->stackSize - ((PRWord)esp - (PRWord)sp);
}
