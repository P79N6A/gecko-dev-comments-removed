




































#include "nsDNSService2.h"
#include "nsIDNSRecord.h"
#include "nsIDNSListener.h"
#include "nsICancelable.h"
#include "nsIProxyObjectManager.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIServiceManager.h"
#include "nsReadableUtils.h"
#include "nsString.h"
#include "nsAutoLock.h"
#include "nsAutoPtr.h"
#include "nsNetCID.h"
#include "nsNetError.h"
#include "prsystem.h"
#include "prnetdb.h"
#include "prmon.h"
#include "prio.h"
#include "plstr.h"

static const char kPrefDnsCacheEntries[]    = "network.dnsCacheEntries";
static const char kPrefDnsCacheExpiration[] = "network.dnsCacheExpiration";
static const char kPrefEnableIDN[]          = "network.enableIDN";
static const char kPrefIPv4OnlyDomains[]    = "network.dns.ipv4OnlyDomains";
static const char kPrefDisableIPv6[]        = "network.dns.disableIPv6";



class nsDNSRecord : public nsIDNSRecord
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDNSRECORD

    nsDNSRecord(nsHostRecord *hostRecord)
        : mHostRecord(hostRecord)
        , mIter(nsnull)
        , mDone(PR_FALSE) {}

private:
    virtual ~nsDNSRecord() {}

    nsRefPtr<nsHostRecord>  mHostRecord;
    void                   *mIter;
    PRBool                  mDone;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSRecord, nsIDNSRecord)

