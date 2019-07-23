













































#include "seccomon.h"
#include "prmem.h"
#include "prerror.h"
#include "plarena.h"
#include "secerr.h"
#include "prmon.h"
#include "nssilock.h"
#include "secport.h"
#include "prvrsion.h"
#include "prenv.h"

#ifdef DEBUG
#define THREADMARK
#endif 

#ifdef THREADMARK
#include "prthread.h"
#endif 

#if defined(XP_UNIX) || defined(XP_OS2) || defined(XP_BEOS)
#include <stdlib.h>
#else
#include "wtypes.h"
#endif

#define SET_ERROR_CODE

#ifdef THREADMARK
typedef struct threadmark_mark_str {
  struct threadmark_mark_str *next;
  void *mark;
} threadmark_mark;

#endif 


#define ARENAPOOL_MAGIC 0xB8AC9BDF 

typedef struct PORTArenaPool_str {
  PLArenaPool arena;
  PRUint32    magic;
  PRLock *    lock;
#ifdef THREADMARK
  PRThread *marking_thread;
  threadmark_mark *first_mark;
#endif
} PORTArenaPool;



unsigned long port_allocFailures;





PORTCharConversionFunc ucs4Utf8ConvertFunc;
PORTCharConversionFunc ucs2Utf8ConvertFunc;
PORTCharConversionWSwapFunc  ucs2AsciiConvertFunc;

void *
PORT_Alloc(size_t bytes)
{
    void *rv;

    
    rv = (void *)PR_Malloc(bytes ? bytes : 1);
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

    rv = (void *)PR_Realloc(oldptr, bytes);
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

    
    rv = (void *)PR_Calloc(1, bytes ? bytes : 1);
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
	PR_Free(ptr);
    }
}

void
PORT_ZFree(void *ptr, size_t len)
{
    if (ptr) {
	memset(ptr, 0, len);
	PR_Free(ptr);
    }
}

char *
PORT_Strdup(const char *str)
{
    size_t len = PORT_Strlen(str)+1;
    char *newstr;

    newstr = (char *)PORT_Alloc(len);
    if (newstr) {
        PORT_Memcpy(newstr, str, len);
    }
    return newstr;
}

void
PORT_SetError(int value)
{	
#ifdef DEBUG_jp96085
    PORT_Assert(value != SEC_ERROR_REUSED_ISSUER_AND_SERIAL);
#endif
    PR_SetError(value, 0);
    return;
}

int
PORT_GetError(void)
{
    return(PR_GetError());
}



PLArenaPool *
PORT_NewArena(unsigned long chunksize)
{
    PORTArenaPool *pool;
    
    pool = PORT_ZNew(PORTArenaPool);
    if (!pool) {
	return NULL;
    }
    pool->magic = ARENAPOOL_MAGIC;
    pool->lock = PZ_NewLock(nssILockArena);
    if (!pool->lock) {
	++port_allocFailures;
	PORT_Free(pool);
	return NULL;
    }
    PL_InitArenaPool(&pool->arena, "security", chunksize, sizeof(double));
    return(&pool->arena);
}

#define MAX_SIZE 0x7fffffffUL

void *
PORT_ArenaAlloc(PLArenaPool *arena, size_t size)
{
    void *p = NULL;

    PORTArenaPool *pool = (PORTArenaPool *)arena;

    if (size <= 0) {
	size = 1;
    }

    if (size > MAX_SIZE) {
	
    } else 
    
    if (ARENAPOOL_MAGIC == pool->magic ) {
	PZ_Lock(pool->lock);
#ifdef THREADMARK
        
	if (pool->marking_thread  &&
	    pool->marking_thread != PR_GetCurrentThread() ) {
	    
	    PZ_Unlock(pool->lock);
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    PORT_Assert(0);
	    return NULL;
	} 
#endif 
	PL_ARENA_ALLOCATE(p, arena, size);
	PZ_Unlock(pool->lock);
    } else {
	PL_ARENA_ALLOCATE(p, arena, size);
    }

    if (!p) {
	++port_allocFailures;
	PORT_SetError(SEC_ERROR_NO_MEMORY);
    }

    return(p);
}

