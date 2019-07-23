







































#include "nsCookieService.h"
#include "nsIServiceManager.h"

#include "nsIIOService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefService.h"
#include "nsICookieConsent.h"
#include "nsICookiePermission.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h" 
#include "nsIPrompt.h"
#include "nsITimer.h"
#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsILineInputStream.h"

#include "nsCOMArray.h"
#include "nsArrayEnumerator.h"
#include "nsAutoPtr.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "prtime.h"
#include "prprf.h"
#include "prnetdb.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsAppDirectoryServiceDefs.h"









static const char kHttpOnlyPrefix[] = "#HttpOnly_";

static const char kCookieFileName[] = "cookies.txt";

static const PRUint32 kLazyWriteTimeout = 5000; 

#undef  LIMIT
#define LIMIT(x, low, high, default) ((x) >= (low) && (x) <= (high) ? (x) : (default))



static const PRUint32 kMaxNumberOfCookies = 1000;
static const PRUint32 kMaxCookiesPerHost  = 50;
static const PRUint32 kMaxBytesPerCookie  = 4096;
static const PRUint32 kMaxBytesPerPath    = 1024;






static const nsCookieStatus STATUS_REJECTED_WITH_ERROR = 5;



#define USEC_PER_SEC   (nsInt64(1000000))
#define NOW_IN_SECONDS (nsInt64(PR_Now()) / USEC_PER_SEC)


static const PRUint32 BEHAVIOR_ACCEPT        = 0;
static const PRUint32 BEHAVIOR_REJECTFOREIGN = 1;
static const PRUint32 BEHAVIOR_REJECT        = 2;
static const PRUint32 BEHAVIOR_P3P           = 3;


static const char kPrefCookiesPermissions[] = "network.cookie.cookieBehavior";
static const char kPrefMaxNumberOfCookies[] = "network.cookie.maxNumber";
static const char kPrefMaxCookiesPerHost[]  = "network.cookie.maxPerHost";


struct nsCookieAttributes
{
  nsCAutoString name;
  nsCAutoString value;
  nsCAutoString host;
  nsCAutoString path;
  nsCAutoString expires;
  nsCAutoString maxage;
  nsInt64 expiryTime;
  PRBool isSession;
  PRBool isSecure;
  PRBool isHttpOnly;
};



struct nsListIter
{
  nsListIter() {}

  nsListIter(nsCookieEntry *aEntry)
   : entry(aEntry)
   , prev(nsnull)
   , current(aEntry ? aEntry->Head() : nsnull) {}

  nsListIter(nsCookieEntry *aEntry,
             nsCookie      *aPrev,
             nsCookie      *aCurrent)
   : entry(aEntry)
   , prev(aPrev)
   , current(aCurrent) {}

  nsListIter& operator++() { prev = current; current = current->Next(); return *this; }

  nsCookieEntry *entry;
  nsCookie      *prev;
  nsCookie      *current;
};



struct nsEnumerationData
{
  nsEnumerationData(nsInt64 aCurrentTime,
                    PRInt64 aOldestTime)
   : currentTime(aCurrentTime)
   , oldestTime(aOldestTime)
   , iter(nsnull, nsnull, nsnull) {}

  
  nsInt64 currentTime;

  
  
  nsInt64 oldestTime;

  
  nsListIter iter;
};







#ifdef MOZ_LOGGING







#define FORCE_PR_LOG
#include "prlog.h"
#endif


#define SET_COOKIE PR_TRUE
#define GET_COOKIE PR_FALSE

#ifdef PR_LOGGING
static PRLogModuleInfo *sCookieLog = PR_NewLogModule("cookie");

#define COOKIE_LOGFAILURE(a, b, c, d) LogFailure(a, b, c, d)
#define COOKIE_LOGSUCCESS(a, b, c, d) LogSuccess(a, b, c, d)

static void
LogFailure(PRBool aSetCookie, nsIURI *aHostURI, const char *aCookieString, const char *aReason)
{
  
  if (!PR_LOG_TEST(sCookieLog, PR_LOG_WARNING)) {
    return;
  }

  nsCAutoString spec;
  if (aHostURI)
    aHostURI->GetAsciiSpec(spec);

  PR_LOG(sCookieLog, PR_LOG_WARNING,
    ("%s%s%s\n", "===== ", aSetCookie ? "COOKIE NOT ACCEPTED" : "COOKIE NOT SENT", " ====="));
  PR_LOG(sCookieLog, PR_LOG_WARNING,("request URL: %s\n", spec.get()));
  if (aSetCookie) {
    PR_LOG(sCookieLog, PR_LOG_WARNING,("cookie string: %s\n", aCookieString));
  }

  PRExplodedTime explodedTime;
  PR_ExplodeTime(PR_Now(), PR_GMTParameters, &explodedTime);
  char timeString[40];
  PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);

  PR_LOG(sCookieLog, PR_LOG_WARNING,("current time: %s", timeString));
  PR_LOG(sCookieLog, PR_LOG_WARNING,("rejected because %s\n", aReason));
  PR_LOG(sCookieLog, PR_LOG_WARNING,("\n"));
}

static void
LogSuccess(PRBool aSetCookie, nsIURI *aHostURI, const char *aCookieString, nsCookie *aCookie)
{
  
  if (!PR_LOG_TEST(sCookieLog, PR_LOG_DEBUG)) {
    return;
  }

  nsCAutoString spec;
  aHostURI->GetAsciiSpec(spec);

  PR_LOG(sCookieLog, PR_LOG_DEBUG,
    ("%s%s%s\n", "===== ", aSetCookie ? "COOKIE ACCEPTED" : "COOKIE SENT", " ====="));
  PR_LOG(sCookieLog, PR_LOG_DEBUG,("request URL: %s\n", spec.get()));
  PR_LOG(sCookieLog, PR_LOG_DEBUG,("cookie string: %s\n", aCookieString));

  PRExplodedTime explodedTime;
  PR_ExplodeTime(PR_Now(), PR_GMTParameters, &explodedTime);
  char timeString[40];
  PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);

  PR_LOG(sCookieLog, PR_LOG_DEBUG,("current time: %s", timeString));

  if (aSetCookie) {
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("----------------\n"));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("name: %s\n", aCookie->Name().get()));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("value: %s\n", aCookie->Value().get()));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("%s: %s\n", aCookie->IsDomain() ? "domain" : "host", aCookie->Host().get()));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("path: %s\n", aCookie->Path().get()));

    if (!aCookie->IsSession()) {
      PR_ExplodeTime(aCookie->Expiry() * USEC_PER_SEC, PR_GMTParameters, &explodedTime);
      PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);
    }

    PR_LOG(sCookieLog, PR_LOG_DEBUG,
      ("expires: %s", aCookie->IsSession() ? "at end of session" : timeString));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("is secure: %s\n", aCookie->IsSecure() ? "true" : "false"));
  }
  PR_LOG(sCookieLog, PR_LOG_DEBUG,("\n"));
}


static inline void
LogFailure(PRBool aSetCookie, nsIURI *aHostURI, const nsAFlatCString &aCookieString, const char *aReason)
{
  LogFailure(aSetCookie, aHostURI, aCookieString.get(), aReason);
}

static inline void
LogSuccess(PRBool aSetCookie, nsIURI *aHostURI, const nsAFlatCString &aCookieString, nsCookie *aCookie)
{
  LogSuccess(aSetCookie, aHostURI, aCookieString.get(), aCookie);
}

