




#if defined(HAVE_RES_NINIT)
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>   
#include <arpa/nameser.h>
#include <resolv.h>
#define RES_RETRY_ON_FAILURE
#endif

#include <stdlib.h>
#include <ctime>
#include "nsHostResolver.h"
#include "nsError.h"
#include "nsISupportsBase.h"
#include "nsISupportsUtils.h"
#include "nsAutoPtr.h"
#include "prthread.h"
#include "prerror.h"
#include "prtime.h"
#include "prlog.h"
#include "pldhash.h"
#include "plstr.h"
#include "nsURLHelper.h"
#include "nsThreadUtils.h"
#include "GetAddrInfo.h"

#include "mozilla/HashFunctions.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/Telemetry.h"
#include "mozilla/VisualEventTracer.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::net;



static const unsigned int NEGATIVE_RECORD_LIFETIME = 60;


















#define HighThreadThreshold     MAX_RESOLVER_THREADS_FOR_ANY_PRIORITY
#define LongIdleTimeoutSeconds  300           // for threads 1 -> HighThreadThreshold
#define ShortIdleTimeoutSeconds 60            // for threads HighThreadThreshold+1 -> MAX_RESOLVER_THREADS

PR_STATIC_ASSERT (HighThreadThreshold <= MAX_RESOLVER_THREADS);



#if defined(PR_LOGGING)
static PRLogModuleInfo *gHostResolverLog = nullptr;
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



#if defined(RES_RETRY_ON_FAILURE)








class nsResState
{
public:
    nsResState()
        
        
        
        
        
        
        
        : mLastReset(PR_IntervalNow())
    {
    }

    bool Reset()
    {
        
        if (PR_IntervalToSeconds(PR_IntervalNow() - mLastReset) < 1)
            return false;

        LOG(("Calling 'res_ninit'.\n"));

        mLastReset = PR_IntervalNow();
        return (res_ninit(&_res) == 0);
    }

private:
    PRIntervalTime mLastReset;
};

#endif 



static inline bool
IsHighPriority(uint16_t flags)
{
    return !(flags & (nsHostResolver::RES_PRIORITY_LOW | nsHostResolver::RES_PRIORITY_MEDIUM));
}

static inline bool
IsMediumPriority(uint16_t flags)
{
    return flags & nsHostResolver::RES_PRIORITY_MEDIUM;
}

static inline bool
IsLowPriority(uint16_t flags)
{
    return flags & nsHostResolver::RES_PRIORITY_LOW;
}





#define RES_KEY_FLAGS(_f) ((_f) & nsHostResolver::RES_CANON_NAME)

nsHostRecord::nsHostRecord(const nsHostKey *key)
    : addr_info_lock("nsHostRecord.addr_info_lock")
    , addr_info_gencnt(0)
    , addr_info(nullptr)
    , addr(nullptr)
    , negative(false)
    , resolving(false)
    , onQueue(false)
    , usingAnyThread(false)
    , mDoomed(false)
#if TTL_AVAILABLE
    , mGetTtl(false)
#endif
    , mBlacklistedCount(0)
    , mResolveAgain(false)
{
    host = ((char *) this) + sizeof(nsHostRecord);
    memcpy((char *) host, key->host, strlen(key->host) + 1);
    flags = key->flags;
    af = key->af;

    PR_INIT_CLIST(this);
    PR_INIT_CLIST(&callbacks);
}

nsresult
nsHostRecord::Create(const nsHostKey *key, nsHostRecord **result)
{
    size_t hostLen = strlen(key->host) + 1;
    size_t size = hostLen + sizeof(nsHostRecord);

    
    
    void *place = ::operator new(size);
    *result = new(place) nsHostRecord(key);
    NS_ADDREF(*result);

    MOZ_EVENT_TRACER_NAME_OBJECT(*result, key->host);

    return NS_OK;
}

void
nsHostRecord::SetExpiration(const mozilla::TimeStamp& now, unsigned int valid, unsigned int grace)
{
    mValidStart = now;
    mGraceStart = now + TimeDuration::FromSeconds(valid);
    mValidEnd = now + TimeDuration::FromSeconds(valid + grace);
}

void
nsHostRecord::CopyExpirationTimesAndFlagsFrom(const nsHostRecord *aFromHostRecord)
{
    
    
    mValidStart = aFromHostRecord->mValidStart;
    mValidEnd = aFromHostRecord->mValidEnd;
    mGraceStart = aFromHostRecord->mGraceStart;
    mDoomed = aFromHostRecord->mDoomed;
}

nsHostRecord::~nsHostRecord()
{
    Telemetry::Accumulate(Telemetry::DNS_BLACKLIST_COUNT, mBlacklistedCount);
    delete addr_info;
    delete addr;
}

bool
nsHostRecord::Blacklisted(NetAddr *aQuery)
{
    
    LOG(("Checking blacklist for host [%s], host record [%p].\n", host, this));

    
    if (!mBlacklistedItems.Length()) {
        return false;
    }

    char buf[kIPv6CStrBufSize];
    if (!NetAddrToString(aQuery, buf, sizeof(buf))) {
        return false;
    }
    nsDependentCString strQuery(buf);

    for (uint32_t i = 0; i < mBlacklistedItems.Length(); i++) {
        if (mBlacklistedItems.ElementAt(i).Equals(strQuery)) {
            LOG(("Address [%s] is blacklisted for host [%s].\n", buf, host));
            return true;
        }
    }

    return false;
}

