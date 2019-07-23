














































#include "nsHttp.h"
#include "nsHttpHandler.h"
#include "nsHttpChannel.h"
#include "nsHttpConnection.h"
#include "nsHttpResponseHead.h"
#include "nsHttpTransaction.h"
#include "nsHttpAuthCache.h"
#include "nsStandardURL.h"
#include "nsIHttpChannel.h"
#include "nsIURL.h"
#include "nsIStandardURL.h"
#include "nsICacheService.h"
#include "nsICategoryManager.h"
#include "nsCategoryManagerUtils.h"
#include "nsICacheService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefLocalizedString.h"
#include "nsISocketProviderService.h"
#include "nsISocketProvider.h"
#include "nsPrintfCString.h"
#include "nsCOMPtr.h"
#include "nsNetCID.h"
#include "nsAutoLock.h"
#include "prprf.h"
#include "nsReadableUtils.h"
#include "nsQuickSort.h"
#include "nsNetUtil.h"
#include "nsIOService.h"

#include "nsIXULAppInfo.h"

#if defined(XP_UNIX) || defined(XP_BEOS)
#include <sys/utsname.h>
#endif

#if defined(XP_WIN)
#include <windows.h>
#endif

#if defined(XP_MACOSX)
#include <Carbon/Carbon.h>
#endif

#if defined(XP_OS2)
#define INCL_DOSMISC
#include <os2.h>
#endif


#ifdef MOZ_IPC
using namespace mozilla::net;
#endif 

#ifdef DEBUG

extern PRThread *gSocketThread;
#endif

static NS_DEFINE_CID(kIOServiceCID, NS_IOSERVICE_CID);
static NS_DEFINE_CID(kStreamConverterServiceCID, NS_STREAMCONVERTERSERVICE_CID);
static NS_DEFINE_CID(kCookieServiceCID, NS_COOKIESERVICE_CID);
static NS_DEFINE_CID(kCacheServiceCID, NS_CACHESERVICE_CID);
static NS_DEFINE_CID(kSocketProviderServiceCID, NS_SOCKETPROVIDERSERVICE_CID);

#define UA_PREF_PREFIX          "general.useragent."
#define UA_APPNAME              "Mozilla"
#define UA_APPVERSION           "5.0"
#define UA_APPSECURITY_FALLBACK "N"

#define HTTP_PREF_PREFIX        "network.http."
#define INTL_ACCEPT_LANGUAGES   "intl.accept_languages"
#define INTL_ACCEPT_CHARSET     "intl.charset.default"
#define NETWORK_ENABLEIDN       "network.enableIDN"
#define BROWSER_PREF_PREFIX     "browser.cache."

#define UA_PREF(_pref) UA_PREF_PREFIX _pref
#define HTTP_PREF(_pref) HTTP_PREF_PREFIX _pref
#define BROWSER_PREF(_pref) BROWSER_PREF_PREFIX _pref



static nsresult
NewURI(const nsACString &aSpec,
       const char *aCharset,
       nsIURI *aBaseURI,
       PRInt32 aDefaultPort,
       nsIURI **aURI)
{
    nsStandardURL *url = new nsStandardURL();
    if (!url)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(url);

    nsresult rv = url->Init(nsIStandardURL::URLTYPE_AUTHORITY,
                            aDefaultPort, aSpec, aCharset, aBaseURI);
    if (NS_FAILED(rv)) {
        NS_RELEASE(url);
        return rv;
    }

    *aURI = url; 
    return NS_OK;
}





nsHttpHandler *gHttpHandler = nsnull;

nsHttpHandler::nsHttpHandler()
    : mConnMgr(nsnull)
    , mHttpVersion(NS_HTTP_VERSION_1_1)
    , mProxyHttpVersion(NS_HTTP_VERSION_1_1)
    , mCapabilities(NS_HTTP_ALLOW_KEEPALIVE)
    , mProxyCapabilities(NS_HTTP_ALLOW_KEEPALIVE)
    , mReferrerLevel(0xff) 
    , mIdleTimeout(10)
    , mMaxRequestAttempts(10)
    , mMaxRequestDelay(10)
    , mMaxConnections(24)
    , mMaxConnectionsPerServer(8)
    , mMaxPersistentConnectionsPerServer(2)
    , mMaxPersistentConnectionsPerProxy(4)
    , mMaxPipelinedRequests(2)
    , mRedirectionLimit(10)
    , mPhishyUserPassLength(1)
    , mPipeliningOverSSL(PR_FALSE)
    , mLastUniqueID(NowInSeconds())
    , mSessionStartTime(0)
    , mProduct("Gecko")
    , mUserAgentIsDirty(PR_TRUE)
    , mUseCache(PR_TRUE)
    , mPromptTempRedirect(PR_TRUE)
    , mSendSecureXSiteReferrer(PR_TRUE)
    , mEnablePersistentHttpsCaching(PR_FALSE)
{
#if defined(PR_LOGGING)
    gHttpLog = PR_NewLogModule("nsHttp");
#endif

    LOG(("Creating nsHttpHandler [this=%x].\n", this));

    NS_ASSERTION(!gHttpHandler, "HTTP handler already created!");
    gHttpHandler = this;
}

nsHttpHandler::~nsHttpHandler()
{
    
    

    LOG(("Deleting nsHttpHandler [this=%x]\n", this));

    
    if (mConnMgr) {
        mConnMgr->Shutdown();
        NS_RELEASE(mConnMgr);
    }

    nsHttp::DestroyAtomTable();

    gHttpHandler = nsnull;
}

nsresult
nsHttpHandler::Init()
{
    nsresult rv;

    LOG(("nsHttpHandler::Init\n"));

    rv = nsHttp::CreateAtomTable();
    if (NS_FAILED(rv))
        return rv;

    mIOService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
        NS_WARNING("unable to continue without io service");
        return rv;
    }

#ifdef MOZ_IPC
    if (IsNeckoChild() && !gNeckoChild)
        NeckoChild::InitNeckoChild();
#endif 

    InitUserAgentComponents();

    
    nsCOMPtr<nsIPrefBranch2> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefBranch) {
        prefBranch->AddObserver(HTTP_PREF_PREFIX, this, PR_TRUE);
        prefBranch->AddObserver(UA_PREF_PREFIX, this, PR_TRUE);
        prefBranch->AddObserver(INTL_ACCEPT_LANGUAGES, this, PR_TRUE); 
        prefBranch->AddObserver(INTL_ACCEPT_CHARSET, this, PR_TRUE);
        prefBranch->AddObserver(NETWORK_ENABLEIDN, this, PR_TRUE);
        prefBranch->AddObserver(BROWSER_PREF("disk_cache_ssl"), this, PR_TRUE);

        PrefsChanged(prefBranch, nsnull);
    }

    mMisc.AssignLiteral("rv:" MOZILLA_VERSION);