#else
#define COOKIE_LOGFAILURE(a, b, c, d)
#define COOKIE_LOGSUCCESS(a, b, c, d)
#endif












PR_STATIC_CALLBACK(int)
compareCookiesForSending(const void *aElement1,
                         const void *aElement2,
                         void       *aData)
{
  const nsCookie *cookie1 = NS_STATIC_CAST(const nsCookie*, aElement1);
  const nsCookie *cookie2 = NS_STATIC_CAST(const nsCookie*, aElement2);

  
  int rv = cookie2->Path().Length() - cookie1->Path().Length();
  if (rv == 0) {
    
    
    
    
    rv = cookie1->CreationTime() - cookie2->CreationTime();
  }
  return rv;
}



PR_STATIC_CALLBACK(int)
compareCookiesForWriting(const void *aElement1,
                         const void *aElement2,
                         void       *aData)
{
  const nsCookie *cookie1 = NS_STATIC_CAST(const nsCookie*, aElement1);
  const nsCookie *cookie2 = NS_STATIC_CAST(const nsCookie*, aElement2);

  
  nsInt64 difference = cookie2->LastAccessed() - cookie1->LastAccessed();
  return (difference > nsInt64(0)) ? 1 : (difference < nsInt64(0)) ? -1 : 0;
}






nsCookieService *nsCookieService::gCookieService = nsnull;

nsCookieService*
nsCookieService::GetSingleton()
{
  if (gCookieService) {
    NS_ADDREF(gCookieService);
    return gCookieService;
  }

  
  
  
  
  
  
  gCookieService = new nsCookieService();
  if (gCookieService) {
    NS_ADDREF(gCookieService);
    if (NS_FAILED(gCookieService->Init())) {
      NS_RELEASE(gCookieService);
    }
  }

  return gCookieService;
}






NS_IMPL_ISUPPORTS6(nsCookieService,
                   nsICookieService,
                   nsICookieServiceInternal,
                   nsICookieManager,
                   nsICookieManager2,
                   nsIObserver,
                   nsISupportsWeakReference)

nsCookieService::nsCookieService()
 : mCookieCount(0)
 , mCookieChanged(PR_FALSE)
 , mCookieIconVisible(PR_FALSE)
 , mCookiesPermissions(BEHAVIOR_ACCEPT)
 , mMaxNumberOfCookies(kMaxNumberOfCookies)
 , mMaxCookiesPerHost(kMaxCookiesPerHost)
{
}

nsresult
nsCookieService::Init()
{
  if (!mHostTable.Init()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsCOMPtr<nsIPrefBranch2> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefBranch) {
    prefBranch->AddObserver(kPrefCookiesPermissions, this, PR_TRUE);
    prefBranch->AddObserver(kPrefMaxNumberOfCookies, this, PR_TRUE);
    prefBranch->AddObserver(kPrefMaxCookiesPerHost,  this, PR_TRUE);
    PrefChanged(prefBranch);
  }

  
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mCookieFile));
  if (mCookieFile) {
    mCookieFile->AppendNative(NS_LITERAL_CSTRING(kCookieFileName));
  }

  Read();

  mObserverService = do_GetService("@mozilla.org/observer-service;1");
  if (mObserverService) {
    mObserverService->AddObserver(this, "profile-before-change", PR_TRUE);
    mObserverService->AddObserver(this, "profile-do-change", PR_TRUE);
    mObserverService->AddObserver(this, "cookieIcon", PR_TRUE);
  }

  mPermissionService = do_GetService(NS_COOKIEPERMISSION_CONTRACTID);

  return NS_OK;
}

nsCookieService::~nsCookieService()
{
  gCookieService = nsnull;

  if (mWriteTimer)
    mWriteTimer->Cancel();
}

NS_IMETHODIMP
nsCookieService::Observe(nsISupports     *aSubject,
                         const char      *aTopic,
                         const PRUnichar *aData)
{
  
  if (!nsCRT::strcmp(aTopic, "profile-before-change")) {
    
    
    if (mWriteTimer) {
      mWriteTimer->Cancel();
      mWriteTimer = 0;
    }

    if (!nsCRT::strcmp(aData, NS_LITERAL_STRING("shutdown-cleanse").get())) {
      RemoveAllFromMemory();
      
      if (mCookieFile) {
        mCookieFile->Remove(PR_FALSE);
      }
    } else {
      Write();
      RemoveAllFromMemory();
    }

  } else if (!nsCRT::strcmp(aTopic, "profile-do-change")) {
    
    
    
    nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(mCookieFile));
    if (NS_SUCCEEDED(rv)) {
      mCookieFile->AppendNative(NS_LITERAL_CSTRING(kCookieFileName));
    }
    Read();

  } else if (!nsCRT::strcmp(aTopic, "cookieIcon")) {
    
    
    mCookieIconVisible = (aData[0] == 'o' && aData[1] == 'n' && aData[2] == '\0');

  } else if (!nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(aSubject);
    if (prefBranch)
      PrefChanged(prefBranch);
  }

  return NS_OK;
}


static inline PRBool ispathdelimiter(char c) { return c == '/' || c == '?' || c == '#' || c == ';'; }

void
nsCookieService::GetCookieList(nsIURI           *aHostURI,
                               nsIURI           *aFirstURI,
                               nsIChannel       *aChannel,
                               const nsACString *aName,
                               PRBool           aHttpBound,
                               nsAutoVoidArray  &aResult)
{
  if (!aHostURI) {
    COOKIE_LOGFAILURE(GET_COOKIE, nsnull, nsnull, "host URI is null");
    return;
  }

  
  nsCookiePolicy cookiePolicy; 
  nsCookieStatus cookieStatus = CheckPrefs(aHostURI, aFirstURI, aChannel, nsnull, cookiePolicy);
  
  switch (cookieStatus) {
  case nsICookie::STATUS_REJECTED:
  case STATUS_REJECTED_WITH_ERROR:
    return;
  }

  
  
  
  nsCAutoString hostFromURI, pathFromURI;
  if (NS_FAILED(aHostURI->GetAsciiHost(hostFromURI)) ||
      NS_FAILED(aHostURI->GetPath(pathFromURI))) {
    COOKIE_LOGFAILURE(GET_COOKIE, aHostURI, nsnull, "couldn't get host/path from URI");
    return;
  }
  
  hostFromURI.Trim(".");
  
  
  hostFromURI.Insert(NS_LITERAL_CSTRING("."), 0);
  ToLowerCase(hostFromURI);

  
  
  
  PRBool isSecure;
  if (NS_FAILED(aHostURI->SchemeIs("https", &isSecure))) {
    isSecure = PR_FALSE;
  }

  nsCookie *cookie;
  nsInt64 currentTime = NOW_IN_SECONDS;
  const char *currentDot = hostFromURI.get();
  const char *nextDot = currentDot + 1;

  
  
  do {
    nsCookieEntry *entry = mHostTable.GetEntry(currentDot);
    cookie = entry ? entry->Head() : nsnull;
    for (; cookie; cookie = cookie->Next()) {
      
      if (aName && !aName->Equals(cookie->Name())) {
        continue;
      }

      
      if (cookie->IsSecure() && !isSecure) {
        continue;
      }

      
      
      if (cookie->IsHttpOnly() && !aHttpBound) {
        continue;
      }

      
      PRUint32 cookiePathLen = cookie->Path().Length();
      if (cookiePathLen > 0 && cookie->Path().Last() == '/') {
        --cookiePathLen;
      }

      
      if (!StringBeginsWith(pathFromURI, Substring(cookie->Path(), 0, cookiePathLen))) {
        continue;
      }

      if (pathFromURI.Length() > cookiePathLen &&
          !ispathdelimiter(pathFromURI.CharAt(cookiePathLen))) {
        







        continue;
      }

      
      if (cookie->Expiry() <= currentTime) {
        continue;
      }

      
      aResult.AppendElement(cookie);
      cookie->SetLastAccessed(currentTime);
    }

    currentDot = nextDot;
    if (currentDot)
      nextDot = strchr(currentDot + 1, '.');

  } while (currentDot);

  
  
  
  aResult.Sort(compareCookiesForSending, nsnull);
}