void
nsHostRecord::ReportUnusable(NetAddr *aAddress)
{
    
    LOG(("Adding address to blacklist for host [%s], host record [%p].\n", host, this));

    ++mBlacklistedCount;

    if (negative)
        mDoomed = true;

    char buf[kIPv6CStrBufSize];
    if (NetAddrToString(aAddress, buf, sizeof(buf))) {
        LOG(("Successfully adding address [%s] to blacklist for host [%s].\n", buf, host));
        mBlacklistedItems.AppendElement(nsCString(buf));
    }
}

void
nsHostRecord::ResetBlacklist()
{
    
    LOG(("Resetting blacklist for host [%s], host record [%p].\n", host, this));
    mBlacklistedItems.Clear();
}

nsHostRecord::ExpirationStatus
nsHostRecord::CheckExpiration(const mozilla::TimeStamp& now) const {
    if (!mGraceStart.IsNull() && now >= mGraceStart
            && !mValidEnd.IsNull() && now < mValidEnd) {
        return nsHostRecord::EXP_GRACE;
    } else if (!mValidEnd.IsNull() && now < mValidEnd) {
        return nsHostRecord::EXP_VALID;
    }

    return nsHostRecord::EXP_EXPIRED;
}


bool
nsHostRecord::HasUsableResult(const mozilla::TimeStamp& now, uint16_t queryFlags) const
{
    if (mDoomed) {
        return false;
    }

    
    if (negative && IsHighPriority(queryFlags)) {
        return false;
    }

    if (CheckExpiration(now) == EXP_EXPIRED) {
        return false;
    }

    return addr_info || addr || negative;
}

static size_t
SizeOfResolveHostCallbackListExcludingHead(const PRCList *head,
                                           MallocSizeOf mallocSizeOf)
{
    size_t n = 0;
    PRCList *curr = head->next;
    while (curr != head) {
        nsResolveHostCallback *callback =
            static_cast<nsResolveHostCallback*>(curr);
        n += callback->SizeOfIncludingThis(mallocSizeOf);
        curr = curr->next;
    }
    return n;
}

size_t
nsHostRecord::SizeOfIncludingThis(MallocSizeOf mallocSizeOf) const
{
    size_t n = mallocSizeOf(this);

    
    
    
    

    n += SizeOfResolveHostCallbackListExcludingHead(&callbacks, mallocSizeOf);
    n += addr_info ? addr_info->SizeOfIncludingThis(mallocSizeOf) : 0;
    n += mallocSizeOf(addr);

    n += mBlacklistedItems.SizeOfExcludingThis(mallocSizeOf);
    for (size_t i = 0; i < mBlacklistedItems.Length(); i++) {
        n += mBlacklistedItems[i].SizeOfExcludingThisMustBeUnshared(mallocSizeOf);
    }
    return n;
}

nsHostRecord::DnsPriority
nsHostRecord::GetPriority(uint16_t aFlags)
{
    if (IsHighPriority(aFlags)){
        return nsHostRecord::DNS_PRIORITY_HIGH;
    } else if (IsMediumPriority(aFlags)) {
        return nsHostRecord::DNS_PRIORITY_MEDIUM;
    }

    return nsHostRecord::DNS_PRIORITY_LOW;
}



bool
nsHostRecord::RemoveOrRefresh()
{
    if (resolving) {
        if (!onQueue) {
            
            
            
            mResolveAgain = true;
        }
        
        
        return false;
    }
    
    return true;
}



struct nsHostDBEnt : PLDHashEntryHdr
{
    nsHostRecord *rec;
};

static PLDHashNumber
HostDB_HashKey(PLDHashTable *table, const void *key)
{
    const nsHostKey *hk = static_cast<const nsHostKey *>(key);
    return AddToHash(HashString(hk->host), RES_KEY_FLAGS(hk->flags), hk->af);
}

static bool
HostDB_MatchEntry(PLDHashTable *table,
                  const PLDHashEntryHdr *entry,
                  const void *key)
{
    const nsHostDBEnt *he = static_cast<const nsHostDBEnt *>(entry);
    const nsHostKey *hk = static_cast<const nsHostKey *>(key); 

    return !strcmp(he->rec->host, hk->host) &&
            RES_KEY_FLAGS (he->rec->flags) == RES_KEY_FLAGS(hk->flags) &&
            he->rec->af == hk->af;
}

static void
HostDB_MoveEntry(PLDHashTable *table,
                 const PLDHashEntryHdr *from,
                 PLDHashEntryHdr *to)
{
    static_cast<nsHostDBEnt *>(to)->rec =
            static_cast<const nsHostDBEnt *>(from)->rec;
}

static void
HostDB_ClearEntry(PLDHashTable *table,
                  PLDHashEntryHdr *entry)
{
    nsHostDBEnt *he = static_cast<nsHostDBEnt*>(entry);
    MOZ_ASSERT(he, "nsHostDBEnt is null!");

    nsHostRecord *hr = he->rec;
    MOZ_ASSERT(hr, "nsHostDBEnt has null host record!");

    LOG(("Clearing cache db entry for host [%s].\n", hr->host));
#if defined(DEBUG) && defined(PR_LOGGING)
    {
        MutexAutoLock lock(hr->addr_info_lock);
        if (!hr->addr_info) {
            LOG(("No address info for host [%s].\n", hr->host));
        } else {
            if (!hr->mValidEnd.IsNull()) {
                TimeDuration diff = hr->mValidEnd - TimeStamp::NowLoRes();
                LOG(("Record for [%s] expires in %f seconds.\n", hr->host,
                     diff.ToSeconds()));
            } else {
                LOG(("Record for [%s] not yet valid.\n", hr->host));
            }

            NetAddrElement *addrElement = nullptr;
            char buf[kIPv6CStrBufSize];
            do {
                if (!addrElement) {
                    addrElement = hr->addr_info->mAddresses.getFirst();
                } else {
                    addrElement = addrElement->getNext();
                }

                if (addrElement) {
                    NetAddrToString(&addrElement->mAddress, buf, sizeof(buf));
                    LOG(("  [%s]\n", buf));
                }
            }
            while (addrElement);
        }
    }
#endif
    NS_RELEASE(he->rec);
}

