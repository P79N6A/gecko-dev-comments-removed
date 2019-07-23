




































#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif

#if defined(HAVE_RES_NINIT)
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>   
#include <arpa/nameser.h>
#include <resolv.h>
#define RES_RETRY_ON_FAILURE
#endif

#include <stdlib.h>
#include "nsHostResolver.h"
#include "nsNetError.h"
#include "nsISupportsBase.h"
#include "nsISupportsUtils.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "pratom.h"
#include "prthread.h"
#include "prerror.h"
#include "prcvar.h"
#include "prtime.h"
#include "prlong.h"
#include "prlog.h"
#include "pldhash.h"
#include "plstr.h"
#include "nsURLHelper.h"



#define MAX_THREADS 8
#define IDLE_TIMEOUT PR_SecondsToInterval(60)



#if defined(PR_LOGGING)
static PRLogModuleInfo *gHostResolverLog = nsnull;
#define LOG(args) PR_LOG(gHostResolverLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif



static inline void
MoveCList(PRCList &from, PRCList &to)
{
    if (!PR_CLIST_IS_EMPTY(&from)) {
        to.next = from.next;
        to.prev = from.prev;
        to.next->prev = &to;
        to.prev->next = &to;
        PR_INIT_CLIST(&from);
    }             
}

static PRUint32
NowInMinutes()
{
    PRTime now = PR_Now(), minutes, factor;
    LL_I2L(factor, 60 * PR_USEC_PER_SEC);
    LL_DIV(minutes, now, factor);
    PRUint32 result;
    LL_L2UI(result, minutes);
    return result;
}



#if defined(RES_RETRY_ON_FAILURE)








class nsResState
{
public:
    nsResState()
        
        
        
        
        
        
        
        : mLastReset(PR_IntervalNow())
    {
    }

    PRBool Reset()
    {
        
        if (PR_IntervalToSeconds(PR_IntervalNow() - mLastReset) < 1)
            return PR_FALSE;

        LOG(("calling res_ninit\n"));

        mLastReset = PR_IntervalNow();
        return (res_ninit(&_res) == 0);
    }

private:
    PRIntervalTime mLastReset;
};

#endif 






#define RES_KEY_FLAGS(_f) ((_f) & nsHostResolver::RES_CANON_NAME)

nsresult
nsHostRecord::Create(const nsHostKey *key, nsHostRecord **result)
{
    size_t hostLen = strlen(key->host) + 1;
    size_t size = hostLen + sizeof(nsHostRecord);

    nsHostRecord *rec = (nsHostRecord*) ::operator new(size);
    if (!rec)
        return NS_ERROR_OUT_OF_MEMORY;

    rec->host = ((char *) rec) + sizeof(nsHostRecord);
    rec->flags = RES_KEY_FLAGS(key->flags);
    rec->af = key->af;

    rec->_refc = 1; 
    NS_LOG_ADDREF(rec, 1, "nsHostRecord", sizeof(nsHostRecord));
    rec->addr_info = nsnull;
    rec->addr = nsnull;
    rec->expiration = NowInMinutes();
    rec->resolving = PR_FALSE;
    PR_INIT_CLIST(rec);
    PR_INIT_CLIST(&rec->callbacks);
    memcpy((char *) rec->host, key->host, hostLen);

    *result = rec;
    return NS_OK;
}

nsHostRecord::~nsHostRecord()
{
    if (addr_info)
        PR_FreeAddrInfo(addr_info);
    if (addr)
        free(addr);
}



struct nsHostDBEnt : PLDHashEntryHdr
{
    nsHostRecord *rec;
};

PR_STATIC_CALLBACK(PLDHashNumber)
HostDB_HashKey(PLDHashTable *table, const void *key)
{
    const nsHostKey *hk = NS_STATIC_CAST(const nsHostKey *, key);
    return PL_DHashStringKey(table, hk->host) ^ hk->flags ^ hk->af;
}

PR_STATIC_CALLBACK(PRBool)
HostDB_MatchEntry(PLDHashTable *table,
                  const PLDHashEntryHdr *entry,
                  const void *key)
{
    const nsHostDBEnt *he = NS_STATIC_CAST(const nsHostDBEnt *, entry);
    const nsHostKey *hk = NS_STATIC_CAST(const nsHostKey *, key); 

    return !strcmp(he->rec->host, hk->host) &&
            he->rec->flags == hk->flags &&
            he->rec->af == hk->af;
}

PR_STATIC_CALLBACK(void)
HostDB_MoveEntry(PLDHashTable *table,
                 const PLDHashEntryHdr *from,
                 PLDHashEntryHdr *to)
{
    NS_STATIC_CAST(nsHostDBEnt *, to)->rec =
            NS_STATIC_CAST(const nsHostDBEnt *, from)->rec;
}