NS_IMETHODIMP
nsDNSRecord::GetCanonicalName(nsACString &result)
{
    
    NS_ENSURE_TRUE(mHostRecord->flags & nsHostResolver::RES_CANON_NAME,
                   NS_ERROR_NOT_AVAILABLE);

    
    
    const char *cname;
    if (mHostRecord->addr_info)
        cname = PR_GetCanonNameFromAddrInfo(mHostRecord->addr_info);
    else
        cname = mHostRecord->host;
    result.Assign(cname);
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRecord::GetNextAddr(PRUint16 port, PRNetAddr *addr)
{
    
    
    
    if (mDone)
        return NS_ERROR_NOT_AVAILABLE;

    if (mHostRecord->addr_info) {
        mIter = PR_EnumerateAddrInfo(mIter, mHostRecord->addr_info, port, addr);
        if (!mIter)
            return NS_ERROR_NOT_AVAILABLE;
    }
    else {
        
        NS_ENSURE_STATE(mHostRecord->addr);

        mIter = nsnull; 
        memcpy(addr, mHostRecord->addr, sizeof(PRNetAddr));
        
        port = PR_htons(port);
        if (addr->raw.family == PR_AF_INET)
            addr->inet.port = port;
        else
            addr->ipv6.port = port;
    }
        
    mDone = !mIter;
    return NS_OK; 
}

NS_IMETHODIMP
nsDNSRecord::GetNextAddrAsString(nsACString &result)
{
    PRNetAddr addr;
    nsresult rv = GetNextAddr(0, &addr);
    if (NS_FAILED(rv)) return rv;

    char buf[64];
    if (PR_NetAddrToString(&addr, buf, sizeof(buf)) == PR_SUCCESS) {
        result.Assign(buf);
        return NS_OK;
    }
    NS_ERROR("PR_NetAddrToString failed unexpectedly");
    return NS_ERROR_FAILURE; 
}

NS_IMETHODIMP
nsDNSRecord::HasMore(PRBool *result)
{
    if (mDone)
        *result = PR_FALSE;
    else {
        
        
        void *iterCopy = mIter;
        PRNetAddr addr;
        *result = NS_SUCCEEDED(GetNextAddr(0, &addr));
        mIter = iterCopy; 
        mDone = PR_FALSE;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDNSRecord::Rewind()
{
    mIter = nsnull;
    mDone = PR_FALSE;
    return NS_OK;
}



class nsDNSAsyncRequest : public nsResolveHostCallback
                        , public nsICancelable
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICANCELABLE

    nsDNSAsyncRequest(nsHostResolver   *res,
                      const nsACString &host,
                      nsIDNSListener   *listener,
                      PRUint16          flags,
                      PRUint16          af)
        : mResolver(res)
        , mHost(host)
        , mListener(listener)
        , mFlags(flags)
        , mAF(af) {}
    ~nsDNSAsyncRequest() {}

    void OnLookupComplete(nsHostResolver *, nsHostRecord *, nsresult);

    nsRefPtr<nsHostResolver> mResolver;
    nsCString                mHost; 
    nsCOMPtr<nsIDNSListener> mListener;
    PRUint16                 mFlags;
    PRUint16                 mAF;
};

void
nsDNSAsyncRequest::OnLookupComplete(nsHostResolver *resolver,
                                    nsHostRecord   *hostRecord,
                                    nsresult        status)
{
    
    
    
    nsCOMPtr<nsIDNSRecord> rec;
    if (NS_SUCCEEDED(status)) {
        NS_ASSERTION(hostRecord, "no host record");
        rec = new nsDNSRecord(hostRecord);
        if (!rec)
            status = NS_ERROR_OUT_OF_MEMORY;
    }

    mListener->OnLookupComplete(this, rec, status);
    mListener = nsnull;

    
    
    NS_RELEASE_THIS();
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDNSAsyncRequest, nsICancelable)

NS_IMETHODIMP
nsDNSAsyncRequest::Cancel(nsresult reason)
{
    NS_ENSURE_ARG(NS_FAILED(reason));
    mResolver->DetachCallback(mHost.get(), mFlags, mAF, this, reason);
    return NS_OK;
}



class nsDNSSyncRequest : public nsResolveHostCallback
{
public:
    nsDNSSyncRequest(PRMonitor *mon)
        : mDone(PR_FALSE)
        , mStatus(NS_OK)
        , mMonitor(mon) {}
    virtual ~nsDNSSyncRequest() {}

    void OnLookupComplete(nsHostResolver *, nsHostRecord *, nsresult);

    PRBool                 mDone;
    nsresult               mStatus;
    nsRefPtr<nsHostRecord> mHostRecord;

private:
    PRMonitor             *mMonitor;
};

void
nsDNSSyncRequest::OnLookupComplete(nsHostResolver *resolver,
                                   nsHostRecord   *hostRecord,
                                   nsresult        status)
{
    
    PR_EnterMonitor(mMonitor);
    mDone = PR_TRUE;
    mStatus = status;
    mHostRecord = hostRecord;
    PR_Notify(mMonitor);
    PR_ExitMonitor(mMonitor);
}



nsDNSService::nsDNSService()
    : mLock(nsnull)
{
}

nsDNSService::~nsDNSService()
{
    if (mLock)
        PR_DestroyLock(mLock);
}

NS_IMPL_THREADSAFE_ISUPPORTS3(nsDNSService, nsIDNSService, nsPIDNSService,
                              nsIObserver)

NS_IMETHODIMP
nsDNSService::Init()
{
    NS_ENSURE_TRUE(!mResolver, NS_ERROR_ALREADY_INITIALIZED);

    PRBool firstTime = (mLock == nsnull);

    
    PRUint32 maxCacheEntries  = 20;
    PRUint32 maxCacheLifetime = 1; 
    PRBool   enableIDN        = PR_TRUE;
    PRBool   disableIPv6      = PR_FALSE;
    nsAdoptingCString ipv4OnlyDomains;

    
    nsCOMPtr<nsIPrefBranch2> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        PRInt32 val;
        if (NS_SUCCEEDED(prefs->GetIntPref(kPrefDnsCacheEntries, &val)))
            maxCacheEntries = (PRUint32) val;
        if (NS_SUCCEEDED(prefs->GetIntPref(kPrefDnsCacheExpiration, &val)))
            maxCacheLifetime = val / 60; 

        
        prefs->GetBoolPref(kPrefEnableIDN, &enableIDN);
        prefs->GetBoolPref(kPrefDisableIPv6, &disableIPv6);
        prefs->GetCharPref(kPrefIPv4OnlyDomains, getter_Copies(ipv4OnlyDomains));
    }

    if (firstTime) {
        mLock = PR_NewLock();
        if (!mLock)
            return NS_ERROR_OUT_OF_MEMORY;

        
        if (prefs) {
            prefs->AddObserver(kPrefDnsCacheEntries, this, PR_FALSE);
            prefs->AddObserver(kPrefDnsCacheExpiration, this, PR_FALSE);
            prefs->AddObserver(kPrefEnableIDN, this, PR_FALSE);
            prefs->AddObserver(kPrefIPv4OnlyDomains, this, PR_FALSE);
            prefs->AddObserver(kPrefDisableIPv6, this, PR_FALSE);
        }
    }

    
    
    nsCOMPtr<nsIIDNService> idn;
    if (enableIDN)
        idn = do_GetService(NS_IDNSERVICE_CONTRACTID);

    nsRefPtr<nsHostResolver> res;
    nsresult rv = nsHostResolver::Create(maxCacheEntries,
                                         maxCacheLifetime,
                                         getter_AddRefs(res));
    if (NS_SUCCEEDED(rv)) {
        
        nsAutoLock lock(mLock);
        mResolver = res;
        mIDN = idn;
        mIPv4OnlyDomains = ipv4OnlyDomains; 
        mDisableIPv6 = disableIPv6;
    }

    return rv;
}

NS_IMETHODIMP
nsDNSService::Shutdown()
{
    nsRefPtr<nsHostResolver> res;
    {
        nsAutoLock lock(mLock);
        res = mResolver;
        mResolver = nsnull;
    }
    if (res)
        res->Shutdown();
    return NS_OK;
}

NS_IMETHODIMP
nsDNSService::AsyncResolve(const nsACString  &hostname,
                           PRUint32           flags,
                           nsIDNSListener    *listener,
                           nsIEventTarget    *target,
                           nsICancelable    **result)
{
    
    
    nsRefPtr<nsHostResolver> res;
    nsCOMPtr<nsIIDNService> idn;
    {
        nsAutoLock lock(mLock);
        res = mResolver;
        idn = mIDN;
    }
    NS_ENSURE_TRUE(res, NS_ERROR_OFFLINE);

    const nsACString *hostPtr = &hostname;

    nsresult rv;
    nsCAutoString hostACE;
    if (idn && !IsASCII(hostname)) {
        if (NS_SUCCEEDED(idn->ConvertUTF8toACE(hostname, hostACE)))
            hostPtr = &hostACE;
    }

    nsCOMPtr<nsIDNSListener> listenerProxy;
    if (target) {
        rv = NS_GetProxyForObject(target,
                                  NS_GET_IID(nsIDNSListener),
                                  listener,
                                  NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                  getter_AddRefs(listenerProxy));
        if (NS_FAILED(rv)) return rv;
        listener = listenerProxy;
    }

    PRUint16 af = GetAFForLookup(*hostPtr);

    nsDNSAsyncRequest *req =
            new nsDNSAsyncRequest(res, *hostPtr, listener, flags, af);
    if (!req)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*result = req);

    
    NS_ADDREF(req);
    rv = res->ResolveHost(req->mHost.get(), flags, af, req);
    if (NS_FAILED(rv)) {
        NS_RELEASE(req);
        NS_RELEASE(*result);
    }
    return rv;
}