static void
HostDB_InitEntry(PLDHashEntryHdr *entry,
                 const void *key)
{
    nsHostDBEnt *he = static_cast<nsHostDBEnt *>(entry);
    nsHostRecord::Create(static_cast<const nsHostKey *>(key), &he->rec);
}

static const PLDHashTableOps gHostDB_ops =
{
    HostDB_HashKey,
    HostDB_MatchEntry,
    HostDB_MoveEntry,
    HostDB_ClearEntry,
    HostDB_InitEntry,
};

static PLDHashOperator
HostDB_RemoveEntry(PLDHashTable *table,
                   PLDHashEntryHdr *hdr,
                   uint32_t number,
                   void *arg)
{
    return PL_DHASH_REMOVE;
}

static PLDHashOperator
HostDB_PruneEntry(PLDHashTable *table,
                  PLDHashEntryHdr *hdr,
                  uint32_t number,
                  void *arg)
{
    nsHostDBEnt* ent = static_cast<nsHostDBEnt *>(hdr);
    
    if (ent->rec->RemoveOrRefresh()) {
        PR_REMOVE_LINK(ent->rec);
        return PL_DHASH_REMOVE;
    }
    return PL_DHASH_NEXT;
}



#if TTL_AVAILABLE
static const char kPrefGetTtl[] = "network.dns.get-ttl";
static bool sGetTtlEnabled = false;

static void DnsPrefChanged(const char* aPref, void* aClosure)
{
    MOZ_ASSERT(NS_IsMainThread(),
               "Should be getting pref changed notification on main thread!");

    if (strcmp(aPref, kPrefGetTtl) != 0) {
        LOG(("DnsPrefChanged ignoring pref \"%s\"", aPref));
        return;
    }

    auto self = static_cast<nsHostResolver*>(aClosure);
    MOZ_ASSERT(self);

    sGetTtlEnabled = Preferences::GetBool(kPrefGetTtl);
}
#endif

nsHostResolver::nsHostResolver(uint32_t maxCacheEntries,
                               uint32_t defaultCacheEntryLifetime,
                               uint32_t defaultGracePeriod)
    : mMaxCacheEntries(maxCacheEntries)
    , mDefaultCacheLifetime(defaultCacheEntryLifetime)
    , mDefaultGracePeriod(defaultGracePeriod)
    , mLock("nsHostResolver.mLock")
    , mIdleThreadCV(mLock, "nsHostResolver.mIdleThreadCV")
    , mNumIdleThreads(0)
    , mThreadCount(0)
    , mActiveAnyThreadCount(0)
    , mEvictionQSize(0)
    , mPendingCount(0)
    , mShutdown(true)
{
    mCreationTime = PR_Now();
    PR_INIT_CLIST(&mHighQ);
    PR_INIT_CLIST(&mMediumQ);
    PR_INIT_CLIST(&mLowQ);
    PR_INIT_CLIST(&mEvictionQ);

    mLongIdleTimeout  = PR_SecondsToInterval(LongIdleTimeoutSeconds);
    mShortIdleTimeout = PR_SecondsToInterval(ShortIdleTimeoutSeconds);
}

nsHostResolver::~nsHostResolver()
{
    PL_DHashTableFinish(&mDB);
}

nsresult
nsHostResolver::Init()
{
    if (NS_FAILED(GetAddrInfoInit())) {
        return NS_ERROR_FAILURE;
    }

    PL_DHashTableInit(&mDB, &gHostDB_ops, sizeof(nsHostDBEnt), 0);

    mShutdown = false;

#if TTL_AVAILABLE
    
    
    
    
    {
        DebugOnly<nsresult> rv = Preferences::RegisterCallbackAndCall(
            &DnsPrefChanged, kPrefGetTtl, this);
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                         "Could not register DNS TTL pref callback.");
    }
#endif

#if defined(HAVE_RES_NINIT)
    
    
    
    
    
    static int initCount = 0;
    if (initCount++ > 0) {
        LOG(("Calling 'res_ninit'.\n"));
        res_ninit(&_res);
    }
#endif
    return NS_OK;
}

void
nsHostResolver::ClearPendingQueue(PRCList *aPendingQ)
{
    
    if (!PR_CLIST_IS_EMPTY(aPendingQ)) {
        PRCList *node = aPendingQ->next;
        while (node != aPendingQ) {
            nsHostRecord *rec = static_cast<nsHostRecord *>(node);
            node = node->next;
            OnLookupComplete(rec, NS_ERROR_ABORT, nullptr);
        }
    }
}