void *
PORT_ArenaZAlloc(PLArenaPool *arena, size_t size)
{
    void *p;

    if (size <= 0)
        size = 1;

    p = PORT_ArenaAlloc(arena, size);

    if (p) {
	PORT_Memset(p, 0, size);
    }

    return(p);
}




void
PORT_FreeArena(PLArenaPool *arena, PRBool zero)
{
    PORTArenaPool *pool = (PORTArenaPool *)arena;
    PRLock *       lock = (PRLock *)0;
    size_t         len  = sizeof *arena;
    extern const PRVersionDescription * libVersionPoint(void);
    static const PRVersionDescription * pvd;
    static PRBool  doFreeArenaPool = PR_FALSE;

    if (!pool)
    	return;
    if (ARENAPOOL_MAGIC == pool->magic ) {
	len  = sizeof *pool;
	lock = pool->lock;
	PZ_Lock(lock);
    }
    if (!pvd) {
	





	
	pvd = libVersionPoint();
	if ((pvd->vMajor > 4) || 
	    (pvd->vMajor == 4 && pvd->vMinor > 1) ||
	    (pvd->vMajor == 4 && pvd->vMinor == 1 && pvd->vPatch >= 1)) {
	    const char *ev = PR_GetEnv("NSS_DISABLE_ARENA_FREE_LIST");
	    if (!ev) doFreeArenaPool = PR_TRUE;
	}
    }
    if (zero) {
	PLArena *a;
	for (a = arena->first.next; a; a = a->next) {
	    PR_ASSERT(a->base <= a->avail && a->avail <= a->limit);
	    memset((void *)a->base, 0, a->avail - a->base);
	}
    }
    if (doFreeArenaPool) {
	PL_FreeArenaPool(arena);
    } else {
	PL_FinishArenaPool(arena);
    }
    PORT_ZFree(arena, len);
    if (lock) {
	PZ_Unlock(lock);
	PZ_DestroyLock(lock);
    }
}

void *
PORT_ArenaGrow(PLArenaPool *arena, void *ptr, size_t oldsize, size_t newsize)
{
    PORTArenaPool *pool = (PORTArenaPool *)arena;
    PORT_Assert(newsize >= oldsize);
    
    if (ARENAPOOL_MAGIC == pool->magic ) {
	PZ_Lock(pool->lock);
	
	PL_ARENA_GROW(ptr, arena, oldsize, ( newsize - oldsize ) );
	PZ_Unlock(pool->lock);
    } else {
	PL_ARENA_GROW(ptr, arena, oldsize, ( newsize - oldsize ) );
    }
    
    return(ptr);
}

void *
PORT_ArenaMark(PLArenaPool *arena)
{
    void * result;

    PORTArenaPool *pool = (PORTArenaPool *)arena;
    if (ARENAPOOL_MAGIC == pool->magic ) {
	PZ_Lock(pool->lock);
#ifdef THREADMARK
	{
	  threadmark_mark *tm, **pw;
	  PRThread * currentThread = PR_GetCurrentThread();

	    if (! pool->marking_thread ) {
		
		pool->marking_thread = currentThread;
	    } else if (currentThread != pool->marking_thread ) {
		PZ_Unlock(pool->lock);
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		PORT_Assert(0);
		return NULL;
	    }

	    result = PL_ARENA_MARK(arena);
	    PL_ARENA_ALLOCATE(tm, arena, sizeof(threadmark_mark));
	    if (!tm) {
		PZ_Unlock(pool->lock);
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		return NULL;
	    }

	    tm->mark = result;
	    tm->next = (threadmark_mark *)NULL;

	    pw = &pool->first_mark;
	    while( *pw ) {
		 pw = &(*pw)->next;
	    }

	    *pw = tm;
	}
#else 
	result = PL_ARENA_MARK(arena);
#endif 
	PZ_Unlock(pool->lock);
    } else {
	
	result = PL_ARENA_MARK(arena);
    }
    return result;
}