#if DEBUG
    
    LOG(("> app-name = %s\n", mAppName.get()));
    LOG(("> app-version = %s\n", mAppVersion.get()));
    LOG(("> platform = %s\n", mPlatform.get()));
    LOG(("> oscpu = %s\n", mOscpu.get()));
    LOG(("> security = %s\n", mSecurity.get()));
    LOG(("> language = %s\n", mLanguage.get()));
    LOG(("> misc = %s\n", mMisc.get()));
    LOG(("> vendor = %s\n", mVendor.get()));
    LOG(("> vendor-sub = %s\n", mVendorSub.get()));
    LOG(("> vendor-comment = %s\n", mVendorComment.get()));
    LOG(("> extra = %s\n", mExtraUA.get()));
    LOG(("> product = %s\n", mProduct.get()));
    LOG(("> product-sub = %s\n", mProductSub.get()));
    LOG(("> product-comment = %s\n", mProductComment.get()));
    LOG(("> user-agent = %s\n", UserAgent().get()));
#endif

    mSessionStartTime = NowInSeconds();

    rv = mAuthCache.Init();
    if (NS_FAILED(rv)) return rv;

    rv = InitConnectionMgr();
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<nsIXULAppInfo> appInfo =
        do_GetService("@mozilla.org/xre/app-info;1");
    if (appInfo)
        appInfo->GetPlatformBuildID(mProductSub);
    if (mProductSub.Length() > 8)
        mProductSub.SetLength(8);

    
    
    NS_CreateServicesFromCategory(NS_HTTP_STARTUP_CATEGORY,
                                  static_cast<nsISupports*>(static_cast<void*>(this)),
                                  NS_HTTP_STARTUP_TOPIC);    
    
    mObserverService = do_GetService("@mozilla.org/observer-service;1");
    if (mObserverService) {
        mObserverService->AddObserver(this, "profile-change-net-teardown", PR_TRUE);
        mObserverService->AddObserver(this, "profile-change-net-restore", PR_TRUE);
        mObserverService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);
        mObserverService->AddObserver(this, "net:clear-active-logins", PR_TRUE);
    }
 
    StartPruneDeadConnectionsTimer();
    return NS_OK;
}

nsresult
nsHttpHandler::InitConnectionMgr()
{
    nsresult rv;

    if (!mConnMgr) {
        mConnMgr = new nsHttpConnectionMgr();
        if (!mConnMgr)
            return NS_ERROR_OUT_OF_MEMORY;
        NS_ADDREF(mConnMgr);
    }

    rv = mConnMgr->Init(mMaxConnections,
                        mMaxConnectionsPerServer,
                        mMaxConnectionsPerServer,
                        mMaxPersistentConnectionsPerServer,
                        mMaxPersistentConnectionsPerProxy,
                        mMaxRequestDelay,
                        mMaxPipelinedRequests);
    return rv;
}

void
nsHttpHandler::StartPruneDeadConnectionsTimer()
{
    LOG(("nsHttpHandler::StartPruneDeadConnectionsTimer\n"));

    mTimer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ASSERTION(mTimer, "no timer");
    
    
    if (mTimer)
        mTimer->Init(this, 15*1000, 
                     nsITimer::TYPE_REPEATING_SLACK);
}

void
nsHttpHandler::StopPruneDeadConnectionsTimer()
{
    LOG(("nsHttpHandler::StopPruneDeadConnectionsTimer\n"));

    if (mTimer) {
        mTimer->Cancel();
        mTimer = 0;
    }
}

nsresult
nsHttpHandler::AddStandardRequestHeaders(nsHttpHeaderArray *request,
                                         PRUint8 caps,
                                         PRBool useProxy)
{
    nsresult rv;

    
    rv = request->SetHeader(nsHttp::User_Agent, UserAgent());
    if (NS_FAILED(rv)) return rv;

    
    
    rv = request->SetHeader(nsHttp::Accept, mAccept);
    if (NS_FAILED(rv)) return rv;

    
    if (!mAcceptLanguages.IsEmpty()) {
        
        rv = request->SetHeader(nsHttp::Accept_Language, mAcceptLanguages);
        if (NS_FAILED(rv)) return rv;
    }

    
    rv = request->SetHeader(nsHttp::Accept_Encoding, mAcceptEncodings);
    if (NS_FAILED(rv)) return rv;

    
    rv = request->SetHeader(nsHttp::Accept_Charset, mAcceptCharsets);
    if (NS_FAILED(rv)) return rv;

    
    
    
    
    
    
    
    
    
    NS_NAMED_LITERAL_CSTRING(close, "close");
    NS_NAMED_LITERAL_CSTRING(keepAlive, "keep-alive");

    const nsACString *connectionType = &close;
    if (caps & NS_HTTP_ALLOW_KEEPALIVE) {
        rv = request->SetHeader(nsHttp::Keep_Alive, nsPrintfCString("%u", mIdleTimeout));
        if (NS_FAILED(rv)) return rv;
        connectionType = &keepAlive;
    } else if (useProxy) {
        
        request->SetHeader(nsHttp::Connection, close);
    }

    const nsHttpAtom &header = useProxy ? nsHttp::Proxy_Connection
                                        : nsHttp::Connection;
    return request->SetHeader(header, *connectionType);
}

PRBool
nsHttpHandler::IsAcceptableEncoding(const char *enc)
{
    if (!enc)
        return PR_FALSE;

    
    
    
    
    if (!PL_strncasecmp(enc, "x-", 2))
        enc += 2;
    
    return nsHttp::FindToken(mAcceptEncodings.get(), enc, HTTP_LWS ",") != nsnull;
}

nsresult
nsHttpHandler::GetCacheSession(nsCacheStoragePolicy storagePolicy,
                               nsICacheSession **result)
{
    nsresult rv;

    
    if (!mUseCache)
        return NS_ERROR_NOT_AVAILABLE;

    
    
    
    
    nsCOMPtr<nsICacheService> serv = do_GetService(NS_CACHESERVICE_CONTRACTID,
                                                   &rv);
    if (NS_FAILED(rv)) return rv;

    const char *sessionName = "HTTP";
    switch (storagePolicy) {
    case nsICache::STORE_IN_MEMORY:
        sessionName = "HTTP-memory-only";
        break;
    case nsICache::STORE_OFFLINE:
        sessionName = "HTTP-offline";
        break;
    default:
        break;
    }

    nsCOMPtr<nsICacheSession> cacheSession;
    rv = serv->CreateSession(sessionName,
                             storagePolicy,
                             nsICache::STREAM_BASED,
                             getter_AddRefs(cacheSession));
    if (NS_FAILED(rv)) return rv;

    rv = cacheSession->SetDoomEntriesIfExpired(PR_FALSE);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(*result = cacheSession);

    return NS_OK;
}