void
nsHostResolver::FlushCache()
{
  MutexAutoLock lock(mLock);
  mEvictionQSize = 0;

  
  
  if (!PR_CLIST_IS_EMPTY(&mEvictionQ)) {
      PRCList *node = mEvictionQ.next;
      while (node != &mEvictionQ) {
          nsHostRecord *rec = static_cast<nsHostRecord *>(node);
          node = node->next;
          PR_REMOVE_AND_INIT_LINK(rec);
          PL_DHashTableRemove(&mDB, (nsHostKey *) rec);
          NS_RELEASE(rec);
      }
  }

  
  PL_DHashTableEnumerate(&mDB, HostDB_PruneEntry, nullptr);
}

void
nsHostResolver::Shutdown()
{
    LOG(("Shutting down host resolver.\n"));

#if TTL_AVAILABLE
    {
        DebugOnly<nsresult> rv = Preferences::UnregisterCallback(
            &DnsPrefChanged, kPrefGetTtl, this);
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                         "Could not unregister DNS TTL pref callback.");
    }
#endif

    PRCList pendingQHigh, pendingQMed, pendingQLow, evictionQ;
    PR_INIT_CLIST(&pendingQHigh);
    PR_INIT_CLIST(&pendingQMed);
    PR_INIT_CLIST(&pendingQLow);
    PR_INIT_CLIST(&evictionQ);

    {
        MutexAutoLock lock(mLock);

        mShutdown = true;

        MoveCList(mHighQ, pendingQHigh);
        MoveCList(mMediumQ, pendingQMed);
        MoveCList(mLowQ, pendingQLow);
        MoveCList(mEvictionQ, evictionQ);
        mEvictionQSize = 0;
        mPendingCount = 0;

        if (mNumIdleThreads)
            mIdleThreadCV.NotifyAll();

        
        PL_DHashTableEnumerate(&mDB, HostDB_RemoveEntry, nullptr);
    }

    ClearPendingQueue(&pendingQHigh);
    ClearPendingQueue(&pendingQMed);
    ClearPendingQueue(&pendingQLow);

    if (!PR_CLIST_IS_EMPTY(&evictionQ)) {
        PRCList *node = evictionQ.next;
        while (node != &evictionQ) {
            nsHostRecord *rec = static_cast<nsHostRecord *>(node);
            node = node->next;
            NS_RELEASE(rec);
        }
    }

#ifdef NS_BUILD_REFCNT_LOGGING
    
    
    
    
    
    
    

    PRIntervalTime delay = PR_MillisecondsToInterval(25);
    PRIntervalTime stopTime = PR_IntervalNow() + PR_SecondsToInterval(20);
    while (mThreadCount && PR_IntervalNow() < stopTime)
        PR_Sleep(delay);
#endif

    {
        mozilla::DebugOnly<nsresult> rv = GetAddrInfoShutdown();
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to shutdown GetAddrInfo");
    }
}

void 
nsHostResolver::MoveQueue(nsHostRecord *aRec, PRCList &aDestQ)
{
    NS_ASSERTION(aRec->onQueue, "Moving Host Record Not Currently Queued");
    
    PR_REMOVE_LINK(aRec);
    PR_APPEND_LINK(aRec, &aDestQ);
}