PR_STATIC_CALLBACK(void)
HostDB_ClearEntry(PLDHashTable *table,
                  PLDHashEntryHdr *entry)
{
    LOG(("evicting record\n"));
    nsHostDBEnt *he = NS_STATIC_CAST(nsHostDBEnt *, entry);
#if defined(DEBUG) && defined(PR_LOGGING)
    if (!he->rec->addr_info)
        LOG(("%s: => no addr_info\n", he->rec->host));
    else {
        PRInt32 now = (PRInt32) NowInMinutes();
        PRInt32 diff = (PRInt32) he->rec->expiration - now;
        LOG(("%s: exp=%d => %s\n",
            he->rec->host, diff,
            PR_GetCanonNameFromAddrInfo(he->rec->addr_info)));
        void *iter = nsnull;
        PRNetAddr addr;
        char buf[64];
        for (;;) {
            iter = PR_EnumerateAddrInfo(iter, he->rec->addr_info, 0, &addr);
            if (!iter)
                break;
            PR_NetAddrToString(&addr, buf, sizeof(buf));
            LOG(("  %s\n", buf));
        }
    }
#endif
    NS_RELEASE(he->rec);
}

PR_STATIC_CALLBACK(PRBool)
HostDB_InitEntry(PLDHashTable *table,
                 PLDHashEntryHdr *entry,
                 const void *key)
{
    nsHostDBEnt *he = NS_STATIC_CAST(nsHostDBEnt *, entry);
    nsHostRecord::Create(NS_STATIC_CAST(const nsHostKey *, key), &he->rec);
    return PR_TRUE;
}

static PLDHashTableOps gHostDB_ops =
{
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    HostDB_HashKey,
    HostDB_MatchEntry,
    HostDB_MoveEntry,
    HostDB_ClearEntry,
    PL_DHashFinalizeStub,
    HostDB_InitEntry,
};

PR_STATIC_CALLBACK(PLDHashOperator)
HostDB_RemoveEntry(PLDHashTable *table,
                   PLDHashEntryHdr *hdr,
                   PRUint32 number,
                   void *arg)
{
    return PL_DHASH_REMOVE;
}



nsHostResolver::nsHostResolver(PRUint32 maxCacheEntries,
                               PRUint32 maxCacheLifetime)
    : mMaxCacheEntries(maxCacheEntries)
    , mMaxCacheLifetime(maxCacheLifetime)
    , mLock(nsnull)
    , mIdleThreadCV(nsnull)
    , mHaveIdleThread(PR_FALSE)
    , mThreadCount(0)
    , mEvictionQSize(0)
    , mShutdown(PR_TRUE)
{
    mCreationTime = PR_Now();
    PR_INIT_CLIST(&mPendingQ);
    PR_INIT_CLIST(&mEvictionQ);
}

nsHostResolver::~nsHostResolver()
{
    if (mIdleThreadCV)
        PR_DestroyCondVar(mIdleThreadCV);

    if (mLock)
        PR_DestroyLock(mLock);

    PL_DHashTableFinish(&mDB);
}

nsresult
nsHostResolver::Init()
{
    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    mIdleThreadCV = PR_NewCondVar(mLock);
    if (!mIdleThreadCV)
        return NS_ERROR_OUT_OF_MEMORY;

    PL_DHashTableInit(&mDB, &gHostDB_ops, nsnull, sizeof(nsHostDBEnt), 0);

    mShutdown = PR_FALSE;

#if defined(HAVE_RES_NINIT)
    
    
    
    
    
    static int initCount = 0;
    if (initCount++ > 0) {
        LOG(("calling res_ninit\n"));
        res_ninit(&_res);
    }
#endif
    return NS_OK;
}

void
nsHostResolver::Shutdown()
{
    LOG(("nsHostResolver::Shutdown\n"));

    PRCList pendingQ, evictionQ;
    PR_INIT_CLIST(&pendingQ);
    PR_INIT_CLIST(&evictionQ);

    {
        nsAutoLock lock(mLock);
        
        mShutdown = PR_TRUE;

        MoveCList(mPendingQ, pendingQ);
        MoveCList(mEvictionQ, evictionQ);
        mEvictionQSize = 0;

        if (mHaveIdleThread)
            PR_NotifyCondVar(mIdleThreadCV);
        
        
        PL_DHashTableEnumerate(&mDB, HostDB_RemoveEntry, nsnull);
    }

    
    if (!PR_CLIST_IS_EMPTY(&pendingQ)) {
        PRCList *node = pendingQ.next;
        while (node != &pendingQ) {
            nsHostRecord *rec = NS_STATIC_CAST(nsHostRecord *, node);
            node = node->next;
            OnLookupComplete(rec, NS_ERROR_ABORT, nsnull);
        }
    }

    if (!PR_CLIST_IS_EMPTY(&evictionQ)) {
        PRCList *node = evictionQ.next;
        while (node != &evictionQ) {
            nsHostRecord *rec = NS_STATIC_CAST(nsHostRecord *, node);
            node = node->next;
            NS_RELEASE(rec);
        }
    }

}