NS_IMETHODIMP
nsCookieService::GetCookieValue(nsIURI *aHostURI,
				nsIChannel *aChannel,
                                const nsACString& aName,
				nsACString& aResult)
{
  aResult.Truncate();

  
  nsCOMPtr<nsIURI> firstURI;
  if (aChannel) {
    nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(aChannel);
    if (httpInternal)
      httpInternal->GetDocumentURI(getter_AddRefs(firstURI));
  }

  nsAutoVoidArray foundCookieList;
  GetCookieList(aHostURI, firstURI, aChannel, &aName, PR_FALSE,
                foundCookieList);

  if (!foundCookieList.Count())
    return NS_ERROR_NOT_AVAILABLE;

  nsCookie *cookie = NS_STATIC_CAST(nsCookie*, foundCookieList[0]);

  aResult.Assign(cookie->Value());
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::GetCookieString(nsIURI     *aHostURI,
                                 nsIChannel *aChannel,
                                 char       **aCookie)
{
  
  nsCOMPtr<nsIURI> firstURI;
  if (aChannel) {
    nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(aChannel);
    if (httpInternal)
      httpInternal->GetDocumentURI(getter_AddRefs(firstURI));
  }

  nsAutoVoidArray foundCookieList;
  GetCookieList(aHostURI, firstURI, aChannel, nsnull, PR_FALSE,
                foundCookieList);
  *aCookie = CookieStringFromArray(foundCookieList, aHostURI);
  
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::GetCookieStringFromHttp(nsIURI     *aHostURI,
                                         nsIURI     *aFirstURI,
                                         nsIChannel *aChannel,
                                         char       **aCookie)
{
  nsAutoVoidArray foundCookieList;
  GetCookieList(aHostURI, aFirstURI, aChannel, nsnull, PR_TRUE,
                foundCookieList);
  *aCookie = CookieStringFromArray(foundCookieList, aHostURI);

  return NS_OK;
}

char*
nsCookieService::CookieStringFromArray(const nsAutoVoidArray& aCookieList,
                                       nsIURI *aHostURI)
{
  nsCAutoString cookieData;
  PRInt32 count = aCookieList.Count();
  for (PRInt32 i = 0; i < count; ++i) {
    nsCookie *cookie = NS_STATIC_CAST(nsCookie*, aCookieList.ElementAt(i));

    
    if (!cookie->Name().IsEmpty() || !cookie->Value().IsEmpty()) {
      
      
      if (!cookieData.IsEmpty()) {
        cookieData.AppendLiteral("; ");
      }

      if (!cookie->Name().IsEmpty()) {
        
        cookieData += cookie->Name() + NS_LITERAL_CSTRING("=") + cookie->Value();
      } else {
        
        cookieData += cookie->Value();
      }
    }
  }

  
  
  if (!cookieData.IsEmpty()) {
    COOKIE_LOGSUCCESS(GET_COOKIE, aHostURI, cookieData, nsnull);
    return ToNewCString(cookieData);
  }
  
  return nsnull;
}

NS_IMETHODIMP
nsCookieService::SetCookieValue(nsIURI *aHostURI,
                                nsIChannel *aChannel,
                                const nsACString& aDomain,
                                const nsACString& aPath,
                                const nsACString& aName,
                                const nsACString& aValue,
                                PRBool aIsSession,
                                PRInt64 aExpiry)
{
  
  nsCOMPtr<nsIURI> firstURI;

  if (aChannel) {
    nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(aChannel);
    if (httpInternal)
      httpInternal->GetDocumentURI(getter_AddRefs(firstURI));
  }

  
  nsCookiePolicy cookiePolicy = nsICookie::POLICY_UNKNOWN;
  nsCookieStatus cookieStatus = CheckPrefs(aHostURI, firstURI, aChannel, "", cookiePolicy);
  
  switch (cookieStatus) {
  case nsICookie::STATUS_REJECTED:
    NotifyRejected(aHostURI);
  case STATUS_REJECTED_WITH_ERROR:
    return NS_OK;
  }

  nsCookieAttributes attributes;
  attributes.name = aName;
  attributes.value = aValue;
  attributes.host = aDomain;
  attributes.path = aPath;
  attributes.expiryTime = aExpiry;
  attributes.isSession = aIsSession;

  attributes.isSecure = PR_FALSE;
  aHostURI->SchemeIs("https", &attributes.isSecure);

  CheckAndAdd(aHostURI, aChannel, attributes,
              cookieStatus, cookiePolicy, EmptyCString());
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::SetCookieString(nsIURI     *aHostURI,
                                 nsIPrompt  *aPrompt,
                                 const char *aCookieHeader,
                                 nsIChannel *aChannel)
{
  
  nsCOMPtr<nsIURI> firstURI;

  if (aChannel) {
    nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(aChannel);
    if (httpInternal)
      httpInternal->GetDocumentURI(getter_AddRefs(firstURI));
  }

  return SetCookieStringFromHttp(aHostURI, firstURI, aPrompt, aCookieHeader, nsnull, aChannel);
}

NS_IMETHODIMP
nsCookieService::SetCookieStringFromHttp(nsIURI     *aHostURI,
                                         nsIURI     *aFirstURI,
                                         nsIPrompt  *aPrompt,
                                         const char *aCookieHeader,
                                         const char *aServerTime,
                                         nsIChannel *aChannel) 
{
  if (!aHostURI) {
    COOKIE_LOGFAILURE(SET_COOKIE, nsnull, aCookieHeader, "host URI is null");
    return NS_OK;
  }

  
  nsCookiePolicy cookiePolicy = nsICookie::POLICY_UNKNOWN;
  nsCookieStatus cookieStatus = CheckPrefs(aHostURI, aFirstURI, aChannel, aCookieHeader, cookiePolicy);
  
  switch (cookieStatus) {
  case nsICookie::STATUS_REJECTED:
    NotifyRejected(aHostURI);
  case STATUS_REJECTED_WITH_ERROR:
    return NS_OK;
  }

  
  
  
  
  
  nsInt64 serverTime;
  PRTime tempServerTime;
  if (aServerTime && PR_ParseTimeString(aServerTime, PR_TRUE, &tempServerTime) == PR_SUCCESS) {
    serverTime = nsInt64(tempServerTime) / USEC_PER_SEC;
  } else {
    serverTime = NOW_IN_SECONDS;
  }

  
  nsDependentCString cookieHeader(aCookieHeader);
  while (SetCookieInternal(aHostURI, aChannel,
                           cookieHeader, serverTime,
                           cookieStatus, cookiePolicy));

  
  LazyWrite();
  return NS_OK;
}

void
nsCookieService::LazyWrite()
{
  if (mWriteTimer) {
    mWriteTimer->SetDelay(kLazyWriteTimeout);
  } else {
    mWriteTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mWriteTimer) {
      mWriteTimer->InitWithFuncCallback(DoLazyWrite, this, kLazyWriteTimeout,
                                        nsITimer::TYPE_ONE_SHOT);
    }
  }
}

void
nsCookieService::DoLazyWrite(nsITimer *aTimer,
                             void     *aClosure)
{
  nsCookieService *service = NS_REINTERPRET_CAST(nsCookieService*, aClosure);
  service->Write();
  service->mWriteTimer = 0;
}


void
nsCookieService::NotifyRejected(nsIURI *aHostURI)
{
  if (mObserverService)
    mObserverService->NotifyObservers(aHostURI, "cookie-rejected", nsnull);
}







void
nsCookieService::NotifyChanged(nsICookie2      *aCookie,
                               const PRUnichar *aData)
{
  mCookieChanged = PR_TRUE;

  if (mObserverService)
    mObserverService->NotifyObservers(aCookie, "cookie-changed", aData);

  
  
  
  
  if (mCookiesPermissions == BEHAVIOR_P3P &&
      (!nsCRT::strcmp(aData, NS_LITERAL_STRING("added").get()) ||
       !nsCRT::strcmp(aData, NS_LITERAL_STRING("changed").get()))) {
    nsCookieStatus status;
    aCookie->GetStatus(&status);
    if (status == nsICookie::STATUS_DOWNGRADED ||
        status == nsICookie::STATUS_FLAGGED) {
      mCookieIconVisible = PR_TRUE;
      if (mObserverService)
        mObserverService->NotifyObservers(nsnull, "cookieIcon", NS_LITERAL_STRING("on").get());
    }
  }
}


NS_IMETHODIMP
nsCookieService::GetCookieIconIsVisible(PRBool *aIsVisible)
{
  *aIsVisible = mCookieIconVisible;
  return NS_OK;
}






void
nsCookieService::PrefChanged(nsIPrefBranch *aPrefBranch)
{
  PRInt32 val;
  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefCookiesPermissions, &val)))
    mCookiesPermissions = LIMIT(val, 0, 3, 0);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefMaxNumberOfCookies, &val)))
    mMaxNumberOfCookies = LIMIT(val, 0, 0xFFFF, 0xFFFF);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefMaxCookiesPerHost, &val)))
    mMaxCookiesPerHost = LIMIT(val, 0, 0xFFFF, 0xFFFF);
}