nsresult
nsHostResolver::ResolveHost(const char            *host,
                            uint16_t               flags,
                            uint16_t               af,
                            nsResolveHostCallback *callback)
{
    NS_ENSURE_TRUE(host && *host, NS_ERROR_UNEXPECTED);

    LOG(("Resolving host [%s]%s.\n",
         host, flags & RES_BYPASS_CACHE ? " - bypassing cache" : ""));

    
    
    if (!net_IsValidHostName(nsDependentCString(host)))
        return NS_ERROR_UNKNOWN_HOST;

    
    
    nsRefPtr<nsHostRecord> result;
    nsresult status = NS_OK, rv = NS_OK;
    {
        MutexAutoLock lock(mLock);

        if (mShutdown)
            rv = NS_ERROR_NOT_INITIALIZED;
        else {
            
            PRNetAddr tempAddr;
            
            
            memset(&tempAddr, 0, sizeof(PRNetAddr));
            
            
            
            
            
            
            

            nsHostKey key = { host, flags, af };
            nsHostDBEnt *he = static_cast<nsHostDBEnt *>
                                         (PL_DHashTableAdd(&mDB, &key));

            
            if (!he) {
                LOG(("  Out of memory: no cache entry for [%s].\n", host));
                rv = NS_ERROR_OUT_OF_MEMORY;
            }
            
            else if (!(flags & RES_BYPASS_CACHE) &&
                     he->rec->HasUsableResult(TimeStamp::NowLoRes(), flags)) {
                LOG(("  Using cached record for host [%s].\n", host));
                
                result = he->rec;
                Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2, METHOD_HIT);

                
                
                
                ConditionallyRefreshRecord(he->rec, host);
                
                if (he->rec->negative) {
                    LOG(("  Negative cache entry for [%s].\n", host));
                    Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                                          METHOD_NEGATIVE_HIT);
                    status = NS_ERROR_UNKNOWN_HOST;
                }
            }
            
            
            else if (he->rec->addr) {
                LOG(("  Using cached address for IP Literal [%s].\n", host));
                Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                                      METHOD_LITERAL);
                result = he->rec;
            }
            
            
            
            else if (PR_StringToNetAddr(host, &tempAddr) == PR_SUCCESS) {
                LOG(("  Host is IP Literal [%s].\n", host));
                
                
                he->rec->addr = new NetAddr();
                PRNetAddrToNetAddr(&tempAddr, he->rec->addr);
                
                Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                                      METHOD_LITERAL);
                result = he->rec;
            }
            else if (mPendingCount >= MAX_NON_PRIORITY_REQUESTS &&
                     !IsHighPriority(flags) &&
                     !he->rec->resolving) {
                LOG(("  Lookup queue full: dropping %s priority request for "
                     "[%s].\n",
                     IsMediumPriority(flags) ? "medium" : "low", host));
                Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                                      METHOD_OVERFLOW);
                
                rv = NS_ERROR_DNS_LOOKUP_QUEUE_FULL;
            }
            else if (flags & RES_OFFLINE) {
                LOG(("  Offline request for [%s]; ignoring.\n", host));
                rv = NS_ERROR_OFFLINE;
            }

            
            
            else if (!he->rec->resolving) {
                if (!(flags & RES_BYPASS_CACHE) &&
                    ((af == PR_AF_INET) || (af == PR_AF_INET6))) {
                    
                    const nsHostKey unspecKey = { host, flags, PR_AF_UNSPEC };
                    nsHostDBEnt *unspecHe = static_cast<nsHostDBEnt *>
                        (PL_DHashTableSearch(&mDB, &unspecKey));
                    NS_ASSERTION(!unspecHe ||
                                 (unspecHe && unspecHe->rec),
                                "Valid host entries should contain a record");
                    TimeStamp now = TimeStamp::NowLoRes();
                    if (unspecHe &&
                        unspecHe->rec->HasUsableResult(now, flags)) {

                        MOZ_ASSERT(unspecHe->rec->addr_info || unspecHe->rec->negative,
                                   "Entry should be resolved or negative.");

                        LOG(("  Trying AF_UNSPEC entry for [%s] af: %s.\n",
                            host, (af == PR_AF_INET) ? "AF_INET" : "AF_INET6"));

                        he->rec->addr_info = nullptr;
                        if (unspecHe->rec->negative) {
                            he->rec->negative = unspecHe->rec->negative;
                            he->rec->CopyExpirationTimesAndFlagsFrom(unspecHe->rec);
                        } else if (unspecHe->rec->addr_info) {
                            
                            
                            
                            NetAddrElement *addrIter =
                                unspecHe->rec->addr_info->mAddresses.getFirst();
                            while (addrIter) {
                                if ((af == addrIter->mAddress.inet.family) &&
                                     !unspecHe->rec->Blacklisted(&addrIter->mAddress)) {
                                    if (!he->rec->addr_info) {
                                        he->rec->addr_info = new AddrInfo(
                                            unspecHe->rec->addr_info->mHostName,
                                            unspecHe->rec->addr_info->mCanonicalName);
                                        he->rec->CopyExpirationTimesAndFlagsFrom(unspecHe->rec);
                                    }
                                    he->rec->addr_info->AddAddress(
                                        new NetAddrElement(*addrIter));
                                }
                                addrIter = addrIter->getNext();
                            }
                        }
                        
                        if (he->rec->HasUsableResult(now, flags)) {
                            result = he->rec;
                            if (he->rec->negative) {
                                status = NS_ERROR_UNKNOWN_HOST;
                            }
                            Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                                                  METHOD_HIT);
                            ConditionallyRefreshRecord(he->rec, host);
                        }
                        
                        
                        
                        
                        else if (af == PR_AF_INET6) {
                            LOG(("  No AF_INET6 in AF_UNSPEC entry: "
                                 "[%s] unknown host", host));
                            result = he->rec;
                            he->rec->negative = true;
                            status = NS_ERROR_UNKNOWN_HOST;
                            Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                                                  METHOD_NEGATIVE_HIT);
                        }
                    }
                }
                
                
                if (!result) {
                    LOG(("  No usable address in cache for [%s]", host));
                    
                    PR_APPEND_LINK(callback, &he->rec->callbacks);
                    he->rec->flags = flags;
                    rv = IssueLookup(he->rec);
                    Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                                          METHOD_NETWORK_FIRST);
                    if (NS_FAILED(rv)) {
                        PR_REMOVE_AND_INIT_LINK(callback);
                    }
                    else {
                        LOG(("  DNS lookup for host [%s] blocking pending "
                             "'getaddrinfo' query: callback [%p]",
                             host, callback));
                    }
                }
            }
            else {
                LOG(("  Host [%s] is being resolved. Appending callback [%p].",
                     host, callback));
                PR_APPEND_LINK(callback, &he->rec->callbacks);
                if (he->rec->onQueue) {
                    Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                                          METHOD_NETWORK_SHARED);

                    
                    
                    

                    if (IsHighPriority(flags) &&
                        !IsHighPriority(he->rec->flags)) {
                        
                        MoveQueue(he->rec, mHighQ);
                        he->rec->flags = flags;
                        ConditionallyCreateThread(he->rec);
                    } else if (IsMediumPriority(flags) &&
                               IsLowPriority(he->rec->flags)) {
                        
                        MoveQueue(he->rec, mMediumQ);
                        he->rec->flags = flags;
                        mIdleThreadCV.Notify();
                    }
                }
            }
        }
    }
    if (result) {
        callback->OnLookupComplete(this, result, status);
    }

    return rv;
}

