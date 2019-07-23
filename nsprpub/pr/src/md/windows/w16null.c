




































#include "primpl.h"
#include <sys/timeb.h>


struct _MDLock          _pr_ioq_lock;
HINSTANCE               _pr_hInstance = NULL;
char *                  _pr_top_of_task_stack;
_PRInterruptTable       _pr_interruptTable[] = { { 0 } };















#if defined(HAVE_WATCOM_BUG_2)
PRTime __pascal __export __loadds
#else
PR_IMPLEMENT(PRTime)
#endif
PR_Now(void)
{
    PRInt64 s, ms, ms2us, s2us;
    struct timeb b;

    ftime(&b);
    LL_I2L(ms2us, PR_USEC_PER_MSEC);
    LL_I2L(s2us, PR_USEC_PER_SEC);
    LL_I2L(s, b.time);
    LL_I2L(ms, (PRInt32)b.millitm);
    LL_MUL(ms, ms, ms2us);
    LL_MUL(s, s, s2us);
    LL_ADD(s, s, ms);
    return s;       
}



char *_PR_MD_GET_ENV(const char *name)
{
    return NULL;
}

PRIntn
_PR_MD_PUT_ENV(const char *name)
{
    return NULL;
}

int CALLBACK LibMain( HINSTANCE hInst, WORD wDataSeg, 
                      WORD cbHeapSize, LPSTR lpszCmdLine )
{
    _pr_hInstance = hInst;
    return TRUE;
}



void
_PR_MD_EARLY_INIT()
{
    _tzset();
    return;
}

void
_PR_MD_WAKEUP_CPUS( void )
{
    return;
}    