NS_IMETHODIMP
nsCookieService::RemoveAll()
{
  RemoveAllFromMemory();
  NotifyChanged(nsnull, NS_LITERAL_STRING("cleared").get());
  Write();
  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
COMArrayCallback(nsCookieEntry *aEntry,
                 void          *aArg)
{
  for (nsCookie *cookie = aEntry->Head(); cookie; cookie = cookie->Next()) {
    NS_STATIC_CAST(nsCOMArray<nsICookie>*, aArg)->AppendObject(cookie);
  }
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsCookieService::GetEnumerator(nsISimpleEnumerator **aEnumerator)
{
  RemoveExpiredCookies(NOW_IN_SECONDS);

  nsCOMArray<nsICookie> cookieList(mCookieCount);
  mHostTable.EnumerateEntries(COMArrayCallback, &cookieList);

  return NS_NewArrayEnumerator(aEnumerator, cookieList);
}

NS_IMETHODIMP
nsCookieService::Add(const nsACString &aDomain,
                     const nsACString &aPath,
                     const nsACString &aName,
                     const nsACString &aValue,
                     PRBool            aIsSecure,
                     PRBool            aIsSession,
                     PRInt64           aExpiry)
{
  nsInt64 currentTime = NOW_IN_SECONDS;

  nsRefPtr<nsCookie> cookie =
    nsCookie::Create(aName, aValue, aDomain, aPath,
                     nsInt64(aExpiry),
                     currentTime,
                     aIsSession,
                     aIsSecure,
                     PR_FALSE,
                     nsICookie::STATUS_UNKNOWN,
                     nsICookie::POLICY_UNKNOWN);
  if (!cookie) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  AddInternal(cookie, currentTime, nsnull, nsnull);
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::Remove(const nsACString &aHost,
                        const nsACString &aName,
                        const nsACString &aPath,
                        PRBool           aBlocked)
{
  nsListIter matchIter;
  if (FindCookie(PromiseFlatCString(aHost),
                 PromiseFlatCString(aName),
                 PromiseFlatCString(aPath),
                 matchIter)) {
    nsRefPtr<nsCookie> cookie = matchIter.current;
    RemoveCookieFromList(matchIter);
    NotifyChanged(cookie, NS_LITERAL_STRING("deleted").get());

    
    if (aBlocked && mPermissionService) {
      nsCAutoString host(NS_LITERAL_CSTRING("http://") + cookie->RawHost());
      nsCOMPtr<nsIURI> uri;
      NS_NewURI(getter_AddRefs(uri), host);

      if (uri)
        mPermissionService->SetAccess(uri, nsICookiePermission::ACCESS_DENY);
    }

    LazyWrite();
  }
  return NS_OK;
}






nsresult
nsCookieService::Read()
{
  nsresult rv;
  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), mCookieFile);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  static const char kTrue[] = "TRUE";

  nsCAutoString buffer;
  PRBool isMore = PR_TRUE;
  PRInt32 hostIndex, isDomainIndex, pathIndex, secureIndex, expiresIndex, nameIndex, cookieIndex;
  nsASingleFragmentCString::char_iterator iter;
  PRInt32 numInts;
  PRInt64 expires;
  PRBool isDomain, isHttpOnly = PR_FALSE;
  nsInt64 currentTime = NOW_IN_SECONDS;
  
  
  nsInt64 lastAccessedCounter = currentTime;
  nsCookie *newCookie;

  












  









  while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
    if (StringBeginsWith(buffer, NS_LITERAL_CSTRING(kHttpOnlyPrefix))) {
      isHttpOnly = PR_TRUE;
      hostIndex = sizeof(kHttpOnlyPrefix) - 1;
    } else if (buffer.IsEmpty() || buffer.First() == '#') {
      continue;
    } else {
      isHttpOnly = PR_FALSE;
      hostIndex = 0;
    }

    
    
    
    
    if ((isDomainIndex = buffer.FindChar('\t', hostIndex)     + 1) == 0 ||
        (pathIndex     = buffer.FindChar('\t', isDomainIndex) + 1) == 0 ||
        (secureIndex   = buffer.FindChar('\t', pathIndex)     + 1) == 0 ||
        (expiresIndex  = buffer.FindChar('\t', secureIndex)   + 1) == 0 ||
        (nameIndex     = buffer.FindChar('\t', expiresIndex)  + 1) == 0 ||
        (cookieIndex   = buffer.FindChar('\t', nameIndex)     + 1) == 0) {
      continue;
    }

    
    
    buffer.BeginWriting(iter);
    *(iter += nameIndex - 1) = char(0);
    numInts = PR_sscanf(buffer.get() + expiresIndex, "%lld", &expires);
    if (numInts != 1 || nsInt64(expires) < currentTime) {
      continue;
    }

    isDomain = Substring(buffer, isDomainIndex, pathIndex - isDomainIndex - 1)
               .EqualsLiteral(kTrue);
    const nsASingleFragmentCString &host = Substring(buffer, hostIndex, isDomainIndex - hostIndex - 1);
    
    
    if (isDomain && !host.IsEmpty() && host.First() != '.' ||
        host.FindChar(':') != kNotFound) {
      continue;
    }

    
    newCookie =
      nsCookie::Create(Substring(buffer, nameIndex, cookieIndex - nameIndex - 1),
                       Substring(buffer, cookieIndex, buffer.Length() - cookieIndex),
                       host,
                       Substring(buffer, pathIndex, secureIndex - pathIndex - 1),
                       nsInt64(expires),
                       lastAccessedCounter,
                       PR_FALSE,
                       Substring(buffer, secureIndex, expiresIndex - secureIndex - 1).EqualsLiteral(kTrue),
                       isHttpOnly,
                       nsICookie::STATUS_UNKNOWN,
                       nsICookie::POLICY_UNKNOWN);
    if (!newCookie) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    lastAccessedCounter -= nsInt64(1);

    if (!AddCookieToList(newCookie)) {
      
      
      
      
      
      
      
      delete newCookie;
    }
  }

  mCookieChanged = PR_FALSE;
  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
cookieListCallback(nsCookieEntry *aEntry,
                   void          *aArg)
{
  for (nsCookie *cookie = aEntry->Head(); cookie; cookie = cookie->Next()) {
    NS_STATIC_CAST(nsVoidArray*, aArg)->AppendElement(cookie);
  }
  return PL_DHASH_NEXT;
}

nsresult
nsCookieService::Write()
{
  if (!mCookieChanged) {
    return NS_OK;
  }

  if (!mCookieFile) {
    return NS_ERROR_NULL_POINTER;
  }

  nsresult rv;
  nsCOMPtr<nsIOutputStream> fileOutputStream;
  rv = NS_NewSafeLocalFileOutputStream(getter_AddRefs(fileOutputStream),
                                       mCookieFile,
                                       -1,
                                       0600);
  if (NS_FAILED(rv)) {
    NS_ERROR("failed to open cookies.txt for writing");
    return rv;
  }

  
  nsCOMPtr<nsIOutputStream> bufferedOutputStream;
  rv = NS_NewBufferedOutputStream(getter_AddRefs(bufferedOutputStream), fileOutputStream, 4096);
  if (NS_FAILED(rv)) {
    return rv;
  }

  static const char kHeader[] =
      "# HTTP Cookie File\n"
      "# http://wp.netscape.com/newsref/std/cookie_spec.html\n"
      "# This is a generated file!  Do not edit.\n"
      "# To delete cookies, use the Cookie Manager.\n\n";
  
  static const char kTrue[] = "\tTRUE\t";
  static const char kFalse[] = "\tFALSE\t";
  static const char kTab[] = "\t";
  static const char kNew[] = "\n";

  
  
  nsVoidArray sortedCookieList(mCookieCount);
  mHostTable.EnumerateEntries(cookieListCallback, &sortedCookieList);
  sortedCookieList.Sort(compareCookiesForWriting, nsnull);

  bufferedOutputStream->Write(kHeader, sizeof(kHeader) - 1, &rv);

  











  


   
  nsCookie *cookie;
  nsInt64 currentTime = NOW_IN_SECONDS;
  char dateString[22];
  PRUint32 dateLen;
  for (PRUint32 i = 0; i < mCookieCount; ++i) {
    cookie = NS_STATIC_CAST(nsCookie*, sortedCookieList.ElementAt(i));

    
    if (cookie->IsSession() || cookie->Expiry() <= currentTime) {
      continue;
    }

    
    if (cookie->IsHttpOnly()) {
      bufferedOutputStream->Write(kHttpOnlyPrefix, sizeof(kHttpOnlyPrefix) - 1, &rv);
    }
    bufferedOutputStream->Write(cookie->Host().get(), cookie->Host().Length(), &rv);
    if (cookie->IsDomain()) {
      bufferedOutputStream->Write(kTrue, sizeof(kTrue) - 1, &rv);
    } else {
      bufferedOutputStream->Write(kFalse, sizeof(kFalse) - 1, &rv);
    }
    bufferedOutputStream->Write(cookie->Path().get(), cookie->Path().Length(), &rv);
    if (cookie->IsSecure()) {
      bufferedOutputStream->Write(kTrue, sizeof(kTrue) - 1, &rv);
    } else {
      bufferedOutputStream->Write(kFalse, sizeof(kFalse) - 1, &rv);
    }
    dateLen = PR_snprintf(dateString, sizeof(dateString), "%lld", PRInt64(cookie->Expiry()));
    bufferedOutputStream->Write(dateString, dateLen, &rv);
    bufferedOutputStream->Write(kTab, sizeof(kTab) - 1, &rv);
    bufferedOutputStream->Write(cookie->Name().get(), cookie->Name().Length(), &rv);
    bufferedOutputStream->Write(kTab, sizeof(kTab) - 1, &rv);
    bufferedOutputStream->Write(cookie->Value().get(), cookie->Value().Length(), &rv);
    bufferedOutputStream->Write(kNew, sizeof(kNew) - 1, &rv);
  }

  
  
  nsCOMPtr<nsISafeOutputStream> safeStream = do_QueryInterface(bufferedOutputStream);
  NS_ASSERTION(safeStream, "expected a safe output stream!");
  if (safeStream) {
    rv = safeStream->Finish();
    if (NS_FAILED(rv)) {
      NS_WARNING("failed to save cookie file! possible dataloss");
      return rv;
    }
  }

  mCookieChanged = PR_FALSE;
  return NS_OK;
}








PRBool
nsCookieService::SetCookieInternal(nsIURI             *aHostURI,
                                   nsIChannel         *aChannel,
                                   nsDependentCString &aCookieHeader,
                                   nsInt64             aServerTime,
                                   nsCookieStatus      aStatus,
                                   nsCookiePolicy      aPolicy)
{
  
  
  nsCookieAttributes cookieAttributes;

  
  cookieAttributes.expiryTime = LL_MAXINT;

  
  
  nsDependentCString savedCookieHeader(aCookieHeader);

  
  
  const PRBool newCookie = ParseAttributes(aCookieHeader, cookieAttributes);

  
  
  const nsInt64 currentTime = NOW_IN_SECONDS;
  cookieAttributes.isSession = GetExpiry(cookieAttributes, aServerTime,
                                         currentTime, aStatus);

  CheckAndAdd(aHostURI, aChannel, cookieAttributes,
              aStatus, aPolicy, savedCookieHeader);

  return newCookie;
}

void
nsCookieService::CheckAndAdd(nsIURI               *aHostURI,
                             nsIChannel           *aChannel,
                             nsCookieAttributes   &aAttributes,
                             nsCookieStatus        aStatus,
                             const nsCookiePolicy  aPolicy,
                             const nsAFlatCString &aCookieHeader)
{
  
  if ((aAttributes.name.Length() + aAttributes.value.Length()) > kMaxBytesPerCookie) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "cookie too big (> 4kb)");
    return;
  }

  if (aAttributes.name.FindChar('\t') != kNotFound) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "invalid name character");
    return;
  }

  
  if (!CheckDomain(aAttributes, aHostURI)) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "failed the domain tests");
    return;
  }
  if (!CheckPath(aAttributes, aHostURI)) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "failed the path tests");
    return;
  }

  const nsInt64 currentTime = NOW_IN_SECONDS;

  
  nsRefPtr<nsCookie> cookie =
    nsCookie::Create(aAttributes.name,
                     aAttributes.value,
                     aAttributes.host,
                     aAttributes.path,
                     aAttributes.expiryTime,
                     currentTime,
                     aAttributes.isSession,
                     aAttributes.isSecure,
                     aAttributes.isHttpOnly,
                     aStatus,
                     aPolicy);
  if (!cookie) {
    return;
  }

  
  
  if (mPermissionService) {
    PRBool permission;
    
    
    mPermissionService->CanSetCookie(aHostURI,
                                     aChannel,
                                     NS_STATIC_CAST(nsICookie2*, NS_STATIC_CAST(nsCookie*, cookie)),
                                     &aAttributes.isSession,
                                     &aAttributes.expiryTime.mValue,
                                     &permission);
    if (!permission) {
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "cookie rejected by permission manager");
      NotifyRejected(aHostURI);
      return;
    }

    
    cookie->SetIsSession(aAttributes.isSession);
    cookie->SetExpiry(aAttributes.expiryTime);
  }

  
  AddInternal(cookie, NOW_IN_SECONDS, aHostURI, aCookieHeader.get());
}