nsresult
nsHttpHandler::GetStreamConverterService(nsIStreamConverterService **result)
{
    if (!mStreamConvSvc) {
        nsresult rv;
        mStreamConvSvc = do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    *result = mStreamConvSvc;
    NS_ADDREF(*result);
    return NS_OK;
}

nsICookieService *
nsHttpHandler::GetCookieService()
{
    if (!mCookieService)
        mCookieService = do_GetService(NS_COOKIESERVICE_CONTRACTID);
    return mCookieService;
}

nsresult 
nsHttpHandler::GetIOService(nsIIOService** result)
{
    NS_ADDREF(*result = mIOService);
    return NS_OK;
}


void
nsHttpHandler::NotifyObservers(nsIHttpChannel *chan, const char *event)
{
    LOG(("nsHttpHandler::NotifyObservers [chan=%x event=\"%s\"]\n", chan, event));
    if (mObserverService)
        mObserverService->NotifyObservers(chan, event, nsnull);
}

nsresult
nsHttpHandler::OnChannelRedirect(nsIChannel* oldChan, nsIChannel* newChan,
                                 PRUint32 flags)
{
    
    NS_ASSERTION(gIOService, "Must have an IO service at this point");
    nsresult rv = gIOService->OnChannelRedirect(oldChan, newChan, flags);
    if (NS_FAILED(rv))
        return rv;

    
    nsCOMPtr<nsIChannelEventSink> sink;
    NS_QueryNotificationCallbacks(oldChan, sink);
    if (sink)
        rv = sink->OnChannelRedirect(oldChan, newChan, flags);

    return rv;
}





const nsAFlatCString &
nsHttpHandler::UserAgent()
{
    if (mUserAgentOverride) {
        LOG(("using general.useragent.override : %s\n", mUserAgentOverride.get()));
        return mUserAgentOverride;
    }

    if (mUserAgentIsDirty) {
        BuildUserAgent();
        mUserAgentIsDirty = PR_FALSE;
    }

    return mUserAgent;
}

void
nsHttpHandler::BuildUserAgent()
{
    LOG(("nsHttpHandler::BuildUserAgent\n"));

    NS_ASSERTION(!mAppName.IsEmpty() &&
                 !mAppVersion.IsEmpty() &&
                 !mPlatform.IsEmpty() &&
                 !mSecurity.IsEmpty() &&
                 !mOscpu.IsEmpty(),
                 "HTTP cannot send practical requests without this much");

    
    
    mUserAgent.SetCapacity(mAppName.Length() + 
                           mAppVersion.Length() + 
                           mPlatform.Length() + 
                           mSecurity.Length() +
                           mOscpu.Length() +
                           mLanguage.Length() +
                           mMisc.Length() +
                           mProduct.Length() +
                           mProductSub.Length() +
                           mProductComment.Length() +
                           mVendor.Length() +
                           mVendorSub.Length() +
                           mVendorComment.Length() +
                           mExtraUA.Length() +
                           22);

    
    mUserAgent.Assign(mAppName);
    mUserAgent += '/';
    mUserAgent += mAppVersion;
    mUserAgent += ' ';

    
    mUserAgent += '(';
    mUserAgent += mPlatform;
    mUserAgent.AppendLiteral("; ");
    mUserAgent += mSecurity;
    mUserAgent.AppendLiteral("; ");
    mUserAgent += mOscpu;
    if (!mLanguage.IsEmpty()) {
        mUserAgent.AppendLiteral("; ");
        mUserAgent += mLanguage;
    }
    if (!mMisc.IsEmpty()) {
        mUserAgent.AppendLiteral("; ");
        mUserAgent += mMisc;
    }
    mUserAgent += ')';

    
    if (!mProduct.IsEmpty()) {
        mUserAgent += ' ';
        mUserAgent += mProduct;
        if (!mProductSub.IsEmpty()) {
            mUserAgent += '/';
            mUserAgent += mProductSub;
        }
        if (!mProductComment.IsEmpty()) {
            mUserAgent.AppendLiteral(" (");
            mUserAgent += mProductComment;
            mUserAgent += ')';
        }
    }

    
    if (!mVendor.IsEmpty()) {
        mUserAgent += ' ';
        mUserAgent += mVendor;
        if (!mVendorSub.IsEmpty()) {
            mUserAgent += '/';
            mUserAgent += mVendorSub;
        }
        if (!mVendorComment.IsEmpty()) {
            mUserAgent.AppendLiteral(" (");
            mUserAgent += mVendorComment;
            mUserAgent += ')';
        }
    }

    if (!mExtraUA.IsEmpty())
        mUserAgent += mExtraUA;
}

void
nsHttpHandler::InitUserAgentComponents()
{

      
    mPlatform.AssignLiteral(
#if defined(MOZ_WIDGET_PHOTON)
    "Photon"
#elif defined(XP_OS2)
    "OS/2"
#elif defined(XP_WIN)
    "Windows"
#elif defined(XP_MACOSX)
    "Macintosh"
#elif defined(XP_BEOS)
    "BeOS"
#elif defined(MOZ_X11)
    "X11"
#else
    "?"
#endif
    );

    
#if defined(XP_OS2)
    ULONG os2ver = 0;
    DosQuerySysInfo(QSV_VERSION_MINOR, QSV_VERSION_MINOR,
                    &os2ver, sizeof(os2ver));
    if (os2ver == 11)
        mOscpu.AssignLiteral("2.11");
    else if (os2ver == 30)
        mOscpu.AssignLiteral("Warp 3");
    else if (os2ver == 40)
        mOscpu.AssignLiteral("Warp 4");
    else if (os2ver == 45)
        mOscpu.AssignLiteral("Warp 4.5");

#elif defined(WINCE) || defined(XP_WIN)
    OSVERSIONINFO info = { sizeof(OSVERSIONINFO) };
    if (GetVersionEx(&info)) {
        char *buf = PR_smprintf(
#if defined(WINCE)
                                "WindowsCE %ld.%ld",
#else
                                "Windows NT %ld.%ld",
#endif
                                info.dwMajorVersion,
                                info.dwMinorVersion);
        if (buf) {
            mOscpu = buf;
            PR_smprintf_free(buf);
        }
    }
#elif defined (XP_MACOSX)
#if defined(__ppc__)
    mOscpu.AssignLiteral("PPC Mac OS X");
#elif defined(__i386__) || defined(__x86_64__)
    mOscpu.AssignLiteral("Intel Mac OS X");
#endif
    SInt32 majorVersion, minorVersion;
    if ((::Gestalt(gestaltSystemVersionMajor, &majorVersion) == noErr) &&
        (::Gestalt(gestaltSystemVersionMinor, &minorVersion) == noErr)) {
        mOscpu += nsPrintfCString(" %d.%d", majorVersion, minorVersion);
    }
#elif defined (XP_UNIX) || defined (XP_BEOS)
    struct utsname name;
    
    int ret = uname(&name);
    if (ret >= 0) {
        nsCAutoString buf;
        buf =  (char*)name.sysname;

        if (strcmp(name.machine, "x86_64") == 0 &&
            sizeof(void *) == sizeof(PRInt32)) {
            
            
            
            
            

            buf += " i686 (x86_64)";
        } else {
            buf += ' ';

#ifdef AIX
            
            
            
            buf += (char*)name.version;
            buf += '.';
            buf += (char*)name.release;
#else
            buf += (char*)name.machine;
#endif
        }

        mOscpu.Assign(buf);
    }
#endif

    mUserAgentIsDirty = PR_TRUE;
}

static int StringCompare(const void* s1, const void* s2, void*)
{
    return nsCRT::strcmp(*static_cast<const char *const *>(s1),
                         *static_cast<const char *const *>(s2));
}

void
nsHttpHandler::PrefsChanged(nsIPrefBranch *prefs, const char *pref)
{
    nsresult rv = NS_OK;
    PRInt32 val;

    LOG(("nsHttpHandler::PrefsChanged [pref=%s]\n", pref));

#define PREF_CHANGED(p) ((pref == nsnull) || !PL_strcmp(pref, p))
#define MULTI_PREF_CHANGED(p) \
  ((pref == nsnull) || !PL_strncmp(pref, p, sizeof(p) - 1))

    
    
    

    
    if (PREF_CHANGED(UA_PREF("appName"))) {
        prefs->GetCharPref(UA_PREF("appName"),
            getter_Copies(mAppName));
        if (mAppName.IsEmpty())
            mAppName.AssignLiteral(UA_APPNAME);
        mUserAgentIsDirty = PR_TRUE;
    }
    if (PREF_CHANGED(UA_PREF("appVersion"))) {
        prefs->GetCharPref(UA_PREF("appVersion"),
            getter_Copies(mAppVersion));
        if (mAppVersion.IsEmpty())
            mAppVersion.AssignLiteral(UA_APPVERSION);
        mUserAgentIsDirty = PR_TRUE;
    }

    
    if (PREF_CHANGED(UA_PREF("vendor"))) {
        prefs->GetCharPref(UA_PREF("vendor"),
            getter_Copies(mVendor));
        mUserAgentIsDirty = PR_TRUE;
    }
    if (PREF_CHANGED(UA_PREF("vendorSub"))) {
        prefs->GetCharPref(UA_PREF("vendorSub"),
            getter_Copies(mVendorSub));
        mUserAgentIsDirty = PR_TRUE;
    }
    if (PREF_CHANGED(UA_PREF("vendorComment"))) {
        prefs->GetCharPref(UA_PREF("vendorComment"),
            getter_Copies(mVendorComment));
        mUserAgentIsDirty = PR_TRUE;
    }

    if (MULTI_PREF_CHANGED(UA_PREF("extra."))) {
        mExtraUA.Truncate();

        
        nsCOMPtr<nsIPrefService> service =
            do_GetService(NS_PREFSERVICE_CONTRACTID);
        nsCOMPtr<nsIPrefBranch> branch;
        service->GetBranch(UA_PREF("extra."), getter_AddRefs(branch));
        if (branch) {
            PRUint32 extraCount;
            char **extraItems;
            rv = branch->GetChildList("", &extraCount, &extraItems);
            if (NS_SUCCEEDED(rv) && extraItems) {
                NS_QuickSort(extraItems, extraCount, sizeof(extraItems[0]),
                             StringCompare, nsnull);
                for (char **item = extraItems,
                      **item_end = extraItems + extraCount;
                     item < item_end; ++item) {
                    nsXPIDLCString valStr;
                    branch->GetCharPref(*item, getter_Copies(valStr));
                    if (!valStr.IsEmpty())
                        mExtraUA += NS_LITERAL_CSTRING(" ") + valStr;
                }
                NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(extraCount, extraItems);
            }
        }

        mUserAgentIsDirty = PR_TRUE;
    }

    
    if (PREF_CHANGED(UA_PREF("productComment"))) {
        prefs->GetCharPref(UA_PREF("productComment"),
            getter_Copies(mProductComment));
        mUserAgentIsDirty = PR_TRUE;
    }

    
    if (PREF_CHANGED(UA_PREF("security"))) {
        prefs->GetCharPref(UA_PREF("security"), getter_Copies(mSecurity));
        if (!mSecurity)
            mSecurity.AssignLiteral(UA_APPSECURITY_FALLBACK);
        mUserAgentIsDirty = PR_TRUE;
    }

    
    if (PREF_CHANGED(UA_PREF("locale"))) {
        nsCOMPtr<nsIPrefLocalizedString> pls;
        prefs->GetComplexValue(UA_PREF("locale"),
                               NS_GET_IID(nsIPrefLocalizedString),
                               getter_AddRefs(pls));
        if (pls) {
            nsXPIDLString uval;
            pls->ToString(getter_Copies(uval));
            if (uval)
                CopyUTF16toUTF8(uval, mLanguage);
        }
        else {
            nsXPIDLCString cval;
            rv = prefs->GetCharPref(UA_PREF("locale"), getter_Copies(cval));
            if (cval)
                mLanguage.Assign(cval);
        }

        mUserAgentIsDirty = PR_TRUE;
    }

    
    if (PREF_CHANGED(UA_PREF("override"))) {
        prefs->GetCharPref(UA_PREF("override"),
                            getter_Copies(mUserAgentOverride));
        mUserAgentIsDirty = PR_TRUE;
    }

    
    
    

    if (PREF_CHANGED(HTTP_PREF("keep-alive.timeout"))) {
        rv = prefs->GetIntPref(HTTP_PREF("keep-alive.timeout"), &val);
        if (NS_SUCCEEDED(rv))
            mIdleTimeout = (PRUint16) CLAMP(val, 1, 0xffff);
    }

    if (PREF_CHANGED(HTTP_PREF("request.max-attempts"))) {
        rv = prefs->GetIntPref(HTTP_PREF("request.max-attempts"), &val);
        if (NS_SUCCEEDED(rv))
            mMaxRequestAttempts = (PRUint16) CLAMP(val, 1, 0xffff);
    }

    if (PREF_CHANGED(HTTP_PREF("request.max-start-delay"))) {
        rv = prefs->GetIntPref(HTTP_PREF("request.max-start-delay"), &val);
        if (NS_SUCCEEDED(rv)) {
            mMaxRequestDelay = (PRUint16) CLAMP(val, 0, 0xffff);
            if (mConnMgr)
                mConnMgr->UpdateParam(nsHttpConnectionMgr::MAX_REQUEST_DELAY,
                                      mMaxRequestDelay);
        }
    }

    if (PREF_CHANGED(HTTP_PREF("max-connections"))) {
        rv = prefs->GetIntPref(HTTP_PREF("max-connections"), &val);
        if (NS_SUCCEEDED(rv)) {
            mMaxConnections = (PRUint16) CLAMP(val, 1, 0xffff);
            if (mConnMgr)
                mConnMgr->UpdateParam(nsHttpConnectionMgr::MAX_CONNECTIONS,
                                      mMaxConnections);
        }
    }

    if (PREF_CHANGED(HTTP_PREF("max-connections-per-server"))) {
        rv = prefs->GetIntPref(HTTP_PREF("max-connections-per-server"), &val);
        if (NS_SUCCEEDED(rv)) {
            mMaxConnectionsPerServer = (PRUint8) CLAMP(val, 1, 0xff);
            if (mConnMgr) {
                mConnMgr->UpdateParam(nsHttpConnectionMgr::MAX_CONNECTIONS_PER_HOST,
                                      mMaxConnectionsPerServer);
                mConnMgr->UpdateParam(nsHttpConnectionMgr::MAX_CONNECTIONS_PER_PROXY,
                                      mMaxConnectionsPerServer);
            }
        }
    }

    if (PREF_CHANGED(HTTP_PREF("max-persistent-connections-per-server"))) {
        rv = prefs->GetIntPref(HTTP_PREF("max-persistent-connections-per-server"), &val);
        if (NS_SUCCEEDED(rv)) {
            mMaxPersistentConnectionsPerServer = (PRUint8) CLAMP(val, 1, 0xff);
            if (mConnMgr)
                mConnMgr->UpdateParam(nsHttpConnectionMgr::MAX_PERSISTENT_CONNECTIONS_PER_HOST,
                                      mMaxPersistentConnectionsPerServer);
        }
    }

    if (PREF_CHANGED(HTTP_PREF("max-persistent-connections-per-proxy"))) {
        rv = prefs->GetIntPref(HTTP_PREF("max-persistent-connections-per-proxy"), &val);
        if (NS_SUCCEEDED(rv)) {
            mMaxPersistentConnectionsPerProxy = (PRUint8) CLAMP(val, 1, 0xff);
            if (mConnMgr)
                mConnMgr->UpdateParam(nsHttpConnectionMgr::MAX_PERSISTENT_CONNECTIONS_PER_PROXY,
                                      mMaxPersistentConnectionsPerProxy);
        }
    }

    if (PREF_CHANGED(HTTP_PREF("sendRefererHeader"))) {
        rv = prefs->GetIntPref(HTTP_PREF("sendRefererHeader"), &val);
        if (NS_SUCCEEDED(rv))
            mReferrerLevel = (PRUint8) CLAMP(val, 0, 0xff);
    }

    if (PREF_CHANGED(HTTP_PREF("redirection-limit"))) {
        rv = prefs->GetIntPref(HTTP_PREF("redirection-limit"), &val);
        if (NS_SUCCEEDED(rv))
            mRedirectionLimit = (PRUint8) CLAMP(val, 0, 0xff);
    }

    if (PREF_CHANGED(HTTP_PREF("version"))) {
        nsXPIDLCString httpVersion;
        prefs->GetCharPref(HTTP_PREF("version"), getter_Copies(httpVersion));
        if (httpVersion) {
            if (!PL_strcmp(httpVersion, "1.1"))
                mHttpVersion = NS_HTTP_VERSION_1_1;
            else if (!PL_strcmp(httpVersion, "0.9"))
                mHttpVersion = NS_HTTP_VERSION_0_9;
            else
                mHttpVersion = NS_HTTP_VERSION_1_0;
        }
    }

    if (PREF_CHANGED(HTTP_PREF("proxy.version"))) {
        nsXPIDLCString httpVersion;
        prefs->GetCharPref(HTTP_PREF("proxy.version"), getter_Copies(httpVersion));
        if (httpVersion) {
            if (!PL_strcmp(httpVersion, "1.1"))
                mProxyHttpVersion = NS_HTTP_VERSION_1_1;
            else
                mProxyHttpVersion = NS_HTTP_VERSION_1_0;
            
        }
    }

    PRBool cVar = PR_FALSE;

    if (PREF_CHANGED(HTTP_PREF("keep-alive"))) {
        rv = prefs->GetBoolPref(HTTP_PREF("keep-alive"), &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mCapabilities |= NS_HTTP_ALLOW_KEEPALIVE;
            else
                mCapabilities &= ~NS_HTTP_ALLOW_KEEPALIVE;
        }
    }

    if (PREF_CHANGED(HTTP_PREF("proxy.keep-alive"))) {
        rv = prefs->GetBoolPref(HTTP_PREF("proxy.keep-alive"), &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mProxyCapabilities |= NS_HTTP_ALLOW_KEEPALIVE;
            else
                mProxyCapabilities &= ~NS_HTTP_ALLOW_KEEPALIVE;
        }
    }

    if (PREF_CHANGED(HTTP_PREF("pipelining"))) {
        rv = prefs->GetBoolPref(HTTP_PREF("pipelining"), &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mCapabilities |=  NS_HTTP_ALLOW_PIPELINING;
            else
                mCapabilities &= ~NS_HTTP_ALLOW_PIPELINING;
        }
    }

    if (PREF_CHANGED(HTTP_PREF("pipelining.maxrequests"))) {
        rv = prefs->GetIntPref(HTTP_PREF("pipelining.maxrequests"), &val);
        if (NS_SUCCEEDED(rv)) {
            mMaxPipelinedRequests = CLAMP(val, 1, NS_HTTP_MAX_PIPELINED_REQUESTS);
            if (mConnMgr)
                mConnMgr->UpdateParam(nsHttpConnectionMgr::MAX_PIPELINED_REQUESTS,
                                      mMaxPipelinedRequests);
        }
    }

    if (PREF_CHANGED(HTTP_PREF("pipelining.ssl"))) {
        rv = prefs->GetBoolPref(HTTP_PREF("pipelining.ssl"), &cVar);
        if (NS_SUCCEEDED(rv))
            mPipeliningOverSSL = cVar;
    }

    if (PREF_CHANGED(HTTP_PREF("proxy.pipelining"))) {
        rv = prefs->GetBoolPref(HTTP_PREF("proxy.pipelining"), &cVar);
        if (NS_SUCCEEDED(rv)) {
            if (cVar)
                mProxyCapabilities |=  NS_HTTP_ALLOW_PIPELINING;
            else
                mProxyCapabilities &= ~NS_HTTP_ALLOW_PIPELINING;
        }
    }

    if (PREF_CHANGED(HTTP_PREF("sendSecureXSiteReferrer"))) {
        rv = prefs->GetBoolPref(HTTP_PREF("sendSecureXSiteReferrer"), &cVar);
        if (NS_SUCCEEDED(rv))
            mSendSecureXSiteReferrer = cVar;
    }

    if (PREF_CHANGED(HTTP_PREF("accept.default"))) {
        nsXPIDLCString accept;
        rv = prefs->GetCharPref(HTTP_PREF("accept.default"),
                                  getter_Copies(accept));
        if (NS_SUCCEEDED(rv))
            SetAccept(accept);
    }
    
    if (PREF_CHANGED(HTTP_PREF("accept-encoding"))) {
        nsXPIDLCString acceptEncodings;
        rv = prefs->GetCharPref(HTTP_PREF("accept-encoding"),
                                  getter_Copies(acceptEncodings));
        if (NS_SUCCEEDED(rv))
            SetAcceptEncodings(acceptEncodings);
    }

    if (PREF_CHANGED(HTTP_PREF("use-cache"))) {
        rv = prefs->GetBoolPref(HTTP_PREF("use-cache"), &cVar);
        if (NS_SUCCEEDED(rv)) {
            mUseCache = cVar;
        }
    }

    if (PREF_CHANGED(HTTP_PREF("default-socket-type"))) {
        nsXPIDLCString sval;
        rv = prefs->GetCharPref(HTTP_PREF("default-socket-type"),
                                getter_Copies(sval));
        if (NS_SUCCEEDED(rv)) {
            if (sval.IsEmpty())
                mDefaultSocketType.Adopt(0);
            else {
                
                nsCOMPtr<nsISocketProviderService> sps(
                        do_GetService(NS_SOCKETPROVIDERSERVICE_CONTRACTID));
                if (sps) {
                    nsCOMPtr<nsISocketProvider> sp;
                    rv = sps->GetSocketProvider(sval, getter_AddRefs(sp));
                    if (NS_SUCCEEDED(rv)) {
                        
                        mDefaultSocketType.Assign(sval);
                    }
                }
            }
        }
    }

    if (PREF_CHANGED(HTTP_PREF("prompt-temp-redirect"))) {
        rv = prefs->GetBoolPref(HTTP_PREF("prompt-temp-redirect"), &cVar);
        if (NS_SUCCEEDED(rv)) {
            mPromptTempRedirect = cVar;
        }
    }

    
    if (PREF_CHANGED(BROWSER_PREF("disk_cache_ssl"))) {
        cVar = PR_FALSE;
        rv = prefs->GetBoolPref(BROWSER_PREF("disk_cache_ssl"), &cVar);
        if (NS_SUCCEEDED(rv))
            mEnablePersistentHttpsCaching = cVar;
    }

    if (PREF_CHANGED(HTTP_PREF("phishy-userpass-length"))) {
        rv = prefs->GetIntPref(HTTP_PREF("phishy-userpass-length"), &val);
        if (NS_SUCCEEDED(rv))
            mPhishyUserPassLength = (PRUint8) CLAMP(val, 0, 0xff);
    }

    
    
    

    if (PREF_CHANGED(INTL_ACCEPT_LANGUAGES)) {
        nsCOMPtr<nsIPrefLocalizedString> pls;
        prefs->GetComplexValue(INTL_ACCEPT_LANGUAGES,
                                NS_GET_IID(nsIPrefLocalizedString),
                                getter_AddRefs(pls));
        if (pls) {
            nsXPIDLString uval;
            pls->ToString(getter_Copies(uval));
            if (uval)
                SetAcceptLanguages(NS_ConvertUTF16toUTF8(uval).get());
        } 
    }

    if (PREF_CHANGED(INTL_ACCEPT_CHARSET)) {
        nsCOMPtr<nsIPrefLocalizedString> pls;
        prefs->GetComplexValue(INTL_ACCEPT_CHARSET,
                                NS_GET_IID(nsIPrefLocalizedString),
                                getter_AddRefs(pls));
        if (pls) {
            nsXPIDLString uval;
            pls->ToString(getter_Copies(uval));
            if (uval)
                SetAcceptCharsets(NS_ConvertUTF16toUTF8(uval).get());
        } 
    }

    
    
    

    if (PREF_CHANGED(NETWORK_ENABLEIDN)) {
        PRBool enableIDN = PR_FALSE;
        prefs->GetBoolPref(NETWORK_ENABLEIDN, &enableIDN);
        
        
        
        if (enableIDN && !mIDNConverter) {
            mIDNConverter = do_GetService(NS_IDNSERVICE_CONTRACTID);
            NS_ASSERTION(mIDNConverter, "idnSDK not installed");
        }
        else if (!enableIDN && mIDNConverter)
            mIDNConverter = nsnull;
    }

#undef PREF_CHANGED
#undef MULTI_PREF_CHANGED
}














static nsresult
PrepareAcceptLanguages(const char *i_AcceptLanguages, nsACString &o_AcceptLanguages)
{
    if (!i_AcceptLanguages)
        return NS_OK;

    PRUint32 n, size, wrote;
    double q, dec;
    char *p, *p2, *token, *q_Accept, *o_Accept;
    const char *comma;
    PRInt32 available;

    o_Accept = nsCRT::strdup(i_AcceptLanguages);
    if (!o_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    for (p = o_Accept, n = size = 0; '\0' != *p; p++) {
        if (*p == ',') n++;
            size++;
    }

    available = size + ++n * 11 + 1;
    q_Accept = new char[available];
    if (!q_Accept) {
        nsCRT::free(o_Accept);
        return NS_ERROR_OUT_OF_MEMORY;
    }
    *q_Accept = '\0';
    q = 1.0;
    dec = q / (double) n;
    n = 0;
    p2 = q_Accept;
    for (token = nsCRT::strtok(o_Accept, ",", &p);
         token != (char *) 0;
         token = nsCRT::strtok(p, ",", &p))
    {
        token = net_FindCharNotInSet(token, HTTP_LWS);
        char* trim;
        trim = net_FindCharInSet(token, ";" HTTP_LWS);
        if (trim != (char*)0)  
            *trim = '\0';

        if (*token != '\0') {
            comma = n++ != 0 ? "," : ""; 
            PRUint32 u = QVAL_TO_UINT(q);
            if (u < 10)
                wrote = PR_snprintf(p2, available, "%s%s;q=0.%u", comma, token, u);
            else
                wrote = PR_snprintf(p2, available, "%s%s", comma, token);
            q -= dec;
            p2 += wrote;
            available -= wrote;
            NS_ASSERTION(available > 0, "allocated string not long enough");
        }
    }
    nsCRT::free(o_Accept);

    o_AcceptLanguages.Assign((const char *) q_Accept);
    delete [] q_Accept;

    return NS_OK;
}

nsresult
nsHttpHandler::SetAcceptLanguages(const char *aAcceptLanguages) 
{
    nsCAutoString buf;
    nsresult rv = PrepareAcceptLanguages(aAcceptLanguages, buf);
    if (NS_SUCCEEDED(rv))
        mAcceptLanguages.Assign(buf);
    return rv;
}















static nsresult
PrepareAcceptCharsets(const char *i_AcceptCharset, nsACString &o_AcceptCharset)
{
    PRUint32 n, size, wrote, u;
    PRInt32 available;
    double q, dec;
    char *p, *p2, *token, *q_Accept, *o_Accept;
    const char *acceptable, *comma;
    PRBool add_utf = PR_FALSE;
    PRBool add_asterisk = PR_FALSE;

    if (!i_AcceptCharset)
        acceptable = "";
    else
        acceptable = i_AcceptCharset;
    o_Accept = nsCRT::strdup(acceptable);
    if (nsnull == o_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    for (p = o_Accept, n = size = 0; '\0' != *p; p++) {
        if (*p == ',') n++;
            size++;
    }

    
    

    if (PL_strcasestr(acceptable, "utf-8") == NULL) {
        n++;
        add_utf = PR_TRUE;
    }
    if (PL_strchr(acceptable, '*') == NULL) {
        n++;
        add_asterisk = PR_TRUE;
    }

    available = size + ++n * 11 + 1;
    q_Accept = new char[available];
    if ((char *) 0 == q_Accept)
        return NS_ERROR_OUT_OF_MEMORY;
    *q_Accept = '\0';
    q = 1.0;
    dec = q / (double) n;
    n = 0;
    p2 = q_Accept;
    for (token = nsCRT::strtok(o_Accept, ",", &p);
         token != (char *) 0;
         token = nsCRT::strtok(p, ",", &p)) {
        token = net_FindCharNotInSet(token, HTTP_LWS);
        char* trim;
        trim = net_FindCharInSet(token, ";" HTTP_LWS);
        if (trim != (char*)0)  
            *trim = '\0';

        if (*token != '\0') {
            comma = n++ != 0 ? "," : ""; 
            u = QVAL_TO_UINT(q);
            if (u < 10)
                wrote = PR_snprintf(p2, available, "%s%s;q=0.%u", comma, token, u);
            else
                wrote = PR_snprintf(p2, available, "%s%s", comma, token);
            q -= dec;
            p2 += wrote;
            available -= wrote;
            NS_ASSERTION(available > 0, "allocated string not long enough");
        }
    }
    if (add_utf) {
        comma = n++ != 0 ? "," : ""; 
        u = QVAL_TO_UINT(q);
        if (u < 10)
            wrote = PR_snprintf(p2, available, "%sutf-8;q=0.%u", comma, u);
        else
            wrote = PR_snprintf(p2, available, "%sutf-8", comma);
        q -= dec;
        p2 += wrote;
        available -= wrote;
        NS_ASSERTION(available > 0, "allocated string not long enough");
    }
    if (add_asterisk) {
        comma = n++ != 0 ? "," : ""; 

        
        
        

        q += dec;
        u = QVAL_TO_UINT(q);
        if (u < 10)
            wrote = PR_snprintf(p2, available, "%s*;q=0.%u", comma, u);
        else
            wrote = PR_snprintf(p2, available, "%s*", comma);
        available -= wrote;
        p2 += wrote;
        NS_ASSERTION(available > 0, "allocated string not long enough");
    }
    nsCRT::free(o_Accept);

    
    o_AcceptCharset.Assign(q_Accept);
#if defined DEBUG_havill
    printf("Accept-Charset: %s\n", q_Accept);
#endif
    delete [] q_Accept;
    return NS_OK;
}

nsresult
nsHttpHandler::SetAcceptCharsets(const char *aAcceptCharsets) 
{
    nsCString buf;
    nsresult rv = PrepareAcceptCharsets(aAcceptCharsets, buf);
    if (NS_SUCCEEDED(rv))
        mAcceptCharsets.Assign(buf);
    return rv;
}

nsresult
nsHttpHandler::SetAccept(const char *aAccept) 
{
    mAccept = aAccept;
    return NS_OK;
}

nsresult
nsHttpHandler::SetAcceptEncodings(const char *aAcceptEncodings) 
{
    mAcceptEncodings = aAcceptEncodings;
    return NS_OK;
}





NS_IMPL_THREADSAFE_ISUPPORTS5(nsHttpHandler,
                              nsIHttpProtocolHandler,
                              nsIProxiedProtocolHandler,
                              nsIProtocolHandler,
                              nsIObserver,
                              nsISupportsWeakReference)





NS_IMETHODIMP
nsHttpHandler::GetScheme(nsACString &aScheme)
{
    aScheme.AssignLiteral("http");
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetDefaultPort(PRInt32 *result)
{
    *result = NS_HTTP_DEFAULT_PORT;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_STD | ALLOWS_PROXY | ALLOWS_PROXY_HTTP |
        URI_LOADABLE_BY_ANYONE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::NewURI(const nsACString &aSpec,
                      const char *aCharset,
                      nsIURI *aBaseURI,
                      nsIURI **aURI)
{
    LOG(("nsHttpHandler::NewURI\n"));
    return ::NewURI(aSpec, aCharset, aBaseURI, NS_HTTP_DEFAULT_PORT, aURI);
}

NS_IMETHODIMP
nsHttpHandler::NewChannel(nsIURI *uri, nsIChannel **result)
{
    LOG(("nsHttpHandler::NewChannel\n"));

    NS_ENSURE_ARG_POINTER(uri);
    NS_ENSURE_ARG_POINTER(result);

    PRBool isHttp = PR_FALSE, isHttps = PR_FALSE;

    
    nsresult rv = uri->SchemeIs("http", &isHttp);
    if (NS_FAILED(rv)) return rv;
    if (!isHttp) {
        rv = uri->SchemeIs("https", &isHttps);
        if (NS_FAILED(rv)) return rv;
        if (!isHttps) {
            NS_WARNING("Invalid URI scheme");
            return NS_ERROR_UNEXPECTED;
        }
    }
    
    return NewProxiedChannel(uri, nsnull, result);
}

NS_IMETHODIMP 
nsHttpHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}





NS_IMETHODIMP
nsHttpHandler::NewProxiedChannel(nsIURI *uri,
                                 nsIProxyInfo* givenProxyInfo,
                                 nsIChannel **result)
{
    nsHttpChannel *httpChannel = nsnull;

    LOG(("nsHttpHandler::NewProxiedChannel [proxyInfo=%p]\n",
        givenProxyInfo));
    
    nsCOMPtr<nsProxyInfo> proxyInfo;
    if (givenProxyInfo) {
        proxyInfo = do_QueryInterface(givenProxyInfo);
        NS_ENSURE_ARG(proxyInfo);
    }

    PRBool https;
    nsresult rv = uri->SchemeIs("https", &https);
    if (NS_FAILED(rv))
        return rv;

    NS_NEWXPCOM(httpChannel, nsHttpChannel);
    if (!httpChannel)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(httpChannel);

    
    
    PRInt8 caps;
    if (proxyInfo && !nsCRT::strcmp(proxyInfo->Type(), "http") && !https)
        caps = mProxyCapabilities;
    else
        caps = mCapabilities;

    if (https) {
        
        if (mPipeliningOverSSL)
            caps |= NS_HTTP_ALLOW_PIPELINING;

        
        nsCOMPtr<nsISocketProviderService> spserv =
                do_GetService(NS_SOCKETPROVIDERSERVICE_CONTRACTID);
        if (spserv) {
            nsCOMPtr<nsISocketProvider> provider;
            spserv->GetSocketProvider("ssl", getter_AddRefs(provider));
        }
    }

    rv = httpChannel->Init(uri, caps, proxyInfo);

    if (NS_FAILED(rv)) {
        NS_RELEASE(httpChannel);
        return rv;
    }

    *result = httpChannel;
    return NS_OK;
}





NS_IMETHODIMP
nsHttpHandler::GetUserAgent(nsACString &value)
{
    value = UserAgent();
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetAppName(nsACString &value)
{
    value = mAppName;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetAppVersion(nsACString &value)
{
    value = mAppVersion;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetVendor(nsACString &value)
{
    value = mVendor;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpHandler::SetVendor(const nsACString &value)
{
    mVendor = value;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetVendorSub(nsACString &value)
{
    value = mVendorSub;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpHandler::SetVendorSub(const nsACString &value)
{
    mVendorSub = value;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetVendorComment(nsACString &value)
{
    value = mVendorComment;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpHandler::SetVendorComment(const nsACString &value)
{
    mVendorComment = value;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProduct(nsACString &value)
{
    value = mProduct;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpHandler::SetProduct(const nsACString &value)
{
    mProduct = value;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProductSub(nsACString &value)
{
    value = mProductSub;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpHandler::SetProductSub(const nsACString &value)
{
    mProductSub = value;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetProductComment(nsACString &value)
{
    value = mProductComment;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpHandler::SetProductComment(const nsACString &value)
{
    mProductComment = value;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetPlatform(nsACString &value)
{
    value = mPlatform;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetOscpu(nsACString &value)
{
    value = mOscpu;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetLanguage(nsACString &value)
{
    value = mLanguage;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpHandler::SetLanguage(const nsACString &value)
{
    mLanguage = value;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpHandler::GetMisc(nsACString &value)
{
    value = mMisc;
    return NS_OK;
}
NS_IMETHODIMP
nsHttpHandler::SetMisc(const nsACString &value)
{
    mMisc = value;
    mUserAgentIsDirty = PR_TRUE;
    return NS_OK;
}





NS_IMETHODIMP
nsHttpHandler::Observe(nsISupports *subject,
                       const char *topic,
                       const PRUnichar *data)
{
    LOG(("nsHttpHandler::Observe [topic=\"%s\"]\n", topic));

    if (strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
        nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(subject);
        if (prefBranch)
            PrefsChanged(prefBranch, NS_ConvertUTF16toUTF8(data).get());
    }
    else if (strcmp(topic, "profile-change-net-teardown")    == 0 ||
             strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)    == 0) {

        
        StopPruneDeadConnectionsTimer();

        
        mAuthCache.ClearAll();

        
        if (mConnMgr)
            mConnMgr->Shutdown();

        
        
        mSessionStartTime = NowInSeconds();
    }
    else if (strcmp(topic, "profile-change-net-restore") == 0) {
        
        InitConnectionMgr();

        
        StartPruneDeadConnectionsTimer();
    }
    else if (strcmp(topic, "timer-callback") == 0) {
        
#ifdef DEBUG
        nsCOMPtr<nsITimer> timer = do_QueryInterface(subject);
        NS_ASSERTION(timer == mTimer, "unexpected timer-callback");
#endif
        if (mConnMgr)
            mConnMgr->PruneDeadConnections();
    }
    else if (strcmp(topic, "net:clear-active-logins") == 0) {
        mAuthCache.ClearAll();
    }

    return NS_OK;
}





NS_IMPL_THREADSAFE_ISUPPORTS4(nsHttpsHandler,
                              nsIHttpProtocolHandler,
                              nsIProxiedProtocolHandler,
                              nsIProtocolHandler,
                              nsISupportsWeakReference)

nsresult
nsHttpsHandler::Init()
{
    nsCOMPtr<nsIProtocolHandler> httpHandler(
            do_GetService(NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "http"));
    NS_ASSERTION(httpHandler.get() != nsnull, "no http handler?");
    return NS_OK;
}

NS_IMETHODIMP
nsHttpsHandler::GetScheme(nsACString &aScheme)
{
    aScheme.AssignLiteral("https");
    return NS_OK;
}

NS_IMETHODIMP
nsHttpsHandler::GetDefaultPort(PRInt32 *aPort)
{
    *aPort = NS_HTTPS_DEFAULT_PORT;
    return NS_OK;
}

NS_IMETHODIMP
nsHttpsHandler::GetProtocolFlags(PRUint32 *aProtocolFlags)
{
    return gHttpHandler->GetProtocolFlags(aProtocolFlags);
}

NS_IMETHODIMP
nsHttpsHandler::NewURI(const nsACString &aSpec,
                       const char *aOriginCharset,
                       nsIURI *aBaseURI,
                       nsIURI **_retval)
{
    return ::NewURI(aSpec, aOriginCharset, aBaseURI, NS_HTTPS_DEFAULT_PORT, _retval);
}

NS_IMETHODIMP
nsHttpsHandler::NewChannel(nsIURI *aURI, nsIChannel **_retval)
{
    return gHttpHandler->NewChannel(aURI, _retval);
}

NS_IMETHODIMP
nsHttpsHandler::AllowPort(PRInt32 aPort, const char *aScheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}
