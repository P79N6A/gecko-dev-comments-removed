




































#include "primpl.h"
#include <sys/timeb.h>
#include <stdio.h>














typedef struct DispatchTrace
{
    PRThread *          thread;
    PRUint32            state;
    PRInt16             mdThreadNumber;
    PRInt16             unused;
    PRThreadPriority    priority;
    
} DispatchTrace, *DispatchTracePtr ;

static void TraceDispatch( PRThread *thread );


PRThread                *_pr_primordialThread;






static char * pSource;          
static char * pTarget;          
static int   cxByteCount;       
static int   bytesMoved;        
static FILE *    file1 = 0;     

#define NUM_DISPATCHTRACE_OBJECTS  24
static DispatchTrace dt[NUM_DISPATCHTRACE_OBJECTS] = {0}; 
static PRUint32 dispatchCount = 0;  

static int OldPriorityOfPrimaryThread   = -1;
static int TimeSlicesOnNonPrimaryThread =  0;
static PRUint32 threadNumber = 1;   




















void
_PR_MD_FINAL_INIT()
{
    PRThreadStack *     stack = 0;
    PRInt32             stacksize = 0;
    PRThread *          me = _PR_MD_CURRENT_THREAD();
    
    _PR_ADJUST_STACKSIZE( stacksize );
    stack = _PR_NewStack( stacksize );
    
    me->stack = stack;
    stack->thr = me;
    
    return;
} 


void
_MD_INIT_RUNNING_CPU( struct _PRCPU *cpu )
{
	PR_INIT_CLIST(&(cpu->md.ioQ));
	cpu->md.ioq_max_osfd = -1;
	cpu->md.ioq_timeout = PR_INTERVAL_NO_TIMEOUT;
}    


void
_PR_MD_YIELD( void )
{
    PR_ASSERT(0);
}







void
_PR_MD_INIT_STACK( PRThreadStack *ts, PRIntn redzone )
{
    ts->md.stackTop = ts->stackTop - sizeof(PRThread);
    ts->md.cxByteCount = 0;
    
    return;
} 





PRStatus
_PR_MD_INIT_THREAD(PRThread *thread)
{
    if ( thread->flags & _PR_PRIMORDIAL)
    {
        _pr_primordialThread = thread;
        thread->md.threadNumber = 1;
    }
    else
    {
        thread->md.threadNumber = ++threadNumber;
    }

    thread->md.magic = _MD_MAGIC_THREAD;
    strcpy( thread->md.guardBand, "GuardBand" );
    
    return PR_SUCCESS;
}


PRStatus
_PR_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
    _MD_SWITCH_CONTEXT( thread );
    
    return( PR_SUCCESS );
}

void *PR_W16GetExceptionContext(void)
{
    return _MD_CURRENT_THREAD()->md.exceptionContext;
}

void
PR_W16SetExceptionContext(void *context)
{
    _MD_CURRENT_THREAD()->md.exceptionContext = context;
}














































































































void _MD_RESTORE_CONTEXT(PRThread *t)
{
    dispatchCount++;
    TraceDispatch( t );
    






	

    if (_pr_primordialThread == t) {
        if (OldPriorityOfPrimaryThread != -1) {
            PR_SetThreadPriority(_pr_primordialThread, OldPriorityOfPrimaryThread);
            OldPriorityOfPrimaryThread = -1;
        }
        TimeSlicesOnNonPrimaryThread = 0;
    } else {
        TimeSlicesOnNonPrimaryThread++;
    }

    if ((TimeSlicesOnNonPrimaryThread >= 20) && (OldPriorityOfPrimaryThread == -1)) {
        OldPriorityOfPrimaryThread = PR_GetThreadPriority(_pr_primordialThread);
        PR_SetThreadPriority(_pr_primordialThread, 31);
        TimeSlicesOnNonPrimaryThread = 0;
    }

    


    cxByteCount  = (int) ((PRUint32) _pr_top_of_task_stack - (PRUint32) &t );
    pSource      = (char *) &t;
    pTarget      = (char *)((PRUint32)_pr_currentThread->stack->md.stackTop 
                            - (PRUint32)cxByteCount );
    _pr_currentThread->stack->md.cxByteCount = cxByteCount;
    
    for( bytesMoved = 0; bytesMoved < cxByteCount; bytesMoved++ )
        *(pTarget + bytesMoved ) = *(pSource + bytesMoved );
    
    
    _pr_currentThread = t;

    






    cxByteCount  = t->stack->md.cxByteCount;
    pSource      = t->stack->md.stackTop - cxByteCount;
    pTarget      = _pr_top_of_task_stack - cxByteCount;
    
    errno = (_pr_currentThread)->md.errcode;
    
    __asm 
    {
        mov cx, cxByteCount
        mov si, WORD PTR [pSource]
        mov di, WORD PTR [pTarget]
        mov ax, WORD PTR [pTarget + 2]
        mov es, ax
        mov ax, WORD PTR [pSource + 2]
        mov bx, ds
        mov ds, ax
        rep movsb
        mov ds, bx
    }

    








    __asm {
        mov     ax, WORD PTR [_pr_top_of_task_stack]
        sub     ax, cxByteCount
        mov     sp, ax
    };

    



    Throw((_pr_currentThread)->md.context, 1);
} 


static void TraceDispatch( PRThread *thread )
{
    int i;
    
    



    for( i = NUM_DISPATCHTRACE_OBJECTS -2; i >= 0; i-- )
    {
        dt[i +1] = dt[i];
    }
    
    


    dt->thread = thread;
    dt->state = thread->state;
    dt->mdThreadNumber = thread->md.threadNumber;
    dt->priority = thread->priority;
    
    return;
} 