void
nsCookieService::AddInternal(nsCookie   *aCookie,
                             nsInt64    aCurrentTime,
                             nsIURI     *aHostURI,
                             const char *aCookieHeader)
{
  nsListIter matchIter;
  const PRBool foundCookie =
    FindCookie(aCookie->Host(), aCookie->Name(), aCookie->Path(), matchIter);

  nsRefPtr<nsCookie> oldCookie;
  if (foundCookie) {
    oldCookie = matchIter.current;
    RemoveCookieFromList(matchIter);

    
    if (aCookie->Expiry() <= aCurrentTime) {
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "previously stored cookie was deleted");
      NotifyChanged(oldCookie, NS_LITERAL_STRING("deleted").get());
      return;
    }

    
    if (oldCookie) {
      aCookie->SetCreationTime(oldCookie->CreationTime());
    }

  } else {
    
    if (aCookie->Expiry() <= aCurrentTime) {
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "cookie has already expired");
      return;
    }

    
    nsEnumerationData data(aCurrentTime, LL_MAXINT);
    if (CountCookiesFromHost(aCookie, data) >= mMaxCookiesPerHost) {
      
      oldCookie = data.iter.current;
      RemoveCookieFromList(data.iter);

    } else if (mCookieCount >= mMaxNumberOfCookies) {
      
      RemoveExpiredCookies(aCurrentTime);

      
      if (mCookieCount >= mMaxNumberOfCookies) {
        
        data.oldestTime = LL_MAXINT;
        FindOldestCookie(data);
        oldCookie = data.iter.current;
        RemoveCookieFromList(data.iter);
      }
    }

    
    if (oldCookie)
      NotifyChanged(oldCookie, NS_LITERAL_STRING("deleted").get());
  }

  
  AddCookieToList(aCookie);
  NotifyChanged(aCookie, foundCookie ? NS_LITERAL_STRING("changed").get()
                                     : NS_LITERAL_STRING("added").get());

  COOKIE_LOGSUCCESS(SET_COOKIE, aHostURI, aCookieHeader, aCookie);
}
















































































