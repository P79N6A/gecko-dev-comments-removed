



































#ifndef __SSLMUTEX_H_
#define __SSLMUTEX_H_ 1




















#include "prtypes.h"
#include "prlock.h"

#if defined(NETBSD)
#include <sys/param.h> 
#endif

#if defined(WIN32)

#include <wtypes.h>

typedef struct 
{
    PRBool isMultiProcess;
#ifdef WINNT
    
    struct {
#else
    union {
#endif
        PRLock* sslLock;
        HANDLE sslMutx;
    } u;
} sslMutex;

typedef int    sslPID;

#elif defined(LINUX) || defined(AIX) || defined(BEOS) || defined(BSDI) || (defined(NETBSD) && __NetBSD_Version__ < 500000000) || defined(OPENBSD)

#include <sys/types.h>
#include "prtypes.h"

typedef struct { 
    PRBool isMultiProcess;
    union {
        PRLock* sslLock;
        struct {
            int      mPipes[3]; 
            PRInt32  nWaiters;
        } pipeStr;
    } u;
} sslMutex;
typedef pid_t sslPID;

#elif defined(XP_UNIX) 

#include <sys/types.h>	
#include <semaphore.h>  

typedef struct
{
    PRBool isMultiProcess;
    union {
        PRLock* sslLock;
        sem_t  sem;
    } u;
} sslMutex;

typedef pid_t sslPID;

#else



typedef struct { 
    PRBool isMultiProcess;
    union {
        PRLock* sslLock;
        
    } u;
} sslMutex;

typedef int sslPID;

#endif

#include "seccomon.h"

SEC_BEGIN_PROTOS

extern SECStatus sslMutex_Init(sslMutex *sem, int shared);

extern SECStatus sslMutex_Destroy(sslMutex *sem);

extern SECStatus sslMutex_Unlock(sslMutex *sem);

extern SECStatus sslMutex_Lock(sslMutex *sem);

#ifdef WINNT

extern SECStatus sslMutex_2LevelInit(sslMutex *sem);

#endif

SEC_END_PROTOS

#endif
