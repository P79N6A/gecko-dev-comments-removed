














































#include "seccomon.h"
#include "prmem.h"
#include "prerror.h"
#include "plarena.h"
#include "secerr.h"
#include "prmon.h"
#include "prbit.h"
#include "ck.h"

#ifdef notdef
unsigned long port_allocFailures;





PORTCharConversionFunc ucs4Utf8ConvertFunc;
PORTCharConversionFunc ucs2Utf8ConvertFunc;
PORTCharConversionWSwapFunc  ucs2AsciiConvertFunc;

void *
PORT_Alloc(size_t bytes)
{
    void *rv;

    
    rv = (void *)malloc(bytes ? bytes : 1);
    if (!rv) {
	++port_allocFailures;
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

void *
PORT_Realloc(void *oldptr, size_t bytes)
{
    void *rv;

    rv = (void *)realloc(oldptr, bytes);
    if (!rv) {
	++port_allocFailures;
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

void *
PORT_ZAlloc(size_t bytes)
{
    void *rv;

    
    rv = (void *)calloc(1, bytes ? bytes : 1);
    if (!rv) {
	++port_allocFailures;
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }
    return rv;
}

void
PORT_Free(void *ptr)
{
    if (ptr) {
	free(ptr);
    }
}

void
PORT_ZFree(void *ptr, size_t len)
{
    if (ptr) {
	memset(ptr, 0, len);
	free(ptr);
    }
}




PLArenaPool *
PORT_NewArena(unsigned long chunksize)
{
    PLArenaPool *arena;
    
    arena = (PLArenaPool*)PORT_ZAlloc(sizeof(PLArenaPool));
    if ( arena != NULL ) {
	PR_InitArenaPool(arena, "security", chunksize, sizeof(double));
    }
    return(arena);
}

void *
PORT_ArenaAlloc(PLArenaPool *arena, size_t size)
{
    void *p;

    PL_ARENA_ALLOCATE(p, arena, size);
    if (p == NULL) {
	++port_allocFailures;
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    return(p);
}

void *
PORT_ArenaZAlloc(PLArenaPool *arena, size_t size)
{
    void *p;

    PL_ARENA_ALLOCATE(p, arena, size);
    if (p == NULL) {
	++port_allocFailures;
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    } else {
	PORT_Memset(p, 0, size);
    }

    return(p);
}


void
PORT_FreeArena(PLArenaPool *arena, PRBool zero)
{
    PR_FinishArenaPool(arena);
    PORT_Free(arena);
}

void *
PORT_ArenaGrow(PLArenaPool *arena, void *ptr, size_t oldsize, size_t newsize)
{
    PORT_Assert(newsize >= oldsize);
    
    PL_ARENA_GROW(ptr, arena, oldsize, ( newsize - oldsize ) );
    
    if (ptr == NULL) { 
        ++port_allocFailures;
        PORT_SetError(SEC_ERROR_NO_MEMORY);
    }
    return(ptr);
}

void *
PORT_ArenaMark(PLArenaPool *arena)
{
    void * result;

    result = PL_ARENA_MARK(arena);
    return result;
}

void
PORT_ArenaRelease(PLArenaPool *arena, void *mark)
{
    PL_ARENA_RELEASE(arena, mark);
}

void
PORT_ArenaUnmark(PLArenaPool *arena, void *mark)
{
    
}

char *
PORT_ArenaStrdup(PLArenaPool *arena,const char *str) {
    int len = PORT_Strlen(str)+1;
    char *newstr;

    newstr = (char*)PORT_ArenaAlloc(arena,len);
    if (newstr) {
        PORT_Memcpy(newstr,str,len);
    }
    return newstr;
}
#endif





static PRInt32 stack[2] = {0, 0};

PR_IMPLEMENT(void)
nss_SetError(PRUint32 value)
{	
    stack[0] = value;
    return;
}

PR_IMPLEMENT(PRInt32)
NSS_GetError(void)
{
    return(stack[0]);
}


PR_IMPLEMENT(PRInt32 *)
NSS_GetErrorStack(void)
{
    return(&stack[0]);
}

PR_IMPLEMENT(void)
nss_ClearErrorStack(void)
{
    stack[0] = 0;
    return;
}

#ifdef DEBUG





PR_IMPLEMENT(PRStatus)
nssPointerTracker_initialize(nssPointerTracker *tracker)
{
    return PR_SUCCESS;
}


PR_IMPLEMENT(PRStatus)
nssPointerTracker_finalize(nssPointerTracker *tracker)
{
    return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus)
nssPointerTracker_add(nssPointerTracker *tracker, const void *pointer)
{
     return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus)
nssPointerTracker_remove(nssPointerTracker *tracker, const void *pointer)
{
     return PR_SUCCESS;
}

PR_IMPLEMENT(PRStatus)
nssPointerTracker_verify(nssPointerTracker *tracker, const void *pointer)
{
     return PR_SUCCESS;
}
#endif






#if !(defined(WIN32) && defined(__GNUC__))
PR_IMPLEMENT(PRThread *)
PR_GetCurrentThread(void)
{
     return (PRThread *)1;
}



PR_IMPLEMENT(void)
PR_Assert(const char *expr, const char *file, int line) {
    return; 
}

PR_IMPLEMENT(void *)
PR_Alloc(PRUint32 bytes) { return malloc(bytes); }

PR_IMPLEMENT(void *)
PR_Malloc(PRUint32 bytes) { return malloc(bytes); }

PR_IMPLEMENT(void *)
PR_Calloc(PRUint32 blocks, PRUint32 bytes) { return calloc(blocks,bytes); }

PR_IMPLEMENT(void *)
PR_Realloc(void * blocks, PRUint32 bytes) { return realloc(blocks,bytes); }

PR_IMPLEMENT(void)
PR_Free(void *ptr) { free(ptr); }

#ifdef notdef

#include "secasn1.h"
#include "secoid.h"

const SEC_ASN1Template SECOID_AlgorithmIDTemplate[] = {
    { SEC_ASN1_SEQUENCE,
	  0, NULL, sizeof(SECAlgorithmID) },
    { SEC_ASN1_OBJECT_ID,
	  offsetof(SECAlgorithmID,algorithm), },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_ANY,
	  offsetof(SECAlgorithmID,parameters), },
    { 0, }
};

PR_IMPLEMENT(PRStatus) PR_Sleep(PRIntervalTime ticks) { return PR_SUCCESS; }


PR_IMPLEMENT(PRInt32) PR_AtomicDecrement(PRInt32 *val) { return --(*val); }

PR_IMPLEMENT(PRInt32) PR_AtomicSet(PRInt32 *val) { return ++(*val); }

#endif

 
PR_IMPLEMENT(PRInt32) PR_AtomicIncrement(PRInt32 *val) { return ++(*val); }
#endif 

static CK_C_INITIALIZE_ARGS_PTR nssstub_pInitArgs = NULL;
static CK_C_INITIALIZE_ARGS nssstub_initArgs;
static NSSArena *nssstub_arena = NULL;
static CryptokiLockingState nssstub_LockingState = SingleThreaded;

PR_IMPLEMENT(CK_RV)
nssSetLockArgs(CK_C_INITIALIZE_ARGS_PTR pInitArgs, CryptokiLockingState* returned)
{
    CK_ULONG count = (CK_ULONG)0;
    CK_BBOOL os_ok = CK_FALSE;
    CK_RV rv = CKR_OK;
    if (nssstub_pInitArgs == NULL) {
	if (pInitArgs != NULL) {
	    nssstub_initArgs = *pInitArgs;
	    nssstub_pInitArgs = &nssstub_initArgs;
            if( (CK_CREATEMUTEX )NULL != pInitArgs->CreateMutex  ) count++;
            if( (CK_DESTROYMUTEX)NULL != pInitArgs->DestroyMutex ) count++;
            if( (CK_LOCKMUTEX   )NULL != pInitArgs->LockMutex    ) count++;
            if( (CK_UNLOCKMUTEX )NULL != pInitArgs->UnlockMutex  ) count++;
            os_ok = (pInitArgs->flags & CKF_OS_LOCKING_OK) ? CK_TRUE : CK_FALSE;

            if( (0 != count) && (4 != count) ) {
                rv = CKR_ARGUMENTS_BAD;
                goto loser;
            }
	} else {
	    nssstub_pInitArgs = pInitArgs;
	}
	
    }

    if( (0 == count) && (CK_TRUE == os_ok) ) {
      












      rv = CKR_CANT_LOCK;
      goto loser;
    }

    if( 0 == count ) {
      





      nssstub_LockingState = SingleThreaded;
    } else {
      





      nssstub_LockingState = MultiThreaded;
    }

    loser:
    *returned = nssstub_LockingState;
    return rv;
}






#if !(defined(WIN32) && defined(__GNUC__))
#include "prlock.h"
PR_IMPLEMENT(PRLock *)
PR_NewLock(void) {
	PRLock *lock = NULL;
	NSSCKFWMutex *mlock = NULL;
	CK_RV error;

	mlock = nssCKFWMutex_Create(nssstub_pInitArgs,nssstub_LockingState,nssstub_arena,&error);
	lock = (PRLock *)mlock;

	
	if (lock == NULL) lock=(PRLock *) 1;
	return lock;
}

PR_IMPLEMENT(void) 
PR_DestroyLock(PRLock *lock) {
	NSSCKFWMutex *mlock = (NSSCKFWMutex *)lock;
	if (lock == (PRLock *)1) return;
	nssCKFWMutex_Destroy(mlock);
}

PR_IMPLEMENT(void) 
PR_Lock(PRLock *lock) {
	NSSCKFWMutex *mlock = (NSSCKFWMutex *)lock;
	if (lock == (PRLock *)1) return;
	nssCKFWMutex_Lock(mlock);
}

PR_IMPLEMENT(PRStatus) 
PR_Unlock(PRLock *lock) {
	NSSCKFWMutex *mlock = (NSSCKFWMutex *)lock;
	if (lock == (PRLock *)1) return PR_SUCCESS;
	nssCKFWMutex_Unlock(mlock);
	return PR_SUCCESS;
}

#ifdef notdef
#endif





PR_IMPLEMENT(PRMonitor*) 
PR_NewMonitor(void)
{
    return (PRMonitor *) PR_NewLock();
}


PR_IMPLEMENT(void) 
PR_EnterMonitor(PRMonitor *mon)
{
    PR_Lock( (PRLock *)mon );
}

PR_IMPLEMENT(PRStatus) 
PR_ExitMonitor(PRMonitor *mon)
{
    return PR_Unlock( (PRLock *)mon );
}

#include "prinit.h"



PR_IMPLEMENT(PRStatus) PR_CallOnce(
    PRCallOnceType *once,
    PRCallOnceFN    func)
{
    
    if (1 == PR_AtomicIncrement(&once->initialized)) {
	once->status = (*func)();
    }  else {
    	
    }
    return once->status;
}




PRIntn PR_CeilingLog2(PRUint32 i) {
	PRIntn log2;
	PR_CEILING_LOG2(log2,i);
	return log2;
}
#endif 