void
nsHostResolver::DetachCallback(const char            *host,
                               uint16_t               flags,
                               uint16_t               af,
                               nsResolveHostCallback *callback,
                               nsresult               status)
{
    nsRefPtr<nsHostRecord> rec;
    {
        MutexAutoLock lock(mLock);

        nsHostKey key = { host, flags, af };
        nsHostDBEnt *he = static_cast<nsHostDBEnt *>
                                     (PL_DHashTableSearch(&mDB, &key));
        if (he) {
            
            
            PRCList *node = he->rec->callbacks.next;
            while (node != &he->rec->callbacks) {
                if (static_cast<nsResolveHostCallback *>(node) == callback) {
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
nsHostResolver::ConditionallyCreateThread(nsHostRecord *rec)
{
    if (mNumIdleThreads) {
        
        mIdleThreadCV.Notify();
    }
    else if ((mThreadCount < HighThreadThreshold) ||
             (IsHighPriority(rec->flags) && mThreadCount < MAX_RESOLVER_THREADS)) {
        
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
      LOG(("  Unable to find a thread for looking up host [%s].\n", rec->host));
#endif
    return NS_OK;
}

nsresult
nsHostResolver::IssueLookup(nsHostRecord *rec)
{
    MOZ_EVENT_TRACER_WAIT(rec, "net::dns::resolve");

    nsresult rv = NS_OK;
    NS_ASSERTION(!rec->resolving, "record is already being resolved");

    
    
    
    if (rec->next == rec)
        NS_ADDREF(rec);
    else {
        PR_REMOVE_LINK(rec);
        mEvictionQSize--;
    }
    
    switch (nsHostRecord::GetPriority(rec->flags)) {
        case nsHostRecord::DNS_PRIORITY_HIGH:
            PR_APPEND_LINK(rec, &mHighQ);
            break;

        case nsHostRecord::DNS_PRIORITY_MEDIUM:
            PR_APPEND_LINK(rec, &mMediumQ);
            break;

        case nsHostRecord::DNS_PRIORITY_LOW:
            PR_APPEND_LINK(rec, &mLowQ);
            break;
    }
    mPendingCount++;
    
    rec->resolving = true;
    rec->onQueue = true;

    rv = ConditionallyCreateThread(rec);
    
    LOG (("  DNS thread counters: total=%d any-live=%d idle=%d pending=%d\n",
          mThreadCount,
          mActiveAnyThreadCount,
          mNumIdleThreads,
          mPendingCount));

    return rv;
}

nsresult
nsHostResolver::ConditionallyRefreshRecord(nsHostRecord *rec, const char *host)
{
    if ((rec->CheckExpiration(TimeStamp::NowLoRes()) != nsHostRecord::EXP_VALID
            || rec->negative) && !rec->resolving) {
        LOG(("  Using %s cache entry for host [%s] but starting async renewal.",
            rec->negative ? "negative" :"positive", host));
        IssueLookup(rec);

        if (!rec->negative) {
            
            
            Telemetry::Accumulate(Telemetry::DNS_LOOKUP_METHOD2,
                METHOD_RENEWAL);
        }
    }
    return NS_OK;
}

void
nsHostResolver::DeQueue(PRCList &aQ, nsHostRecord **aResult)
{
    *aResult = static_cast<nsHostRecord *>(aQ.next);
    PR_REMOVE_AND_INIT_LINK(*aResult);
    mPendingCount--;
    (*aResult)->onQueue = false;
}

bool
nsHostResolver::GetHostToLookup(nsHostRecord **result)
{
    bool timedOut = false;
    PRIntervalTime epoch, now, timeout;
    
    MutexAutoLock lock(mLock);

    timeout = (mNumIdleThreads >= HighThreadThreshold) ? mShortIdleTimeout : mLongIdleTimeout;
    epoch = PR_IntervalNow();

    while (!mShutdown) {
        
        
#if TTL_AVAILABLE
        #define SET_GET_TTL(var, val) \
            (var)->mGetTtl = sGetTtlEnabled && (val)
#else
        #define SET_GET_TTL(var, val)
#endif

        if (!PR_CLIST_IS_EMPTY(&mHighQ)) {
            DeQueue (mHighQ, result);
            SET_GET_TTL(*result, false);
            return true;
        }

        if (mActiveAnyThreadCount < HighThreadThreshold) {
            if (!PR_CLIST_IS_EMPTY(&mMediumQ)) {
                DeQueue (mMediumQ, result);
                mActiveAnyThreadCount++;
                (*result)->usingAnyThread = true;
                SET_GET_TTL(*result, true);
                return true;
            }
            
            if (!PR_CLIST_IS_EMPTY(&mLowQ)) {
                DeQueue (mLowQ, result);
                mActiveAnyThreadCount++;
                (*result)->usingAnyThread = true;
                SET_GET_TTL(*result, true);
                return true;
            }
        }
        
        
        
        if (timedOut)
            break;

        
        
        
        
        
        mNumIdleThreads++;
        mIdleThreadCV.Wait(timeout);
        mNumIdleThreads--;
        
        now = PR_IntervalNow();
        
        if ((PRIntervalTime)(now - epoch) >= timeout)
            timedOut = true;
        else {
            
            
            
            timeout -= (PRIntervalTime)(now - epoch);
            epoch = now;
        }
    }
    
    
    mThreadCount--;
    return false;
}

void
nsHostResolver::PrepareRecordExpiration(nsHostRecord* rec) const
{
    MOZ_ASSERT(((bool)rec->addr_info) != rec->negative);
    if (!rec->addr_info) {
        rec->SetExpiration(TimeStamp::NowLoRes(),
                           NEGATIVE_RECORD_LIFETIME, 0);
        LOG(("Caching [%s] negative record for %u seconds.\n", rec->host,
             NEGATIVE_RECORD_LIFETIME));
        return;
    }

    unsigned int lifetime = mDefaultCacheLifetime;
    unsigned int grace = mDefaultGracePeriod;
#if TTL_AVAILABLE
    unsigned int ttl = mDefaultCacheLifetime;
    if (sGetTtlEnabled) {
        MutexAutoLock lock(rec->addr_info_lock);
        if (rec->addr_info && rec->addr_info->ttl != AddrInfo::NO_TTL_DATA) {
            ttl = rec->addr_info->ttl;
        }
        lifetime = ttl;
        grace = 0;
    }
#endif

    rec->SetExpiration(TimeStamp::NowLoRes(), lifetime, grace);
    LOG(("Caching [%s] record for %u seconds (grace %d).",
         rec->host, lifetime, grace));
}






nsHostResolver::LookupStatus
nsHostResolver::OnLookupComplete(nsHostRecord* rec, nsresult status, AddrInfo* result)
{
    
    
    PRCList cbs;
    PR_INIT_CLIST(&cbs);
    {
        MutexAutoLock lock(mLock);

        if (rec->mResolveAgain && (status != NS_ERROR_ABORT)) {
            rec->mResolveAgain = false;
            return LOOKUP_RESOLVEAGAIN;
        }

        
        MoveCList(rec->callbacks, cbs);

        
        
        AddrInfo  *old_addr_info;
        {
            MutexAutoLock lock(rec->addr_info_lock);
            old_addr_info = rec->addr_info;
            rec->addr_info = result;
            rec->addr_info_gencnt++;
        }
        delete old_addr_info;

        rec->negative = !rec->addr_info;
        PrepareRecordExpiration(rec);
        rec->resolving = false;
        
        if (rec->usingAnyThread) {
            mActiveAnyThreadCount--;
            rec->usingAnyThread = false;
        }

        if (!mShutdown) {
            
            PR_APPEND_LINK(rec, &mEvictionQ);
            NS_ADDREF(rec);
            if (mEvictionQSize < mMaxCacheEntries)
                mEvictionQSize++;
            else {
                
                nsHostRecord *head =
                    static_cast<nsHostRecord *>(PR_LIST_HEAD(&mEvictionQ));
                PR_REMOVE_AND_INIT_LINK(head);
                PL_DHashTableRemove(&mDB, (nsHostKey *) head);

                if (!head->negative) {
                    
                    TimeDuration age = TimeStamp::NowLoRes() - head->mValidStart;
                    Telemetry::Accumulate(Telemetry::DNS_CLEANUP_AGE,
                                          static_cast<uint32_t>(age.ToSeconds() / 60));
                }

                
                NS_RELEASE(head);
            }
#if TTL_AVAILABLE
            if (!rec->mGetTtl && !rec->resolving && sGetTtlEnabled) {
                LOG(("Issuing second async lookup for TTL for %s.", rec->host));
                rec->flags =
                  (rec->flags & ~RES_PRIORITY_MEDIUM) | RES_PRIORITY_LOW;
                DebugOnly<nsresult> rv = IssueLookup(rec);
                NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                                 "Could not issue second async lookup for TTL.");
            }
#endif
        }
    }

    MOZ_EVENT_TRACER_DONE(rec, "net::dns::resolve");

    if (!PR_CLIST_IS_EMPTY(&cbs)) {
        PRCList *node = cbs.next;
        while (node != &cbs) {
            nsResolveHostCallback *callback =
                    static_cast<nsResolveHostCallback *>(node);
            node = node->next;
            callback->OnLookupComplete(this, rec, status);
            
        }
    }

    NS_RELEASE(rec);

    return LOOKUP_OK;
}

void
nsHostResolver::CancelAsyncRequest(const char            *host,
                                   uint16_t               flags,
                                   uint16_t               af,
                                   nsIDNSListener        *aListener,
                                   nsresult               status)

{
    MutexAutoLock lock(mLock);

    
    nsHostKey key = { host, flags, af };
    nsHostDBEnt *he = static_cast<nsHostDBEnt *>
                      (PL_DHashTableSearch(&mDB, &key));
    if (he) {
        nsHostRecord* recPtr = nullptr;
        PRCList *node = he->rec->callbacks.next;
        
        
        while (node != &he->rec->callbacks) {
            nsResolveHostCallback *callback
                = static_cast<nsResolveHostCallback *>(node);
            if (callback && (callback->EqualsAsyncListener(aListener))) {
                
                PR_REMOVE_LINK(callback);
                recPtr = he->rec;
                callback->OnLookupComplete(this, recPtr, status);
                break;
            }
            node = node->next;
        }

        
        if (recPtr && PR_CLIST_IS_EMPTY(&recPtr->callbacks)) {
            PL_DHashTableRemove(&mDB, (nsHostKey *)recPtr);
            
            if (recPtr->next != recPtr) {
                PR_REMOVE_LINK(recPtr);
                NS_RELEASE(recPtr);
            }
        }
    }
}

static size_t
SizeOfHostDBEntExcludingThis(PLDHashEntryHdr* hdr, MallocSizeOf mallocSizeOf,
                             void*)
{
    nsHostDBEnt* ent = static_cast<nsHostDBEnt*>(hdr);
    return ent->rec->SizeOfIncludingThis(mallocSizeOf);
}

size_t
nsHostResolver::SizeOfIncludingThis(MallocSizeOf mallocSizeOf) const
{
    MutexAutoLock lock(mLock);

    size_t n = mallocSizeOf(this);
    n += PL_DHashTableSizeOfExcludingThis(&mDB, SizeOfHostDBEntExcludingThis,
                                          mallocSizeOf);

    
    
    
    

    return n;
}

void
nsHostResolver::ThreadFunc(void *arg)
{
    LOG(("DNS lookup thread - starting execution.\n"));

    static nsThreadPoolNaming naming;
    naming.SetThreadPoolName(NS_LITERAL_CSTRING("DNS Resolver"));

#if defined(RES_RETRY_ON_FAILURE)
    nsResState rs;
#endif
    nsHostResolver *resolver = (nsHostResolver *)arg;
    nsHostRecord *rec  = nullptr;
    AddrInfo *ai = nullptr;

    while (rec || resolver->GetHostToLookup(&rec)) {
        LOG(("DNS lookup thread - Calling getaddrinfo for host [%s].\n",
             rec->host));

        TimeStamp startTime = TimeStamp::Now();
        MOZ_EVENT_TRACER_EXEC(rec, "net::dns::resolve");

#if TTL_AVAILABLE
        bool getTtl = rec->mGetTtl;
#else
        bool getTtl = false;
#endif

        
        
        bool disableIPv4 = rec->af == PR_AF_INET6;
        uint16_t af = disableIPv4 ? PR_AF_UNSPEC : rec->af;
        nsresult status = GetAddrInfo(rec->host, af, rec->flags, &ai, getTtl);
#if defined(RES_RETRY_ON_FAILURE)
        if (NS_FAILED(status) && rs.Reset()) {
            status = GetAddrInfo(rec->host, af, rec->flags, &ai, getTtl);
        }
#endif

        TimeDuration elapsed = TimeStamp::Now() - startTime;
        uint32_t millis = static_cast<uint32_t>(elapsed.ToMilliseconds());

        if (NS_SUCCEEDED(status)) {
            Telemetry::ID histogramID;
            if (!rec->addr_info_gencnt) {
                
                histogramID = Telemetry::DNS_LOOKUP_TIME;
            } else if (!getTtl) {
                
                histogramID = Telemetry::DNS_RENEWAL_TIME;
            } else {
                
                histogramID = Telemetry::DNS_RENEWAL_TIME_FOR_TTL;
            }
            Telemetry::Accumulate(histogramID, millis);
        } else {
            Telemetry::Accumulate(Telemetry::DNS_FAILED_LOOKUP_TIME, millis);
        }

        
        LOG(("DNS lookup thread - lookup completed for host [%s]: %s.\n",
             rec->host, ai ? "success" : "failure: unknown host"));
        if (LOOKUP_RESOLVEAGAIN == resolver->OnLookupComplete(rec, status, ai)) {
            
            LOG(("DNS lookup thread - Re-resolving host [%s].\n",
                 rec->host));
        } else {
            rec = nullptr;
        }
    }
    NS_RELEASE(resolver);
    LOG(("DNS lookup thread - queue empty, thread finished.\n"));
}

nsresult
nsHostResolver::Create(uint32_t maxCacheEntries,
                       uint32_t defaultCacheEntryLifetime,
                       uint32_t defaultGracePeriod,
                       nsHostResolver **result)
{
#if defined(PR_LOGGING)
    if (!gHostResolverLog)
        gHostResolverLog = PR_NewLogModule("nsHostResolver");
#endif

    nsHostResolver *res = new nsHostResolver(maxCacheEntries, defaultCacheEntryLifetime,
                                             defaultGracePeriod);
    if (!res)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(res);

    nsresult rv = res->Init();
    if (NS_FAILED(rv))
        NS_RELEASE(res);

    *result = res;
    return rv;
}

PLDHashOperator
CacheEntryEnumerator(PLDHashTable *table, PLDHashEntryHdr *entry,
                     uint32_t number, void *arg)
{
    
    
    nsHostRecord *rec = static_cast<nsHostDBEnt*>(entry)->rec;
    MOZ_ASSERT(rec, "rec should never be null here!");
    if (!rec || !rec->addr_info || !rec->host) {
        return PL_DHASH_NEXT;
    }

    DNSCacheEntries info;
    info.hostname = rec->host;
    info.family = rec->af;
    info.expiration =
        (int64_t)(rec->mValidEnd - TimeStamp::NowLoRes()).ToSeconds();
    if (info.expiration <= 0) {
        
        return PL_DHASH_NEXT;
    }

    {
        MutexAutoLock lock(rec->addr_info_lock);

        NetAddr *addr = nullptr;
        NetAddrElement *addrElement = rec->addr_info->mAddresses.getFirst();
        if (addrElement) {
            addr = &addrElement->mAddress;
        }
        while (addr) {
            char buf[kIPv6CStrBufSize];
            if (NetAddrToString(addr, buf, sizeof(buf))) {
                info.hostaddr.AppendElement(buf);
            }
            addr = nullptr;
            addrElement = addrElement->getNext();
            if (addrElement) {
                addr = &addrElement->mAddress;
            }
        }
    }

    nsTArray<DNSCacheEntries> *args = static_cast<nsTArray<DNSCacheEntries> *>(arg);
    args->AppendElement(info);

    return PL_DHASH_NEXT;
}

void
nsHostResolver::GetDNSCacheEntries(nsTArray<DNSCacheEntries> *args)
{
    PL_DHashTableEnumerate(&mDB, CacheEntryEnumerator, args);
}