static void
port_ArenaZeroAfterMark(PLArenaPool *arena, void *mark)
{
    PLArena *a = arena->current;
    if (a->base <= (PRUword)mark && (PRUword)mark <= a->avail) {
	
	memset(mark, 0, a->avail - (PRUword)mark);
    } else {
	
	for (a = arena->first.next; a; a = a->next) {
	    PR_ASSERT(a->base <= a->avail && a->avail <= a->limit);
	    if (a->base <= (PRUword)mark && (PRUword)mark <= a->avail) {
		memset(mark, 0, a->avail - (PRUword)mark);
		a = a->next;
		break;
	    }
	}
	for (; a; a = a->next) {
	    PR_ASSERT(a->base <= a->avail && a->avail <= a->limit);
	    memset((void *)a->base, 0, a->avail - a->base);
	}
    }
}

static void
port_ArenaRelease(PLArenaPool *arena, void *mark, PRBool zero)
{
    PORTArenaPool *pool = (PORTArenaPool *)arena;
    if (ARENAPOOL_MAGIC == pool->magic ) {
	PZ_Lock(pool->lock);
#ifdef THREADMARK
	{
	    threadmark_mark **pw, *tm;

	    if (PR_GetCurrentThread() != pool->marking_thread ) {
		PZ_Unlock(pool->lock);
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		PORT_Assert(0);
		return  ;
	    }

	    pw = &pool->first_mark;
	    while( *pw && (mark != (*pw)->mark) ) {
		pw = &(*pw)->next;
	    }

	    if (! *pw ) {
		
		PZ_Unlock(pool->lock);
		PORT_SetError(SEC_ERROR_NO_MEMORY);
		PORT_Assert(0);
		return  ;
	    }

	    tm = *pw;
	    *pw = (threadmark_mark *)NULL;

	    if (zero) {
		port_ArenaZeroAfterMark(arena, mark);
	    }
	    PL_ARENA_RELEASE(arena, mark);

	    if (! pool->first_mark ) {
		pool->marking_thread = (PRThread *)NULL;
	    }
	}
#else 
	if (zero) {
	    port_ArenaZeroAfterMark(arena, mark);
	}
	PL_ARENA_RELEASE(arena, mark);
#endif 
	PZ_Unlock(pool->lock);
    } else {
	if (zero) {
	    port_ArenaZeroAfterMark(arena, mark);
	}
	PL_ARENA_RELEASE(arena, mark);
    }
}

void
PORT_ArenaRelease(PLArenaPool *arena, void *mark)
{
    port_ArenaRelease(arena, mark, PR_FALSE);
}




void
PORT_ArenaZRelease(PLArenaPool *arena, void *mark)
{
    port_ArenaRelease(arena, mark, PR_TRUE);
}

void
PORT_ArenaUnmark(PLArenaPool *arena, void *mark)
{
#ifdef THREADMARK
    PORTArenaPool *pool = (PORTArenaPool *)arena;
    if (ARENAPOOL_MAGIC == pool->magic ) {
	threadmark_mark **pw, *tm;

	PZ_Lock(pool->lock);

	if (PR_GetCurrentThread() != pool->marking_thread ) {
	    PZ_Unlock(pool->lock);
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    PORT_Assert(0);
	    return  ;
	}

	pw = &pool->first_mark;
	while( ((threadmark_mark *)NULL != *pw) && (mark != (*pw)->mark) ) {
	    pw = &(*pw)->next;
	}

	if ((threadmark_mark *)NULL == *pw ) {
	    
	    PZ_Unlock(pool->lock);
	    PORT_SetError(SEC_ERROR_NO_MEMORY);
	    PORT_Assert(0);
	    return  ;
	}

	tm = *pw;
	*pw = (threadmark_mark *)NULL;

	if (! pool->first_mark ) {
	    pool->marking_thread = (PRThread *)NULL;
	}

	PZ_Unlock(pool->lock);
    }
#endif 
}