static inline PRBool iswhitespace     (char c) { return c == ' '  || c == '\t'; }
static inline PRBool isterminator     (char c) { return c == '\n' || c == '\r'; }
static inline PRBool isquoteterminator(char c) { return isterminator(c) || c == '"'; }
static inline PRBool isvalueseparator (char c) { return isterminator(c) || c == ';'; }
static inline PRBool istokenseparator (char c) { return isvalueseparator(c) || c == '='; }



PRBool
nsCookieService::GetTokenValue(nsASingleFragmentCString::const_char_iterator &aIter,
                               nsASingleFragmentCString::const_char_iterator &aEndIter,
                               nsDependentCSubstring                         &aTokenString,
                               nsDependentCSubstring                         &aTokenValue,
                               PRBool                                        &aEqualsFound)
{
  nsASingleFragmentCString::const_char_iterator start, lastSpace;
  
  aTokenValue.Rebind(aIter, aIter);

  
  
  while (aIter != aEndIter && iswhitespace(*aIter))
    ++aIter;
  start = aIter;
  while (aIter != aEndIter && !istokenseparator(*aIter))
    ++aIter;

  
  lastSpace = aIter;
  if (lastSpace != start) {
    while (--lastSpace != start && iswhitespace(*lastSpace));
    ++lastSpace;
  }
  aTokenString.Rebind(start, lastSpace);

  aEqualsFound = (*aIter == '=');
  if (aEqualsFound) {
    
    while (++aIter != aEndIter && iswhitespace(*aIter));

    start = aIter;

    if (*aIter == '"') {
      
      
      
      
      while (++aIter != aEndIter && !isquoteterminator(*aIter)) {
        
        
        
        
        if (*aIter == '\\' && (++aIter == aEndIter || isterminator(*aIter)))
          break;
      }

      if (aIter != aEndIter && !isterminator(*aIter)) {
        
        aTokenValue.Rebind(start, ++aIter);
        
        while (aIter != aEndIter && !isvalueseparator(*aIter))
          ++aIter;
      }
    } else {
      
      
      while (aIter != aEndIter && !isvalueseparator(*aIter))
        ++aIter;

      
      if (aIter != start) {
        lastSpace = aIter;
        while (--lastSpace != start && iswhitespace(*lastSpace));
        aTokenValue.Rebind(start, ++lastSpace);
      }
    }
  }

  
  if (aIter != aEndIter) {
    
    if (isterminator(*aIter)) {
      ++aIter;
      return PR_TRUE;
    }
    
    ++aIter;
  }
  return PR_FALSE;
}



PRBool
nsCookieService::ParseAttributes(nsDependentCString &aCookieHeader,
                                 nsCookieAttributes &aCookieAttributes)
{
  static const char kPath[]    = "path";
  static const char kDomain[]  = "domain";
  static const char kExpires[] = "expires";
  static const char kMaxage[]  = "max-age";
  static const char kSecure[]  = "secure";
  static const char kHttpOnly[]  = "httponly";

  nsASingleFragmentCString::const_char_iterator tempBegin, tempEnd;
  nsASingleFragmentCString::const_char_iterator cookieStart, cookieEnd;
  aCookieHeader.BeginReading(cookieStart);
  aCookieHeader.EndReading(cookieEnd);

  aCookieAttributes.isSecure = PR_FALSE;
  aCookieAttributes.isHttpOnly = PR_FALSE;
  
  nsDependentCSubstring tokenString(cookieStart, cookieStart);
  nsDependentCSubstring tokenValue (cookieStart, cookieStart);
  PRBool newCookie, equalsFound;

  
  
  
  
  
  newCookie = GetTokenValue(cookieStart, cookieEnd, tokenString, tokenValue, equalsFound);
  if (equalsFound) {
    aCookieAttributes.name = tokenString;
    aCookieAttributes.value = tokenValue;
  } else {
    aCookieAttributes.value = tokenString;
  }

  
  while (cookieStart != cookieEnd && !newCookie) {
    newCookie = GetTokenValue(cookieStart, cookieEnd, tokenString, tokenValue, equalsFound);

    if (!tokenValue.IsEmpty()) {
      tokenValue.BeginReading(tempBegin);
      tokenValue.EndReading(tempEnd);
      if (*tempBegin == '"' && *--tempEnd == '"') {
        
        tokenValue.Rebind(++tempBegin, tempEnd);
      }
    }

    
    if (tokenString.LowerCaseEqualsLiteral(kPath))
      aCookieAttributes.path = tokenValue;

    else if (tokenString.LowerCaseEqualsLiteral(kDomain))
      aCookieAttributes.host = tokenValue;

    else if (tokenString.LowerCaseEqualsLiteral(kExpires))
      aCookieAttributes.expires = tokenValue;

    else if (tokenString.LowerCaseEqualsLiteral(kMaxage))
      aCookieAttributes.maxage = tokenValue;

    
    else if (tokenString.LowerCaseEqualsLiteral(kSecure))
      aCookieAttributes.isSecure = PR_TRUE;
      
    
    
    else if (tokenString.LowerCaseEqualsLiteral(kHttpOnly))
      aCookieAttributes.isHttpOnly = PR_TRUE;
  }

  
  aCookieHeader.Rebind(cookieStart, cookieEnd);
  return newCookie;
}