nsresult
nsHostResolver::ResolveHost(const char            *host,
                            PRUint16               flags,
                            PRUint16               af,
                            nsResolveHostCallback *callback)
{
    NS_ENSURE_TRUE(host && *host, NS_ERROR_UNEXPECTED);

    LOG(("nsHostResolver::ResolveHost [host=%s]\n", host));

    
    
    if (!net_IsValidHostName(nsDependentCString(host)))
        return NS_ERROR_UNKNOWN_HOST;

    
    
    nsRefPtr<nsHostRecord> result;
    nsresult status = NS_OK, rv = NS_OK;
    {
        nsAutoLock lock(mLock);

        if (mShutdown)
            rv = NS_ERROR_NOT_INITIALIZED;
        else {
            PRNetAddr tempAddr;

            
            
            memset(&tempAddr, 0, sizeof(PRNetAddr));
            
            
            
            
            
            
            

            nsHostKey key = { host, flags, af };
            nsHostDBEnt *he = NS_STATIC_CAST(nsHostDBEnt *,
                    PL_DHashTableOperate(&mDB, &key, PL_DHASH_ADD));

            
            if (!he || !he->rec)
                rv = NS_ERROR_OUT_OF_MEMORY;
            
            else if (!(flags & RES_BYPASS_CACHE) &&
                     he->rec->HasResult() &&
                     NowInMinutes() <= he->rec->expiration) {
                LOG(("using cached record\n"));
                
                result = he->rec;
            }
            
            
            
            else if (PR_StringToNetAddr(host, &tempAddr) == PR_SUCCESS) {
                
                
                he->rec->addr = (PRNetAddr *) malloc(sizeof(PRNetAddr));
                if (!he->rec->addr)
                    status = NS_ERROR_OUT_OF_MEMORY;
                else
                    memcpy(he->rec->addr, &tempAddr, sizeof(PRNetAddr));
                
                result = he->rec;
            }
            
            else {
                
                PR_APPEND_LINK(callback, &he->rec->callbacks);

                if (!he->rec->resolving) {
                    rv = IssueLookup(he->rec);
                    if (NS_FAILED(rv))
                        PR_REMOVE_AND_INIT_LINK(callback);
                }
            }
        }
    }
    if (result)
        callback->OnLookupComplete(this, result, status);
    return rv;
}

void
nsHostResolver::DetachCallback(const char            *host,
                               PRUint16               flags,
                               PRUint16               af,
                               nsResolveHostCallback *callback,
                               nsresult               status)
{
    nsRefPtr<nsHostRecord> rec;
    {
        nsAutoLock lock(mLock);

        nsHostKey key = { host, flags, af };
        nsHostDBEnt *he = NS_STATIC_CAST(nsHostDBEnt *,
                PL_DHashTableOperate(&mDB, &key, PL_DHASH_LOOKUP));
        if (he && he->rec) {
            
            
            PRCList *node = he->rec->callbacks.next;
            while (node != &he->rec->callbacks) {
                if (NS_STATIC_CAST(nsResolveHostCallback *, node) == callback) {
                    PR_REMOVE_LINK(callback);
                    rec = he->rec;
                    break;
                }
                node = node->next;
            }
        }
    }

    
    
    if (rec)
        callback->OnLookupComplete(this, rec, status);
}