char *
PORT_ArenaStrdup(PLArenaPool *arena, const char *str) {
    int len = PORT_Strlen(str)+1;
    char *newstr;

    newstr = (char*)PORT_ArenaAlloc(arena,len);
    if (newstr) {
        PORT_Memcpy(newstr,str,len);
    }
    return newstr;
}












void
PORT_SetUCS4_UTF8ConversionFunction(PORTCharConversionFunc convFunc)
{ 
    ucs4Utf8ConvertFunc = convFunc;
}

void
PORT_SetUCS2_ASCIIConversionFunction(PORTCharConversionWSwapFunc convFunc)
{ 
    ucs2AsciiConvertFunc = convFunc;
}

void
PORT_SetUCS2_UTF8ConversionFunction(PORTCharConversionFunc convFunc)
{ 
    ucs2Utf8ConvertFunc = convFunc;
}

PRBool 
PORT_UCS4_UTF8Conversion(PRBool toUnicode, unsigned char *inBuf,
			 unsigned int inBufLen, unsigned char *outBuf,
			 unsigned int maxOutBufLen, unsigned int *outBufLen)
{
    if(!ucs4Utf8ConvertFunc) {
      return sec_port_ucs4_utf8_conversion_function(toUnicode,
        inBuf, inBufLen, outBuf, maxOutBufLen, outBufLen);
    }

    return (*ucs4Utf8ConvertFunc)(toUnicode, inBuf, inBufLen, outBuf, 
				  maxOutBufLen, outBufLen);
}

PRBool 
PORT_UCS2_UTF8Conversion(PRBool toUnicode, unsigned char *inBuf,
			 unsigned int inBufLen, unsigned char *outBuf,
			 unsigned int maxOutBufLen, unsigned int *outBufLen)
{
    if(!ucs2Utf8ConvertFunc) {
      return sec_port_ucs2_utf8_conversion_function(toUnicode,
        inBuf, inBufLen, outBuf, maxOutBufLen, outBufLen);
    }

    return (*ucs2Utf8ConvertFunc)(toUnicode, inBuf, inBufLen, outBuf, 
				  maxOutBufLen, outBufLen);
}

PRBool 
PORT_ISO88591_UTF8Conversion(const unsigned char *inBuf,
			 unsigned int inBufLen, unsigned char *outBuf,
			 unsigned int maxOutBufLen, unsigned int *outBufLen)
{
    return sec_port_iso88591_utf8_conversion_function(inBuf, inBufLen,
      outBuf, maxOutBufLen, outBufLen);
}

PRBool 
PORT_UCS2_ASCIIConversion(PRBool toUnicode, unsigned char *inBuf,
			  unsigned int inBufLen, unsigned char *outBuf,
			  unsigned int maxOutBufLen, unsigned int *outBufLen,
			  PRBool swapBytes)
{
    if(!ucs2AsciiConvertFunc) {
	return PR_FALSE;
    }

    return (*ucs2AsciiConvertFunc)(toUnicode, inBuf, inBufLen, outBuf, 
				  maxOutBufLen, outBufLen, swapBytes);
}





int
NSS_PutEnv(const char * envVarName, const char * envValue)
{
#ifdef _WIN32_WCE
    return SECFailure;
#else
    SECStatus result = SECSuccess;
    char *    encoded;
    int       putEnvFailed;
#ifdef _WIN32
    PRBool      setOK;

    setOK = SetEnvironmentVariable(envVarName, envValue);
    if (!setOK) {
        SET_ERROR_CODE
        return SECFailure;
    }
#endif

    encoded = (char *)PORT_ZAlloc(strlen(envVarName) + 2 + strlen(envValue));
    strcpy(encoded, envVarName);
    strcat(encoded, "=");
    strcat(encoded, envValue);

    putEnvFailed = putenv(encoded); 
    if (putEnvFailed) {
        SET_ERROR_CODE
        result = SECFailure;
        PORT_Free(encoded);
    }
    return result;
#endif
}