NS_IMETHODIMP
nsDNSService::Resolve(const nsACString &hostname,
                      PRUint32          flags,
                      nsIDNSRecord    **result)
{
    
    
    nsRefPtr<nsHostResolver> res;
    nsCOMPtr<nsIIDNService> idn;
    {
        nsAutoLock lock(mLock);
        res = mResolver;
        idn = mIDN;
    }
    NS_ENSURE_TRUE(res, NS_ERROR_OFFLINE);

    const nsACString *hostPtr = &hostname;

    nsresult rv;
    nsCAutoString hostACE;
    if (idn && !IsASCII(hostname)) {
        if (NS_SUCCEEDED(idn->ConvertUTF8toACE(hostname, hostACE)))
            hostPtr = &hostACE;
    }

    
    
    
    
    
    
    
    
    PRMonitor *mon = PR_NewMonitor();
    if (!mon)
        return NS_ERROR_OUT_OF_MEMORY;

    PR_EnterMonitor(mon);
    nsDNSSyncRequest syncReq(mon);

    PRUint16 af = GetAFForLookup(*hostPtr);

    rv = res->ResolveHost(PromiseFlatCString(*hostPtr).get(), flags, af, &syncReq);
    if (NS_SUCCEEDED(rv)) {
        
        while (!syncReq.mDone)
            PR_Wait(mon, PR_INTERVAL_NO_TIMEOUT);

        if (NS_FAILED(syncReq.mStatus))
            rv = syncReq.mStatus;
        else {
            NS_ASSERTION(syncReq.mHostRecord, "no host record");
            nsDNSRecord *rec = new nsDNSRecord(syncReq.mHostRecord);
            if (!rec)
                rv = NS_ERROR_OUT_OF_MEMORY;
            else
                NS_ADDREF(*result = rec);
        }
    }

    PR_ExitMonitor(mon);
    PR_DestroyMonitor(mon);
    return rv;
}

NS_IMETHODIMP
nsDNSService::GetMyHostName(nsACString &result)
{
    char name[100];
    if (PR_GetSystemInfo(PR_SI_HOSTNAME, name, sizeof(name)) == PR_SUCCESS) {
        result = name;
        return NS_OK;
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDNSService::Observe(nsISupports *subject, const char *topic, const PRUnichar *data)
{
    
    NS_ASSERTION(strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0,
        "unexpected observe call");

    
    
    
    
    
    
    
    

    if (mResolver) {
        Shutdown();
        Init();
    }
    return NS_OK;
}

PRUint16
nsDNSService::GetAFForLookup(const nsACString &host)
{
    if (mDisableIPv6)
        return PR_AF_INET;

    nsAutoLock lock(mLock);

    PRUint16 af = PR_AF_UNSPEC;

    if (!mIPv4OnlyDomains.IsEmpty()) {
        const char *domain, *domainEnd, *end;
        PRUint32 hostLen, domainLen;

        
        domain = mIPv4OnlyDomains.BeginReading();
        domainEnd = mIPv4OnlyDomains.EndReading(); 

        nsACString::const_iterator hostStart;
        host.BeginReading(hostStart);
        hostLen = host.Length();

        do {
            
            while (*domain == ' ' || *domain == '\t')
                ++domain;

            
            end = strchr(domain, ',');
            if (!end)
                end = domainEnd;

            
            
            domainLen = end - domain;
            if (domainLen && hostLen >= domainLen) {
                const char *hostTail = hostStart.get() + hostLen - domainLen;
                if (PL_strncasecmp(domain, hostTail, domainLen) == 0) {
                    
                    
                    if (hostLen == domainLen ||
                            *hostTail == '.' || *(hostTail - 1) == '.') {
                        af = PR_AF_INET;
                        break;
                    }
                }
            }

            domain = end + 1;
        } while (*end);
    }

    return af;
}
