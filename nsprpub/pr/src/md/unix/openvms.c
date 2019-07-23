




































#include "primpl.h"

void _MD_EarlyInit(void)
{
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
#ifndef _PR_PTHREADS
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
#else
	*np = 0;
	return NULL;
#endif
}

#ifndef _PR_PTHREADS
void
_MD_SET_PRIORITY(_MDThread *thread, PRUintn newPri)
{
    return;
}

PRStatus
_MD_InitializeThread(PRThread *thread)
{
	return PR_SUCCESS;
}

PRStatus
_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
    PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    _PR_MD_SWITCH_CONTEXT(thread);
    return PR_SUCCESS;
}

PRStatus
_MD_WAKEUP_WAITER(PRThread *thread)
{
    if (thread) {
	PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    }
    return PR_SUCCESS;
}


void
_MD_YIELD(void)
{
    PR_NOT_REACHED("_MD_YIELD should not be called for OSF1.");
}

PRStatus
_MD_CREATE_THREAD(
    PRThread *thread,
    void (*start) (void *),
    PRThreadPriority priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize)
{
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for OSF1.");
	return PR_FAILURE;
}
#endif 

#ifdef _PR_HAVE_ATOMIC_CAS

#include <c_asm.h>

#define _PR_OSF_ATOMIC_LOCK 1

void 
PR_StackPush(PRStack *stack, PRStackElem *stack_elem)
{
long locked;

	do {
		while ((long) stack->prstk_head.prstk_elem_next ==
							_PR_OSF_ATOMIC_LOCK)
			;
		locked = __ATOMIC_EXCH_QUAD(&stack->prstk_head.prstk_elem_next,
								_PR_OSF_ATOMIC_LOCK);	

	} while (locked == _PR_OSF_ATOMIC_LOCK);
	stack_elem->prstk_elem_next = (PRStackElem *) locked;
	


	asm("mb");
	stack->prstk_head.prstk_elem_next = stack_elem;
}

PRStackElem * 
PR_StackPop(PRStack *stack)
{
PRStackElem *element;
long locked;

	do {
		while ((long)stack->prstk_head.prstk_elem_next == _PR_OSF_ATOMIC_LOCK)
			;
		locked = __ATOMIC_EXCH_QUAD(&stack->prstk_head.prstk_elem_next,
								_PR_OSF_ATOMIC_LOCK);	

	} while (locked == _PR_OSF_ATOMIC_LOCK);

	element = (PRStackElem *) locked;

	if (element == NULL) {
		stack->prstk_head.prstk_elem_next = NULL;
	} else {
		stack->prstk_head.prstk_elem_next =
			element->prstk_elem_next;
	}
	


	asm("mb");
	return element;
}
#endif 









int thread_suspend(PRThread *thr_id) {

    extern int pthread_suspend_np (
			pthread_t                       thread,
			__pthreadLongUint_t             *regs,
			void                            *spare);

    __pthreadLongUint_t regs[34];
    int res;

    




    res = pthread_suspend_np(thr_id->id,&regs[0],0);
    if (res==0)
	thr_id->sp = (void *) regs[30]; 

    thr_id->suspend |= PT_THREAD_SUSPENDED;

    
    return 0;
}

int thread_resume(PRThread *thr_id) {
    extern int pthread_resume_np(pthread_t thread);
    int res;

    res = pthread_resume_np (thr_id->id);
	
    thr_id->suspend |= PT_THREAD_RESUMED;

    return 0;
}

























void PR_VMS_Stub1(void) { }
void PR_VMS_Stub2(void) { }
void PR_VMS_Stub3(void) { }
void PR_VMS_Stub4(void) { }
void PR_VMS_Stub5(void) { }
void PR_VMS_Stub6(void) { }
void PR_VMS_Stub7(void) { }
void PR_VMS_Stub8(void) { }
void PR_VMS_Stub9(void) { }
void PR_VMS_Stub10(void) { }
void PR_VMS_Stub11(void) { }
void PR_VMS_Stub12(void) { }
void PR_VMS_Stub13(void) { }
void PR_VMS_Stub14(void) { }
void PR_VMS_Stub15(void) { }
void PR_VMS_Stub16(void) { }
void PR_VMS_Stub17(void) { }
void PR_VMS_Stub18(void) { }
void PR_VMS_Stub19(void) { }
void PR_VMS_Stub20(void) { }
void PR_VMS_Stub21(void) { }
void PR_VMS_Stub22(void) { }
void PR_VMS_Stub23(void) { }
void PR_VMS_Stub24(void) { }
void PR_VMS_Stub25(void) { }
void PR_VMS_Stub26(void) { }
void PR_VMS_Stub27(void) { }
void PR_VMS_Stub28(void) { }
void PR_VMS_Stub29(void) { }
void PR_VMS_Stub30(void) { }
void PR_VMS_Stub31(void) { }
void PR_VMS_Stub32(void) { }
void PR_VMS_Stub33(void) { }
void PR_VMS_Stub34(void) { }
void PR_VMS_Stub35(void) { }
void PR_VMS_Stub36(void) { }
void PR_VMS_Stub37(void) { }
void PR_VMS_Stub38(void) { }
void PR_VMS_Stub39(void) { }
void PR_VMS_Stub40(void) { }
void PR_VMS_Stub41(void) { }
void PR_VMS_Stub42(void) { }
void PR_VMS_Stub43(void) { }
void PR_VMS_Stub44(void) { }
void PR_VMS_Stub45(void) { }
void PR_VMS_Stub46(void) { }
void PR_VMS_Stub47(void) { }
void PR_VMS_Stub48(void) { }
void PR_VMS_Stub49(void) { }
void PR_VMS_Stub50(void) { }
void PR_VMS_Stub51(void) { }
void PR_VMS_Stub52(void) { }
void PR_VMS_Stub53(void) { }
