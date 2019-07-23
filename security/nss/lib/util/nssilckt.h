



































































































































#ifndef _NSSILCKT_H_
#define _NSSILCKT_H_

#include "utilrename.h"
#include "prtypes.h"
#include "prmon.h"
#include "prlock.h"
#include "prcvar.h"

typedef enum {
    nssILockArena = 0,
    nssILockSession = 1,
    nssILockObject = 2,
    nssILockRefLock = 3,
    nssILockCert = 4,
    nssILockCertDB = 5,
    nssILockDBM = 6,
    nssILockCache = 7,
    nssILockSSL = 8,
    nssILockList = 9,
    nssILockSlot = 10,
    nssILockFreelist = 11,
    nssILockOID = 12,
    nssILockAttribute = 13,
    nssILockPK11cxt = 14,  
    nssILockRWLock = 15,
    nssILockOther = 16,
    nssILockSelfServ = 17,
    nssILockKeyDB = 18,
    nssILockLast  
} nssILockType;




#if defined(NEED_NSS_ILOCK)





typedef enum  {
    FlushTT = 0,
    NewLock = 1,
    Lock = 2,
    Unlock = 3,
    DestroyLock = 4,
    NewCondVar = 5,
    WaitCondVar = 6,
    NotifyCondVar = 7,
    NotifyAllCondVar = 8,
    DestroyCondVar = 9,
    NewMonitor = 10,
    EnterMonitor = 11,
    ExitMonitor = 12,
    Notify = 13,
    NotifyAll = 14,
    Wait = 15,
    DestroyMonitor = 16
} nssILockOp;




struct pzTrace_s {
    PRUint32        threadID; 
    nssILockOp      op;       
    nssILockType    ltype;    
    PRIntervalTime  callTime; 
    PRIntervalTime  heldTime; 
    void            *lock;        
    PRIntn          line;     
    char            file[24]; 
};




typedef struct pzlock_s PZLock;
typedef struct pzcondvar_s PZCondVar;
typedef struct pzmonitor_s PZMonitor;

#else 

#define PZLock                  PRLock
#define PZCondVar               PRCondVar
#define PZMonitor               PRMonitor
    
#endif 

#endif 