PRBool
nsCookieService::IsIPAddress(const nsAFlatCString &aHost)
{
  PRNetAddr addr;
  return (PR_StringToNetAddr(aHost.get(), &addr) == PR_SUCCESS);
}

PRBool
nsCookieService::IsInDomain(const nsACString &aDomain,
                            const nsACString &aHost,
                            PRBool           aIsDomain)
{
  
  
  
  if (!aIsDomain) {
    return aDomain.Equals(aHost);
  }

  
  








  
  PRUint32 domainLength = aDomain.Length();
  PRInt32 lengthDifference = aHost.Length() - domainLength;
  
  
  
  
  if (lengthDifference == 0) {
    return aDomain.Equals(aHost);
  }
  
  if (lengthDifference > 0) {
    return aDomain.Equals(Substring(aHost, lengthDifference, domainLength));
  }
  
  if (lengthDifference == -1) {
    return Substring(aDomain, 1, domainLength - 1).Equals(aHost);
  }
  
  return PR_FALSE;
}

PRBool
nsCookieService::IsForeign(nsIURI *aHostURI,
                           nsIURI *aFirstURI)
{
  
  if (!aFirstURI) {
    return PR_FALSE;
  }

  
  
  
  PRBool isChrome = PR_FALSE;
  nsresult rv = aFirstURI->SchemeIs("chrome", &isChrome);
  if (NS_SUCCEEDED(rv) && isChrome) {
    return PR_FALSE;
  }

  
  nsCAutoString currentHost, firstHost;
  if (NS_FAILED(aHostURI->GetAsciiHost(currentHost)) ||
      NS_FAILED(aFirstURI->GetAsciiHost(firstHost))) {
    return PR_TRUE;
  }
  
  currentHost.Trim(".");
  firstHost.Trim(".");
  ToLowerCase(currentHost);
  ToLowerCase(firstHost);

  
  

  
  
  
  if (IsIPAddress(firstHost)) {
    return !IsInDomain(firstHost, currentHost, PR_FALSE);
  }

  
  
  
  
  
  
  
  
  
  PRUint32 dotsInFirstHost = firstHost.CountChar('.');
  if (dotsInFirstHost == currentHost.CountChar('.') &&
      dotsInFirstHost >= 2) {
    
    PRInt32 dot1 = firstHost.FindChar('.');
    return !IsInDomain(Substring(firstHost, dot1, firstHost.Length() - dot1), currentHost);
  }

  
  
  return !IsInDomain(NS_LITERAL_CSTRING(".") + firstHost, currentHost);
}

nsCookieStatus
nsCookieService::CheckPrefs(nsIURI         *aHostURI,
                            nsIURI         *aFirstURI,
                            nsIChannel     *aChannel,
                            const char     *aCookieHeader,
                            nsCookiePolicy &aPolicy)
{
  
  
  
  
  
  
  
  
  
  

  
  
  

  
  
  nsCAutoString currentURIScheme, firstURIScheme;
  nsresult rv, rv2 = NS_OK;
  rv = aHostURI->GetScheme(currentURIScheme);
  if (aFirstURI) {
    rv2 = aFirstURI->GetScheme(firstURIScheme);
  }
  if (NS_FAILED(rv) || NS_FAILED(rv2)) {
    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "couldn't get scheme of host URI");
    return STATUS_REJECTED_WITH_ERROR;
  }

  
  if (currentURIScheme.EqualsLiteral("ftp")) {
    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "ftp sites cannot read cookies");
    return STATUS_REJECTED_WITH_ERROR;
  }

  
  
  if (mPermissionService) {
    nsCookieAccess access;
    rv = mPermissionService->CanAccess(aHostURI, aFirstURI, aChannel, &access);

    
    if (NS_SUCCEEDED(rv)) {
      switch (access) {
      case nsICookiePermission::ACCESS_DENY:
        COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "cookies are blocked for this site");
        return nsICookie::STATUS_REJECTED;

      case nsICookiePermission::ACCESS_ALLOW:
        return nsICookie::STATUS_ACCEPTED;
      }
    }
  }

  
  if (mCookiesPermissions == BEHAVIOR_REJECT) {
    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "cookies are disabled");
    return nsICookie::STATUS_REJECTED;

  } else if (mCookiesPermissions == BEHAVIOR_REJECTFOREIGN) {
    
    

    
    
    
    if (IsForeign(aHostURI, aFirstURI)) {
      COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "originating server test failed");
      return nsICookie::STATUS_REJECTED;
    }

  } else if (mCookiesPermissions == BEHAVIOR_P3P) {
    
    

    nsCookieStatus p3pStatus = nsICookie::STATUS_UNKNOWN;

    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aChannel);

    
    if (!mP3PService)
      mP3PService = do_GetService(NS_COOKIECONSENT_CONTRACTID);

    if (mP3PService) {
      
      PRBool isForeign = IsForeign(aHostURI, aFirstURI);
      mP3PService->GetConsent(aHostURI, httpChannel, isForeign, &aPolicy, &p3pStatus);
    }

    if (p3pStatus == nsICookie::STATUS_REJECTED) {
      COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "P3P test failed");
    }
    return p3pStatus;
  }

  
  return nsICookie::STATUS_ACCEPTED;
}


PRBool
nsCookieService::CheckDomain(nsCookieAttributes &aCookieAttributes,
                             nsIURI             *aHostURI)
{
  
  nsCAutoString hostFromURI;
  if (NS_FAILED(aHostURI->GetAsciiHost(hostFromURI))) {
    return PR_FALSE;
  }
  
  hostFromURI.Trim(".");
  ToLowerCase(hostFromURI);

  
  if (!aCookieAttributes.host.IsEmpty()) {
    aCookieAttributes.host.Trim(".");
    
    ToLowerCase(aCookieAttributes.host);

    
    
    
    
    
    if (IsIPAddress(aCookieAttributes.host)) {
      return IsInDomain(aCookieAttributes.host, hostFromURI, PR_FALSE);
    }

    





    PRInt32 dot = aCookieAttributes.host.FindChar('.');
    if (dot == kNotFound) {
      
      return PR_FALSE;
    }

    
    aCookieAttributes.host.Insert(NS_LITERAL_CSTRING("."), 0);
    if (!IsInDomain(aCookieAttributes.host, hostFromURI)) {
      return PR_FALSE;
    }

    







  
  } else {
    
    if (hostFromURI.IsEmpty()) {
      PRBool isFileURI = PR_FALSE;
      aHostURI->SchemeIs("file", &isFileURI);
      if (!isFileURI)
        return PR_FALSE;
    }
    aCookieAttributes.host = hostFromURI;
  }

  return PR_TRUE;
}