nsresult
nsHostResolver::IssueLookup(nsHostRecord *rec)
{
    NS_ASSERTION(!rec->resolving, "record is already being resolved"); 

    
    
    
    if (rec->next == rec)
        NS_ADDREF(rec);
    else {
        PR_REMOVE_LINK(rec);
        mEvictionQSize--;
    }
    PR_APPEND_LINK(rec, &mPendingQ);
    rec->resolving = PR_TRUE;

    if (mHaveIdleThread) {
        
        PR_NotifyCondVar(mIdleThreadCV);
    }
    else if (mThreadCount < MAX_THREADS) {
        
        NS_ADDREF_THIS(); 
        mThreadCount++;
        PRThread *thr = PR_CreateThread(PR_SYSTEM_THREAD,
                                        ThreadFunc,
                                        this,
                                        PR_PRIORITY_NORMAL,
                                        PR_GLOBAL_THREAD,
                                        PR_UNJOINABLE_THREAD,
                                        0);
        if (!thr) {
            mThreadCount--;
            NS_RELEASE_THIS();
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
#if defined(PR_LOGGING)
    else
      LOG(("lookup waiting for thread - %s ...\n", rec->host));
#endif

    return NS_OK;
}

PRBool
nsHostResolver::GetHostToLookup(nsHostRecord **result)
{
    nsAutoLock lock(mLock);

    PRIntervalTime start = PR_IntervalNow(), timeout = IDLE_TIMEOUT;
    
    
    
    
    
    
    
    
    while (PR_CLIST_IS_EMPTY(&mPendingQ) && !mHaveIdleThread && !mShutdown) {
        
        mHaveIdleThread = PR_TRUE;
        PR_WaitCondVar(mIdleThreadCV, timeout);
        mHaveIdleThread = PR_FALSE;

        PRIntervalTime delta = PR_IntervalNow() - start;
        if (delta >= timeout)
            break;
        timeout -= delta;
        start += delta;
    }

    if (!PR_CLIST_IS_EMPTY(&mPendingQ)) {
        
        *result = NS_STATIC_CAST(nsHostRecord *, mPendingQ.next);
        PR_REMOVE_AND_INIT_LINK(*result);
        return PR_TRUE;
    }

    
    mThreadCount--;
    return PR_FALSE;
}

void
nsHostResolver::OnLookupComplete(nsHostRecord *rec, nsresult status, PRAddrInfo *result)
{
    
    
    PRCList cbs;
    PR_INIT_CLIST(&cbs);
    {
        nsAutoLock lock(mLock);

        
        MoveCList(rec->callbacks, cbs);

        
        rec->addr_info = result;
        rec->expiration = NowInMinutes() + mMaxCacheLifetime;
        rec->resolving = PR_FALSE;
        
        if (rec->addr_info && !mShutdown) {
            
            PR_APPEND_LINK(rec, &mEvictionQ);
            NS_ADDREF(rec);
            if (mEvictionQSize < mMaxCacheEntries)
                mEvictionQSize++;
            else {
                
                nsHostRecord *head =
                    NS_STATIC_CAST(nsHostRecord *, PR_LIST_HEAD(&mEvictionQ));
                PR_REMOVE_AND_INIT_LINK(head);
                PL_DHashTableOperate(&mDB, (nsHostKey *) head, PL_DHASH_REMOVE);
                
                NS_RELEASE(head);
            }
        }
    }

    if (!PR_CLIST_IS_EMPTY(&cbs)) {
        PRCList *node = cbs.next;
        while (node != &cbs) {
            nsResolveHostCallback *callback =
                    NS_STATIC_CAST(nsResolveHostCallback *, node);
            node = node->next;
            callback->OnLookupComplete(this, rec, status);
            
        }
    }

    NS_RELEASE(rec);
}



void PR_CALLBACK
nsHostResolver::ThreadFunc(void *arg)
{
    LOG(("nsHostResolver::ThreadFunc entering\n"));
#if defined(RES_RETRY_ON_FAILURE)
    nsResState rs;
#endif

    nsHostResolver *resolver = (nsHostResolver *) arg;
    nsHostRecord *rec;
    PRAddrInfo *ai;
    while (resolver->GetHostToLookup(&rec)) {
        LOG(("resolving %s ...\n", rec->host));

        PRIntn flags = PR_AI_ADDRCONFIG;
        if (!(rec->flags & RES_CANON_NAME))
            flags |= PR_AI_NOCANONNAME;

        ai = PR_GetAddrInfoByName(rec->host, rec->af, flags);
#if defined(RES_RETRY_ON_FAILURE)
        if (!ai && rs.Reset())
            ai = PR_GetAddrInfoByName(rec->host, rec->af, flags);
#endif

        
        nsresult status = ai ? NS_OK : NS_ERROR_UNKNOWN_HOST;
        resolver->OnLookupComplete(rec, status, ai);
        LOG(("lookup complete for %s ...\n", rec->host));
    }
    NS_RELEASE(resolver);
    LOG(("nsHostResolver::ThreadFunc exiting\n"));
}



nsresult
nsHostResolver::Create(PRUint32         maxCacheEntries,
                       PRUint32         maxCacheLifetime,
                       nsHostResolver **result)
{
#if defined(PR_LOGGING)
    if (!gHostResolverLog)
        gHostResolverLog = PR_NewLogModule("nsHostResolver");
#endif

    nsHostResolver *res = new nsHostResolver(maxCacheEntries,
                                             maxCacheLifetime);
    if (!res)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(res);

    nsresult rv = res->Init();
    if (NS_FAILED(rv))
        NS_RELEASE(res);

    *result = res;
    return rv;
}
