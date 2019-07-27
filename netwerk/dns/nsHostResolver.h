




#ifndef nsHostResolver_h__
#define nsHostResolver_h__

#include "nscore.h"
#include "prclist.h"
#include "prnetdb.h"
#include "pldhash.h"
#include "mozilla/CondVar.h"
#include "mozilla/Mutex.h"
#include "nsISupportsImpl.h"
#include "nsIDNSListener.h"
#include "nsString.h"
#include "nsTArray.h"
#include "GetAddrInfo.h"
#include "mozilla/net/DNS.h"
#include "mozilla/net/DashboardTypes.h"
#include "mozilla/TimeStamp.h"

class nsHostResolver;
class nsHostRecord;
class nsResolveHostCallback;

#define MAX_RESOLVER_THREADS_FOR_ANY_PRIORITY  3
#define MAX_RESOLVER_THREADS_FOR_HIGH_PRIORITY 5
#define MAX_NON_PRIORITY_REQUESTS 150

#define MAX_RESOLVER_THREADS (MAX_RESOLVER_THREADS_FOR_ANY_PRIORITY + \
                              MAX_RESOLVER_THREADS_FOR_HIGH_PRIORITY)

struct nsHostKey
{
    const char *host;
    uint16_t    flags;
    uint16_t    af;
};




class nsHostRecord : public PRCList, public nsHostKey
{
    typedef mozilla::Mutex Mutex;

public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsHostRecord)

    
    static nsresult Create(const nsHostKey *key, nsHostRecord **record);

    








    







    Mutex        addr_info_lock;
    int          addr_info_gencnt; 
    mozilla::net::AddrInfo *addr_info;
    mozilla::net::NetAddr  *addr;
    bool         negative;   




    enum ExpirationStatus {
        EXP_VALID,
        EXP_GRACE,
        EXP_EXPIRED,
    };

    ExpirationStatus CheckExpiration(const mozilla::TimeStamp& now) const;

    
    mozilla::TimeStamp mValidStart;

    
    mozilla::TimeStamp mValidEnd;

    
    
    
    mozilla::TimeStamp mGraceStart;

    
    
    void SetExpiration(const mozilla::TimeStamp& now, unsigned int valid,
                       unsigned int grace);

    
    bool HasUsableResult(const mozilla::TimeStamp& now, uint16_t queryFlags = 0) const;

    
    bool   Blacklisted(mozilla::net::NetAddr *query);
    void   ResetBlacklist();
    void   ReportUnusable(mozilla::net::NetAddr *addr);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

    enum DnsPriority {
        DNS_PRIORITY_LOW,
        DNS_PRIORITY_MEDIUM,
        DNS_PRIORITY_HIGH,
    };
    static DnsPriority GetPriority(uint16_t aFlags);

private:
    friend class nsHostResolver;


    PRCList callbacks; 

    bool    resolving; 



    bool    onQueue;  
    bool    usingAnyThread; 
    bool    mDoomed; 

#if TTL_AVAILABLE
    bool    mGetTtl;
#endif

    
    
    uint32_t mBlacklistedCount;

    
    
    
    nsTArray<nsCString> mBlacklistedItems;

    explicit nsHostRecord(const nsHostKey *key);           
   ~nsHostRecord();
};





class NS_NO_VTABLE nsResolveHostCallback : public PRCList
{
public:
    
















    virtual void OnLookupComplete(nsHostResolver *resolver,
                                  nsHostRecord   *record,
                                  nsresult        status) = 0;
    












    virtual bool EqualsAsyncListener(nsIDNSListener *aListener) = 0;

    virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf) const = 0;
};




class nsHostResolver
{
    typedef mozilla::CondVar CondVar;
    typedef mozilla::Mutex Mutex;

public:
    


    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(nsHostResolver)

    


    static nsresult Create(uint32_t maxCacheEntries, 
                           uint32_t defaultCacheEntryLifetime, 
                           uint32_t defaultGracePeriod, 
                           nsHostResolver **resolver);
    
    



    void Shutdown();

    






    nsresult ResolveHost(const char            *hostname,
                         uint16_t               flags,
                         uint16_t               af,
                         nsResolveHostCallback *callback);

    





    void DetachCallback(const char            *hostname,
                        uint16_t               flags,
                        uint16_t               af,
                        nsResolveHostCallback *callback,
                        nsresult               status);

    






    void CancelAsyncRequest(const char            *host,
                            uint16_t               flags,
                            uint16_t               af,
                            nsIDNSListener        *aListener,
                            nsresult               status);
    






    enum {
        RES_BYPASS_CACHE = 1 << 0,
        RES_CANON_NAME   = 1 << 1,
        RES_PRIORITY_MEDIUM   = 1 << 2,
        RES_PRIORITY_LOW  = 1 << 3,
        RES_SPECULATE     = 1 << 4,
        
        RES_OFFLINE       = 1 << 6
    };

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

private:
   explicit nsHostResolver(uint32_t maxCacheEntries,
                           uint32_t defaultCacheEntryLifetime,
                           uint32_t defaultGracePeriod);
   ~nsHostResolver();

    nsresult Init();
    nsresult IssueLookup(nsHostRecord *);
    bool     GetHostToLookup(nsHostRecord **m);
    void     OnLookupComplete(nsHostRecord *, nsresult, mozilla::net::AddrInfo *);
    void     DeQueue(PRCList &aQ, nsHostRecord **aResult);
    void     ClearPendingQueue(PRCList *aPendingQueue);
    nsresult ConditionallyCreateThread(nsHostRecord *rec);

    



    nsresult ConditionallyRefreshRecord(nsHostRecord *rec, const char *host);
    
#if TTL_AVAILABLE
    

    
    
    
    
    void DnsExperimentChangedInternal();

    
    static void DnsExperimentChanged(const char* aPref, void* aClosure);

    
    void InitCRandom();
#endif

    static void  MoveQueue(nsHostRecord *aRec, PRCList &aDestQ);
    
    static void ThreadFunc(void *);

    enum {
        METHOD_HIT = 1,
        METHOD_RENEWAL = 2,
        METHOD_NEGATIVE_HIT = 3,
        METHOD_LITERAL = 4,
        METHOD_OVERFLOW = 5,
        METHOD_NETWORK_FIRST = 6,
        METHOD_NETWORK_SHARED = 7
    };

    uint32_t      mMaxCacheEntries;
    uint32_t      mDefaultCacheLifetime; 
    uint32_t      mDefaultGracePeriod; 
    mutable Mutex mLock;    
    CondVar       mIdleThreadCV;
    uint32_t      mNumIdleThreads;
    uint32_t      mThreadCount;
    uint32_t      mActiveAnyThreadCount;
    PLDHashTable  mDB;
    PRCList       mHighQ;
    PRCList       mMediumQ;
    PRCList       mLowQ;
    PRCList       mEvictionQ;
    uint32_t      mEvictionQSize;
    uint32_t      mPendingCount;
    PRTime        mCreationTime;
    bool          mShutdown;
    PRIntervalTime mLongIdleTimeout;
    PRIntervalTime mShortIdleTimeout;

    
    void PrepareRecordExpiration(nsHostRecord* rec) const;

public:
    


    void GetDNSCacheEntries(nsTArray<mozilla::net::DNSCacheEntries> *);
};

#endif 
