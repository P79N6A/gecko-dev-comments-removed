




































#ifndef nsHostResolver_h__
#define nsHostResolver_h__

#include "nscore.h"
#include "nsAtomicRefcnt.h"
#include "prclist.h"
#include "prnetdb.h"
#include "pldhash.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nsTArray.h"

class nsHostResolver;
class nsHostRecord;
class nsResolveHostCallback;


#define NS_DECL_REFCOUNTED_THREADSAFE(classname)                             \
  private:                                                                   \
    nsAutoRefCnt _refc;                                                      \
  public:                                                                    \
    PRInt32 AddRef() {                                                       \
        PRInt32 n = NS_AtomicIncrementRefcnt(_refc);                         \
        NS_LOG_ADDREF(this, n, #classname, sizeof(classname));               \
        return n;                                                            \
    }                                                                        \
    PRInt32 Release() {                                                      \
        PRInt32 n = NS_AtomicDecrementRefcnt(_refc);                         \
        NS_LOG_RELEASE(this, n, #classname);                                 \
        if (n == 0)                                                          \
            delete this;                                                     \
        return n;                                                            \
    }

#define MAX_RESOLVER_THREADS_FOR_ANY_PRIORITY  3
#define MAX_RESOLVER_THREADS_FOR_HIGH_PRIORITY 5
#define MAX_RESOLVER_THREADS (MAX_RESOLVER_THREADS_FOR_ANY_PRIORITY + \
                              MAX_RESOLVER_THREADS_FOR_HIGH_PRIORITY)

struct nsHostKey
{
    const char *host;
    PRUint16    flags;
    PRUint16    af;
};




class nsHostRecord : public PRCList, public nsHostKey
{
    typedef mozilla::Mutex Mutex;

public:
    NS_DECL_REFCOUNTED_THREADSAFE(nsHostRecord)

    
    static nsresult Create(const nsHostKey *key, nsHostRecord **record);

    








    







    Mutex        addr_info_lock;
    int          addr_info_gencnt; 
    PRAddrInfo  *addr_info;
    PRNetAddr   *addr;
    PRBool       negative;   




    PRUint32     expiration; 

    PRBool HasResult() const { return addr_info || addr || negative; }

    
    PRBool Blacklisted(PRNetAddr *query);
    void   ResetBlacklist();
    void   ReportUnusable(PRNetAddr *addr);

private:
    friend class nsHostResolver;

    PRCList callbacks; 

    PRBool  resolving; 

 
    
    PRBool  onQueue;  
    PRBool  usingAnyThread; 

    
    
    
    nsTArray<nsCString> mBlacklistedItems;

    nsHostRecord(const nsHostKey *key);           
   ~nsHostRecord();
};





class NS_NO_VTABLE nsResolveHostCallback : public PRCList
{
public:
    
















    virtual void OnLookupComplete(nsHostResolver *resolver,
                                  nsHostRecord   *record,
                                  nsresult        status) = 0;
};




class nsHostResolver
{
    typedef mozilla::CondVar CondVar;
    typedef mozilla::Mutex Mutex;

public:
    


    NS_DECL_REFCOUNTED_THREADSAFE(nsHostResolver)

    


    static nsresult Create(PRUint32         maxCacheEntries,  
                           PRUint32         maxCacheLifetime, 
                           nsHostResolver **resolver);
    
    



    void Shutdown();

    






    nsresult ResolveHost(const char            *hostname,
                         PRUint16               flags,
                         PRUint16               af,
                         nsResolveHostCallback *callback);

    





    void DetachCallback(const char            *hostname,
                        PRUint16               flags,
                        PRUint16               af,
                        nsResolveHostCallback *callback,
                        nsresult               status);

    






    enum {
        RES_BYPASS_CACHE = 1 << 0,
        RES_CANON_NAME   = 1 << 1,
        RES_PRIORITY_MEDIUM   = 1 << 2,
        RES_PRIORITY_LOW  = 1 << 3,
        RES_SPECULATE     = 1 << 4   
    };

private:
    nsHostResolver(PRUint32 maxCacheEntries=50, PRUint32 maxCacheLifetime=1);
   ~nsHostResolver();

    nsresult Init();
    nsresult IssueLookup(nsHostRecord *);
    PRBool   GetHostToLookup(nsHostRecord **m);
    void     OnLookupComplete(nsHostRecord *, nsresult, PRAddrInfo *);
    void     DeQueue(PRCList &aQ, nsHostRecord **aResult);
    void     ClearPendingQueue(PRCList *aPendingQueue);
    nsresult ConditionallyCreateThread(nsHostRecord *rec);
    
    static void  MoveQueue(nsHostRecord *aRec, PRCList &aDestQ);
    
    static void ThreadFunc(void *);

    PRUint32      mMaxCacheEntries;
    PRUint32      mMaxCacheLifetime;
    Mutex         mLock;
    CondVar       mIdleThreadCV;
    PRUint32      mNumIdleThreads;
    PRUint32      mThreadCount;
    PRUint32      mActiveAnyThreadCount;
    PLDHashTable  mDB;
    PRCList       mHighQ;
    PRCList       mMediumQ;
    PRCList       mLowQ;
    PRCList       mEvictionQ;
    PRUint32      mEvictionQSize;
    PRUint32      mPendingCount;
    PRTime        mCreationTime;
    PRBool        mShutdown;
    PRIntervalTime mLongIdleTimeout;
    PRIntervalTime mShortIdleTimeout;
};

#endif 
