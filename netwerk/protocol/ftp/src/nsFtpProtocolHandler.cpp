


















































#include "nsFtpProtocolHandler.h"
#include "nsFTPChannel.h"
#include "nsIURL.h"
#include "nsIStandardURL.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProgressEventSink.h"
#include "prlog.h"
#include "nsNetUtil.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIObserverService.h"
#include "nsEscape.h"



#if defined(PR_LOGGING)











PRLogModuleInfo* gFTPLog = nsnull;
#endif
#define LOG(args) PR_LOG(gFTPLog, PR_LOG_DEBUG, args)



#define IDLE_TIMEOUT_PREF     "network.ftp.idleConnectionTimeout"
#define IDLE_CONNECTION_LIMIT 8 /* TODO pref me */

static NS_DEFINE_CID(kStandardURLCID, NS_STANDARDURL_CID);
static NS_DEFINE_CID(kCacheServiceCID, NS_CACHESERVICE_CID);

nsFtpProtocolHandler *gFtpHandler = nsnull;



nsFtpProtocolHandler::nsFtpProtocolHandler()
    : mIdleTimeout(-1)
{
#if defined(PR_LOGGING)
    if (!gFTPLog)
        gFTPLog = PR_NewLogModule("nsFtp");
#endif
    LOG(("FTP:creating handler @%x\n", this));

    gFtpHandler = this;
}

nsFtpProtocolHandler::~nsFtpProtocolHandler()
{
    LOG(("FTP:destroying handler @%x\n", this));

    NS_ASSERTION(mRootConnectionList.Count() == 0, "why wasn't Observe called?");

    gFtpHandler = nsnull;
}

NS_IMPL_THREADSAFE_ISUPPORTS4(nsFtpProtocolHandler,
                              nsIProtocolHandler,
                              nsIProxiedProtocolHandler,
                              nsIObserver,
                              nsISupportsWeakReference)

nsresult
nsFtpProtocolHandler::Init()
{
    nsresult rv;

    if (mIdleTimeout == -1) {
        nsCOMPtr<nsIPrefBranch2> branch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;

        rv = branch->GetIntPref(IDLE_TIMEOUT_PREF, &mIdleTimeout);
        if (NS_FAILED(rv))
            mIdleTimeout = 5*60; 

        rv = branch->AddObserver(IDLE_TIMEOUT_PREF, this, PR_TRUE);
        if (NS_FAILED(rv)) return rv;
    }

    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1");
    if (observerService)
        observerService->AddObserver(this,
                                     "network:offline-about-to-go-offline",
                                     PR_TRUE);
    
    return NS_OK;
}

    



NS_IMETHODIMP
nsFtpProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("ftp");
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = 21; 
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_STD | ALLOWS_PROXY | ALLOWS_PROXY_HTTP |
        URI_LOADABLE_BY_ANYONE; 
    return NS_OK;
}

NS_IMETHODIMP
nsFtpProtocolHandler::NewURI(const nsACString &aSpec,
                             const char *aCharset,
                             nsIURI *aBaseURI,
                             nsIURI **result)
{
    nsCAutoString spec(aSpec);
    char *fwdPtr = spec.BeginWriting();

    

    PRInt32 len = NS_UnescapeURL(fwdPtr);

    
    
    spec.Truncate(len);

    
    if (spec.FindCharInSet(CRLF) >= 0 || spec.FindChar('\0') >= 0)
        return NS_ERROR_MALFORMED_URI;

    nsresult rv;
    nsCOMPtr<nsIStandardURL> url = do_CreateInstance(kStandardURLCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = url->Init(nsIStandardURL::URLTYPE_AUTHORITY, 21, aSpec, aCharset, aBaseURI);
    if (NS_FAILED(rv)) return rv;

    return CallQueryInterface(url, result);
}

NS_IMETHODIMP
nsFtpProtocolHandler::NewChannel(nsIURI* url, nsIChannel* *result)
{
    return NewProxiedChannel(url, nsnull, result);
}

NS_IMETHODIMP
nsFtpProtocolHandler::NewProxiedChannel(nsIURI* uri, nsIProxyInfo* proxyInfo,
                                        nsIChannel* *result)
{
    NS_ENSURE_ARG_POINTER(uri);
    nsFtpChannel *channel = new nsFtpChannel(uri, proxyInfo);
    if (!channel)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(channel);

    nsresult rv = channel->Init();
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        return rv;
    }
    
    *result = channel;
    return rv;
}