PRBool
nsCookieService::CheckPath(nsCookieAttributes &aCookieAttributes,
                           nsIURI             *aHostURI)
{
  
  if (aCookieAttributes.path.IsEmpty()) {
    
    
    
    
    nsCOMPtr<nsIURL> hostURL = do_QueryInterface(aHostURI);
    if (hostURL) {
      hostURL->GetDirectory(aCookieAttributes.path);
    } else {
      aHostURI->GetPath(aCookieAttributes.path);
      PRInt32 slash = aCookieAttributes.path.RFindChar('/');
      if (slash != kNotFound) {
        aCookieAttributes.path.Truncate(slash + 1);
      }
    }

  } else {
    if (aCookieAttributes.path.Length() > kMaxBytesPerPath ||
        aCookieAttributes.path.FindChar('\t') != kNotFound )
      return PR_FALSE;

#if 0
    





    
    nsCAutoString pathFromURI;
    if (NS_FAILED(aHostURI->GetPath(pathFromURI)) ||
        !StringBeginsWith(pathFromURI, aCookieAttributes.path)) {
      return PR_FALSE;
    }
#endif
  }

  return PR_TRUE;
}

PRBool
nsCookieService::GetExpiry(nsCookieAttributes &aCookieAttributes,
                           nsInt64            aServerTime,
                           nsInt64            aCurrentTime,
                           nsCookieStatus     aStatus)
{
  






  nsInt64 delta;

  
  if (!aCookieAttributes.maxage.IsEmpty()) {
    
    PRInt64 maxage;
    PRInt32 numInts = PR_sscanf(aCookieAttributes.maxage.get(), "%lld", &maxage);

    
    if (numInts != 1) {
      return PR_TRUE;
    }

    delta = nsInt64(maxage);

  
  } else if (!aCookieAttributes.expires.IsEmpty()) {
    nsInt64 expires;
    PRTime tempExpires;

    
    if (PR_ParseTimeString(aCookieAttributes.expires.get(), PR_TRUE, &tempExpires) == PR_SUCCESS) {
      expires = nsInt64(tempExpires) / USEC_PER_SEC;
    } else {
      return PR_TRUE;
    }

    delta = expires - aServerTime;

  
  } else {
    return PR_TRUE;
  }

  
  
  aCookieAttributes.expiryTime = aCurrentTime + delta;

  
  
  
  return aStatus == nsICookie::STATUS_DOWNGRADED &&
         aCookieAttributes.expiryTime > aCurrentTime;
}






void
nsCookieService::RemoveAllFromMemory()
{
  
  
  mHostTable.Clear();
  mCookieCount = 0;
  mCookieChanged = PR_TRUE;
}

PLDHashOperator PR_CALLBACK
removeExpiredCallback(nsCookieEntry *aEntry,
                      void          *aArg)
{
  const nsInt64 &currentTime = *NS_STATIC_CAST(nsInt64*, aArg);
  for (nsListIter iter(aEntry, nsnull, aEntry->Head()); iter.current; ) {
    if (iter.current->Expiry() <= currentTime)
      
      nsCookieService::gCookieService->RemoveCookieFromList(iter);
    else
      ++iter;
  }
  return PL_DHASH_NEXT;
}


void
nsCookieService::RemoveExpiredCookies(nsInt64 aCurrentTime)
{
  mHostTable.EnumerateEntries(removeExpiredCallback, &aCurrentTime);
}




NS_IMETHODIMP
nsCookieService::FindMatchingCookie(nsICookie2 *aCookie,
                                    PRUint32   *aCountFromHost,
                                    PRBool     *aFoundCookie)
{
  NS_ENSURE_ARG_POINTER(aCookie);

  
  nsEnumerationData data(NOW_IN_SECONDS, LL_MININT);
  nsCookie *cookie = NS_STATIC_CAST(nsCookie*, aCookie);

  *aCountFromHost = CountCookiesFromHost(cookie, data);
  *aFoundCookie = FindCookie(cookie->Host(), cookie->Name(), cookie->Path(), data.iter);
  return NS_OK;
}



PRUint32
nsCookieService::CountCookiesFromHost(nsCookie          *aCookie,
                                      nsEnumerationData &aData)
{
  PRUint32 countFromHost = 0;

  nsCAutoString hostWithDot(NS_LITERAL_CSTRING(".") + aCookie->RawHost());

  const char *currentDot = hostWithDot.get();
  const char *nextDot = currentDot + 1;
  do {
    nsCookieEntry *entry = mHostTable.GetEntry(currentDot);
    for (nsListIter iter(entry); iter.current; ++iter) {
      
      if (iter.current->Expiry() > aData.currentTime) {
        ++countFromHost;

        
        if (aData.oldestTime > iter.current->LastAccessed()) {
          aData.oldestTime = iter.current->LastAccessed();
          aData.iter = iter;
        }
      }
    }

    currentDot = nextDot;
    if (currentDot)
      nextDot = strchr(currentDot + 1, '.');

  } while (currentDot);

  return countFromHost;
}


PRBool
nsCookieService::FindCookie(const nsAFlatCString &aHost,
                            const nsAFlatCString &aName,
                            const nsAFlatCString &aPath,
                            nsListIter           &aIter)
{
  nsCookieEntry *entry = mHostTable.GetEntry(aHost.get());
  for (aIter = nsListIter(entry); aIter.current; ++aIter) {
    if (aPath.Equals(aIter.current->Path()) &&
        aName.Equals(aIter.current->Name())) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}


void
nsCookieService::RemoveCookieFromList(nsListIter &aIter)
{
  if (!aIter.prev && !aIter.current->Next()) {
    
    
    
    mHostTable.RawRemoveEntry(aIter.entry);
    aIter.current = nsnull;

  } else {
    
    nsCookie *next = aIter.current->Next();
    NS_RELEASE(aIter.current);
    if (aIter.prev) {
      
      aIter.current = aIter.prev->Next() = next;
    } else {
      
      aIter.current = aIter.entry->Head() = next;
    }
  }

  --mCookieCount;
  mCookieChanged = PR_TRUE;
}

PRBool
nsCookieService::AddCookieToList(nsCookie *aCookie)
{
  nsCookieEntry *entry = mHostTable.PutEntry(aCookie->Host().get());

  if (!entry) {
    NS_ERROR("can't insert element into a null entry!");
    return PR_FALSE;
  }

  NS_ADDREF(aCookie);

  aCookie->Next() = entry->Head();
  entry->Head() = aCookie;
  ++mCookieCount;
  mCookieChanged = PR_TRUE;

  return PR_TRUE;
}

PR_STATIC_CALLBACK(PLDHashOperator)
findOldestCallback(nsCookieEntry *aEntry,
                   void          *aArg)
{
  nsEnumerationData *data = NS_STATIC_CAST(nsEnumerationData*, aArg);
  for (nsListIter iter(aEntry, nsnull, aEntry->Head()); iter.current; ++iter) {
    
    if (data->oldestTime > iter.current->LastAccessed()) {
      data->oldestTime = iter.current->LastAccessed();
      data->iter = iter;
    }
  }
  return PL_DHASH_NEXT;
}

void
nsCookieService::FindOldestCookie(nsEnumerationData &aData)
{
  mHostTable.EnumerateEntries(findOldestCallback, &aData);
}
