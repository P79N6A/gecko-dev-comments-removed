



































































































#ifndef _NSSILOCK_H_
#define _NSSILOCK_H_

#include "utilrename.h"
#include "prtypes.h"
#include "prmon.h"
#include "prlock.h"
#include "prcvar.h"

#include "nssilckt.h"

PR_BEGIN_EXTERN_C

#if defined(NEED_NSS_ILOCK)

#define PZ_NewLock(t) pz_NewLock((t),__FILE__,__LINE__)
extern PZLock * 
    pz_NewLock(
        nssILockType ltype,
        char *file,
        PRIntn  line
    );

#define PZ_Lock(k)  pz_Lock((k),__FILE__,__LINE__)
extern void
    pz_Lock(
        PZLock *lock,
        char *file,
        PRIntn line
    );

#define PZ_Unlock(k) pz_Unlock((k),__FILE__,__LINE__)
extern PRStatus
    pz_Unlock(
        PZLock *lock,
        char *file,
        PRIntn line
    );

#define PZ_DestroyLock(k) pz_DestroyLock((k),__FILE__,__LINE__)
extern void
    pz_DestroyLock(
        PZLock *lock,
        char *file,
        PRIntn line
    );


#define PZ_NewCondVar(l)        pz_NewCondVar((l),__FILE__,__LINE__)
extern PZCondVar *
    pz_NewCondVar(
        PZLock *lock,
        char *file,
        PRIntn line
    );

#define PZ_DestroyCondVar(v)    pz_DestroyCondVar((v),__FILE__,__LINE__)
extern void
    pz_DestroyCondVar(
        PZCondVar *cvar,
        char *file,
        PRIntn line
    );

#define PZ_WaitCondVar(v,t)       pz_WaitCondVar((v),(t),__FILE__,__LINE__)
extern PRStatus
    pz_WaitCondVar(
        PZCondVar *cvar,
        PRIntervalTime timeout,
        char *file,
        PRIntn line
    );

#define PZ_NotifyCondVar(v)     pz_NotifyCondVar((v),__FILE__,__LINE__)
extern PRStatus
    pz_NotifyCondVar(
        PZCondVar *cvar,
        char *file,
        PRIntn line
    );

#define PZ_NotifyAllCondVar(v)  pz_NotifyAllCondVar((v),__FILE__,__LINE__)
extern PRStatus
    pz_NotifyAllCondVar(
        PZCondVar *cvar,
        char *file,
        PRIntn line
    );


#define PZ_NewMonitor(t) pz_NewMonitor((t),__FILE__,__LINE__)
extern PZMonitor *
    pz_NewMonitor( 
        nssILockType ltype,
        char *file,
        PRIntn line
    );

#define PZ_DestroyMonitor(m) pz_DestroyMonitor((m),__FILE__,__LINE__)
extern void
    pz_DestroyMonitor(
        PZMonitor *mon,
        char *file,
        PRIntn line
    );

#define PZ_EnterMonitor(m) pz_EnterMonitor((m),__FILE__,__LINE__)
extern void
    pz_EnterMonitor(
        PZMonitor *mon,
        char *file,
        PRIntn line
    );


#define PZ_ExitMonitor(m) pz_ExitMonitor((m),__FILE__,__LINE__)
extern PRStatus
    pz_ExitMonitor(
        PZMonitor *mon,
        char *file,
        PRIntn line
    );

#define PZ_InMonitor(m)  (PZ_GetMonitorEntryCount(m) > 0 )
#define PZ_GetMonitorEntryCount(m) pz_GetMonitorEntryCount((m),__FILE__,__LINE__)
extern PRIntn
    pz_GetMonitorEntryCount(
        PZMonitor *mon,
        char *file,
        PRIntn line
    );

#define PZ_Wait(m,i) pz_Wait((m),((i)),__FILE__,__LINE__)
extern PRStatus
    pz_Wait(
        PZMonitor *mon,
        PRIntervalTime ticks,
        char *file,
        PRIntn line
    );

#define PZ_Notify(m) pz_Notify((m),__FILE__,__LINE__)
extern PRStatus
    pz_Notify(
        PZMonitor *mon,
        char *file,
        PRIntn line
    );

#define PZ_NotifyAll(m) pz_NotifyAll((m),__FILE__,__LINE__)
extern PRStatus
    pz_NotifyAll(
        PZMonitor *mon,
        char *file,
        PRIntn line
    );

#define PZ_TraceFlush() pz_TraceFlush()
extern void pz_TraceFlush( void );

#else 

#define PZ_NewLock(t)           PR_NewLock()
#define PZ_DestroyLock(k)       PR_DestroyLock((k))
#define PZ_Lock(k)              PR_Lock((k))
#define PZ_Unlock(k)            PR_Unlock((k))

#define PZ_NewCondVar(l)        PR_NewCondVar((l))
#define PZ_DestroyCondVar(v)    PR_DestroyCondVar((v))
#define PZ_WaitCondVar(v,t)     PR_WaitCondVar((v),(t))
#define PZ_NotifyCondVar(v)     PR_NotifyCondVar((v))
#define PZ_NotifyAllCondVar(v)  PR_NotifyAllCondVar((v))

#define PZ_NewMonitor(t)        PR_NewMonitor()
#define PZ_DestroyMonitor(m)    PR_DestroyMonitor((m))
#define PZ_EnterMonitor(m)      PR_EnterMonitor((m))
#define PZ_ExitMonitor(m)       PR_ExitMonitor((m))
#define PZ_InMonitor(m)         PR_InMonitor((m))
#define PZ_Wait(m,t)            PR_Wait(((m)),((t)))
#define PZ_Notify(m)            PR_Notify((m))
#define PZ_NotifyAll(m)         PR_Notify((m))
#define PZ_TraceFlush()

    
#endif 

PR_END_EXTERN_C
#endif 