NS_IMETHODIMP 
nsFtpProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    *_retval = (port == 21 || port == 22);
    return NS_OK;
}



void
nsFtpProtocolHandler::Timeout(nsITimer *aTimer, void *aClosure)
{
    LOG(("FTP:timeout reached for %p\n", aClosure));

    PRBool found = gFtpHandler->mRootConnectionList.RemoveElement(aClosure);
    if (!found) {
        NS_ERROR("timerStruct not found");
        return;
    }

    timerStruct* s = (timerStruct*)aClosure;
    delete s;
}

nsresult
nsFtpProtocolHandler::RemoveConnection(nsIURI *aKey, nsFtpControlConnection* *_retval)
{
    NS_ASSERTION(_retval, "null pointer");
    NS_ASSERTION(aKey, "null pointer");
    
    *_retval = nsnull;

    nsCAutoString spec;
    aKey->GetPrePath(spec);
    
    LOG(("FTP:removing connection for %s\n", spec.get()));
   
    timerStruct* ts = nsnull;
    PRInt32 i;
    PRBool found = PR_FALSE;
    
    for (i=0;i<mRootConnectionList.Count();++i) {
        ts = (timerStruct*)mRootConnectionList[i];
        if (strcmp(spec.get(), ts->key) == 0) {
            found = PR_TRUE;
            mRootConnectionList.RemoveElementAt(i);
            break;
        }
    }

    if (!found)
        return NS_ERROR_FAILURE;

    
    *_retval = ts->conn;
    ts->conn = nsnull;
    delete ts;

    return NS_OK;
}

nsresult
nsFtpProtocolHandler::InsertConnection(nsIURI *aKey, nsFtpControlConnection *aConn)
{
    NS_ASSERTION(aConn, "null pointer");
    NS_ASSERTION(aKey, "null pointer");
    
    nsCAutoString spec;
    aKey->GetPrePath(spec);

    LOG(("FTP:inserting connection for %s\n", spec.get()));

    nsresult rv;
    nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    if (NS_FAILED(rv)) return rv;
    
    timerStruct* ts = new timerStruct();
    if (!ts)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = timer->InitWithFuncCallback(nsFtpProtocolHandler::Timeout,
                                     ts,
                                     mIdleTimeout*1000,
                                     nsITimer::TYPE_REPEATING_SLACK);
    if (NS_FAILED(rv)) {
        delete ts;
        return rv;
    }
    
    ts->key = ToNewCString(spec);
    if (!ts->key) {
        delete ts;
        return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(aConn);
    ts->conn = aConn;
    ts->timer = timer;

    
    
    
    
    
    if (mRootConnectionList.Count() == IDLE_CONNECTION_LIMIT) {
        PRInt32 i;
        for (i=0;i<mRootConnectionList.Count();++i) {
            timerStruct *candidate = (timerStruct *) mRootConnectionList[i];
            if (strcmp(candidate->key, ts->key) == 0) {
                mRootConnectionList.RemoveElementAt(i);
                delete candidate;
                break;
            }
        }
        if (mRootConnectionList.Count() == IDLE_CONNECTION_LIMIT) {
            timerStruct *eldest = (timerStruct *) mRootConnectionList[0];
            mRootConnectionList.RemoveElementAt(0);
            delete eldest;
        }
    }

    mRootConnectionList.AppendElement(ts);
    return NS_OK;
}




NS_IMETHODIMP
nsFtpProtocolHandler::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
    LOG(("FTP:observing [%s]\n", aTopic));

    if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
        nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(aSubject);
        if (!branch) {
            NS_ERROR("no prefbranch");
            return NS_ERROR_UNEXPECTED;
        }
        PRInt32 timeout;
        nsresult rv = branch->GetIntPref(IDLE_TIMEOUT_PREF, &timeout);
        if (NS_SUCCEEDED(rv))
            mIdleTimeout = timeout;
    } else if (!strcmp(aTopic, "network:offline-about-to-go-offline")) {
        PRInt32 i;
        for (i=0;i<mRootConnectionList.Count();++i)
            delete (timerStruct*)mRootConnectionList[i];
        mRootConnectionList.Clear();
    } else {
        NS_NOTREACHED("unexpected topic");
    }

    return NS_OK;
}
