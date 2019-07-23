








































#include "nsCookieService.h"
#include "nsIServiceManager.h"

#include "nsIIOService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIPrefService.h"
#include "nsICookiePermission.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsIChannel.h"
#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsILineInputStream.h"
#include "nsIEffectiveTLDService.h"
#include "nsIIDNService.h"

#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsIMutableArray.h"
#include "nsArrayEnumerator.h"
#include "nsEnumeratorUtils.h"
#include "nsAutoPtr.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "prtime.h"
#include "prprf.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsAppDirectoryServiceDefs.h"
#include "mozIStorageService.h"
#include "mozStorageHelper.h"
#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"








static const char kHttpOnlyPrefix[] = "#HttpOnly_";

static const char kCookieFileName[] = "cookies.sqlite";
#define COOKIES_SCHEMA_VERSION 2

static const PRInt64 kCookieStaleThreshold = 60 * PR_USEC_PER_SEC; 
static const PRInt64 kCookiePurgeAge = 30 * 24 * 60 * 60 * PR_USEC_PER_SEC; 

static const char kOldCookieFileName[] = "cookies.txt";

#undef  LIMIT
#define LIMIT(x, low, high, default) ((x) >= (low) && (x) <= (high) ? (x) : (default))

#undef  ADD_TEN_PERCENT
#define ADD_TEN_PERCENT(i) ((i) + (i)/10)



static const PRUint32 kMaxNumberOfCookies = 3000;
static const PRUint32 kMaxCookiesPerHost  = 50;
static const PRUint32 kMaxBytesPerCookie  = 4096;
static const PRUint32 kMaxBytesPerPath    = 1024;


static const PRUint32 STATUS_ACCEPTED            = 0;
static const PRUint32 STATUS_REJECTED            = 1;




static const PRUint32 STATUS_REJECTED_WITH_ERROR = 2;


static const PRUint32 BEHAVIOR_ACCEPT        = 0;
static const PRUint32 BEHAVIOR_REJECTFOREIGN = 1;
static const PRUint32 BEHAVIOR_REJECT        = 2;


static const char kPrefCookiesPermissions[] = "network.cookie.cookieBehavior";
static const char kPrefMaxNumberOfCookies[] = "network.cookie.maxNumber";
static const char kPrefMaxCookiesPerHost[]  = "network.cookie.maxPerHost";
static const char kPrefCookiePurgeAge[]     = "network.cookie.purgeAge";


struct nsCookieAttributes
{
  nsCAutoString name;
  nsCAutoString value;
  nsCAutoString host;
  nsCAutoString path;
  nsCAutoString expires;
  nsCAutoString maxage;
  PRInt64 expiryTime;
  PRBool isSession;
  PRBool isSecure;
  PRBool isHttpOnly;
};




struct nsListIter
{
  
  nsListIter()
  {
  }

  
  
  explicit
  nsListIter(nsCookieEntry *aEntry, nsCookieEntry::IndexType aIndex)
   : entry(aEntry)
   , index(aIndex)
  {
  }

  
  nsCookie * Cookie() const
  {
    return entry->GetCookies()[index];
  }

  nsCookieEntry            *entry;
  nsCookieEntry::IndexType  index;
};



struct nsEnumerationData
{
  nsEnumerationData(PRInt64 aCurrentTime, PRInt64 aOldestTime)
   : currentTime(aCurrentTime)
   , oldestTime(aOldestTime)
  {
  }

  
  PRInt64 currentTime;

  
  
  PRInt64 oldestTime;

  
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

#define COOKIE_LOGFAILURE(a, b, c, d)    LogFailure(a, b, c, d)
#define COOKIE_LOGSUCCESS(a, b, c, d, e) LogSuccess(a, b, c, d, e)

#define COOKIE_LOGEVICTED(a)                   \
  PR_BEGIN_MACRO                               \
    if (PR_LOG_TEST(sCookieLog, PR_LOG_DEBUG)) \
      LogEvicted(a);                           \
  PR_END_MACRO

#define COOKIE_LOGSTRING(lvl, fmt)   \
  PR_BEGIN_MACRO                     \
    PR_LOG(sCookieLog, lvl, fmt);    \
    PR_LOG(sCookieLog, lvl, ("\n")); \
  PR_END_MACRO

static void
LogFailure(PRBool aSetCookie, nsIURI *aHostURI, const char *aCookieString, const char *aReason)
{
  
  if (!PR_LOG_TEST(sCookieLog, PR_LOG_WARNING))
    return;

  nsCAutoString spec;
  if (aHostURI)
    aHostURI->GetAsciiSpec(spec);

  PR_LOG(sCookieLog, PR_LOG_WARNING,
    ("===== %s =====\n", aSetCookie ? "COOKIE NOT ACCEPTED" : "COOKIE NOT SENT"));
  PR_LOG(sCookieLog, PR_LOG_WARNING,("request URL: %s\n", spec.get()));
  if (aSetCookie)
    PR_LOG(sCookieLog, PR_LOG_WARNING,("cookie string: %s\n", aCookieString));

  PRExplodedTime explodedTime;
  PR_ExplodeTime(PR_Now(), PR_GMTParameters, &explodedTime);
  char timeString[40];
  PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);

  PR_LOG(sCookieLog, PR_LOG_WARNING,("current time: %s", timeString));
  PR_LOG(sCookieLog, PR_LOG_WARNING,("rejected because %s\n", aReason));
  PR_LOG(sCookieLog, PR_LOG_WARNING,("\n"));
}

static void
LogCookie(nsCookie *aCookie)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(PR_Now(), PR_GMTParameters, &explodedTime);
  char timeString[40];
  PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);

  PR_LOG(sCookieLog, PR_LOG_DEBUG,("current time: %s", timeString));

  if (aCookie) {
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("----------------\n"));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("name: %s\n", aCookie->Name().get()));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("value: %s\n", aCookie->Value().get()));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("%s: %s\n", aCookie->IsDomain() ? "domain" : "host", aCookie->Host().get()));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("path: %s\n", aCookie->Path().get()));

    PR_ExplodeTime(aCookie->Expiry() * PR_USEC_PER_SEC, PR_GMTParameters, &explodedTime);
    PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);
    PR_LOG(sCookieLog, PR_LOG_DEBUG,
      ("expires: %s%s", timeString, aCookie->IsSession() ? " (at end of session)" : ""));

    PR_ExplodeTime(aCookie->CreationID(), PR_GMTParameters, &explodedTime);
    PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);
    PR_LOG(sCookieLog, PR_LOG_DEBUG,
      ("created: %s (id %lld)", timeString, aCookie->CreationID()));

    PR_LOG(sCookieLog, PR_LOG_DEBUG,("is secure: %s\n", aCookie->IsSecure() ? "true" : "false"));
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("is httpOnly: %s\n", aCookie->IsHttpOnly() ? "true" : "false"));
  }
}

static void
LogSuccess(PRBool aSetCookie, nsIURI *aHostURI, const char *aCookieString, nsCookie *aCookie, PRBool aReplacing)
{
  
  if (!PR_LOG_TEST(sCookieLog, PR_LOG_DEBUG)) {
    return;
  }

  nsCAutoString spec;
  if (aHostURI)
    aHostURI->GetAsciiSpec(spec);

  PR_LOG(sCookieLog, PR_LOG_DEBUG,
    ("===== %s =====\n", aSetCookie ? "COOKIE ACCEPTED" : "COOKIE SENT"));
  PR_LOG(sCookieLog, PR_LOG_DEBUG,("request URL: %s\n", spec.get()));
  PR_LOG(sCookieLog, PR_LOG_DEBUG,("cookie string: %s\n", aCookieString));
  if (aSetCookie)
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("replaces existing cookie: %s\n", aReplacing ? "true" : "false"));

  LogCookie(aCookie);

  PR_LOG(sCookieLog, PR_LOG_DEBUG,("\n"));
}

static void
LogEvicted(nsCookie *aCookie)
{
  PR_LOG(sCookieLog, PR_LOG_DEBUG,("===== COOKIE EVICTED =====\n"));

  LogCookie(aCookie);

  PR_LOG(sCookieLog, PR_LOG_DEBUG,("\n"));
}


static inline void
LogFailure(PRBool aSetCookie, nsIURI *aHostURI, const nsAFlatCString &aCookieString, const char *aReason)
{
  LogFailure(aSetCookie, aHostURI, aCookieString.get(), aReason);
}

static inline void
LogSuccess(PRBool aSetCookie, nsIURI *aHostURI, const nsAFlatCString &aCookieString, nsCookie *aCookie, PRBool aReplacing)
{
  LogSuccess(aSetCookie, aHostURI, aCookieString.get(), aCookie, aReplacing);
}

#else
#define COOKIE_LOGFAILURE(a, b, c, d)    PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define COOKIE_LOGSUCCESS(a, b, c, d, e) PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define COOKIE_LOGEVICTED(a)             PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define COOKIE_LOGSTRING(a, b)           PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#endif






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






NS_IMPL_ISUPPORTS5(nsCookieService,
                   nsICookieService,
                   nsICookieManager,
                   nsICookieManager2,
                   nsIObserver,
                   nsISupportsWeakReference)

nsCookieService::nsCookieService()
 : mDBState(&mDefaultDBState)
 , mCookiesPermissions(BEHAVIOR_ACCEPT)
 , mMaxNumberOfCookies(kMaxNumberOfCookies)
 , mMaxCookiesPerHost(kMaxCookiesPerHost)
 , mCookiePurgeAge(kCookiePurgeAge)
{
}

nsresult
nsCookieService::Init()
{
  if (!mDBState->hostTable.Init()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv;
  mTLDService = do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mIDNService = do_GetService(NS_IDNSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPrefBranch2> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefBranch) {
    prefBranch->AddObserver(kPrefCookiesPermissions, this, PR_TRUE);
    prefBranch->AddObserver(kPrefMaxNumberOfCookies, this, PR_TRUE);
    prefBranch->AddObserver(kPrefMaxCookiesPerHost,  this, PR_TRUE);
    prefBranch->AddObserver(kPrefCookiePurgeAge,     this, PR_TRUE);
    PrefChanged(prefBranch);
  }

  
  
  rv = InitDB();
  if (NS_FAILED(rv))
    COOKIE_LOGSTRING(PR_LOG_WARNING, ("Init(): InitDB() gave error %x", rv));

  mObserverService = do_GetService("@mozilla.org/observer-service;1");
  if (mObserverService) {
    mObserverService->AddObserver(this, "profile-before-change", PR_TRUE);
    mObserverService->AddObserver(this, "profile-do-change", PR_TRUE);
    mObserverService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_TRUE);

    nsCOMPtr<nsIPrivateBrowsingService> pbs =
      do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    if (pbs) {
      PRBool inPrivateBrowsing = PR_FALSE;
      pbs->GetPrivateBrowsingEnabled(&inPrivateBrowsing);
      if (inPrivateBrowsing) {
        Observe(nsnull, NS_PRIVATE_BROWSING_SWITCH_TOPIC,
                NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).get());
      }
    }
  }

  mPermissionService = do_GetService(NS_COOKIEPERMISSION_CONTRACTID);
  if (!mPermissionService) {
    NS_WARNING("nsICookiePermission implementation not available - some features won't work!");
    COOKIE_LOGSTRING(PR_LOG_WARNING, ("Init(): nsICookiePermission implementation not available"));
  }

  return NS_OK;
}

nsresult
nsCookieService::InitDB()
{
  NS_ASSERTION(mDBState == &mDefaultDBState, "not in default DB state");

  
  nsresult rv = TryInitDB(PR_FALSE);
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    COOKIE_LOGSTRING(PR_LOG_WARNING, ("InitDB(): db corrupt, trying again", rv));

    rv = TryInitDB(PR_TRUE);
  }

  if (NS_FAILED(rv)) {
    
    CloseDB();
  }
  return rv;
}

nsresult
nsCookieService::TryInitDB(PRBool aDeleteExistingDB)
{
  
  CloseDB();
  RemoveAllFromMemory();

  nsCOMPtr<nsIFile> cookieFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(cookieFile));
  if (NS_FAILED(rv)) return rv;

  cookieFile->AppendNative(NS_LITERAL_CSTRING(kCookieFileName));

  
  if (aDeleteExistingDB) {
    rv = cookieFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<mozIStorageService> storage = do_GetService("@mozilla.org/storage/service;1");
  if (!storage)
    return NS_ERROR_UNEXPECTED;

  
  
  rv = storage->OpenUnsharedDatabase(cookieFile, getter_AddRefs(mDBState->dbConn));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool tableExists = PR_FALSE;
  mDBState->dbConn->TableExists(NS_LITERAL_CSTRING("moz_cookies"), &tableExists);
  if (!tableExists) {
      rv = CreateTable();
      NS_ENSURE_SUCCESS(rv, rv);

  } else {
    
    PRInt32 dbSchemaVersion;
    rv = mDBState->dbConn->GetSchemaVersion(&dbSchemaVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    switch (dbSchemaVersion) {
    
    
    
    case 1:
      {
        
        rv = mDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "ALTER TABLE moz_cookies ADD lastAccessed INTEGER"));
        NS_ENSURE_SUCCESS(rv, rv);

        
        rv = mDBState->dbConn->SetSchemaVersion(COOKIES_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      

    case COOKIES_SCHEMA_VERSION:
      break;

    case 0:
      {
        NS_WARNING("couldn't get schema version!");
          
        
        
        
        
        
        rv = mDBState->dbConn->SetSchemaVersion(COOKIES_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      

    
    
    
    
    
    
    default:
      {
        
        nsCOMPtr<mozIStorageStatement> stmt;
        rv = mDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT "
            "id, "
            "name, "
            "value, "
            "host, "
            "path, "
            "expiry, "
            "isSecure, "
            "isHttpOnly "
          "FROM moz_cookies"), getter_AddRefs(stmt));
        if (NS_SUCCEEDED(rv))
          break;

        
        rv = mDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE moz_cookies"));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = CreateTable();
        NS_ENSURE_SUCCESS(rv, rv);
      }
      break;
    }
  }

  
  mDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA synchronous = OFF"));

  
  mDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("PRAGMA locking_mode = EXCLUSIVE"));

  
  rv = mDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_cookies ("
      "id, "
      "name, "
      "value, "
      "host, "
      "path, "
      "expiry, "
      "lastAccessed, "
      "isSecure, "
      "isHttpOnly"
    ") VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9)"),
    getter_AddRefs(mDBState->stmtInsert));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_cookies WHERE id = ?1"),
    getter_AddRefs(mDBState->stmtDelete));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_cookies SET lastAccessed = ?1 WHERE id = ?2"),
    getter_AddRefs(mDBState->stmtUpdate));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aDeleteExistingDB)
    return NS_OK;

  
  if (tableExists)
    return Read();

  nsCOMPtr<nsIFile> oldCookieFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(oldCookieFile));
  if (NS_FAILED(rv)) return rv;

  oldCookieFile->AppendNative(NS_LITERAL_CSTRING(kOldCookieFileName));
  rv = ImportCookies(oldCookieFile);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_FILE_NOT_FOUND)
      return NS_OK;

    return rv;
  }

  
  oldCookieFile->Remove(PR_FALSE);
  return NS_OK;
}


nsresult
nsCookieService::CreateTable()
{
  
  nsresult rv = mDBState->dbConn->SetSchemaVersion(COOKIES_SCHEMA_VERSION);
  if (NS_FAILED(rv)) return rv;

  
  return mDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE moz_cookies ("
      "id INTEGER PRIMARY KEY, "
      "name TEXT, "
      "value TEXT, "
      "host TEXT, "
      "path TEXT, "
      "expiry INTEGER, "
      "lastAccessed INTEGER, "
      "isSecure INTEGER, "
      "isHttpOnly INTEGER"
    ")"));
}

void
nsCookieService::CloseDB()
{
  NS_ASSERTION(!mPrivateDBState.dbConn, "private DB connection should always be null");

  
  
  mDefaultDBState.stmtInsert = nsnull;
  mDefaultDBState.stmtDelete = nsnull;
  mDefaultDBState.stmtUpdate = nsnull;
  mDefaultDBState.dbConn = nsnull;
}

nsCookieService::~nsCookieService()
{
  CloseDB();

  gCookieService = nsnull;
}

NS_IMETHODIMP
nsCookieService::Observe(nsISupports     *aSubject,
                         const char      *aTopic,
                         const PRUnichar *aData)
{
  
  if (!strcmp(aTopic, "profile-before-change")) {
    
    
    RemoveAllFromMemory();

    if (mDBState->dbConn) {
      if (!nsCRT::strcmp(aData, NS_LITERAL_STRING("shutdown-cleanse").get())) {
        
        nsresult rv = mDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM moz_cookies"));
        if (NS_FAILED(rv))
          NS_WARNING("db delete failed");
      }

      
      CloseDB();
    }

  } else if (!strcmp(aTopic, "profile-do-change")) {
    
    
    
    
    if (mDBState == &mPrivateDBState) {
      mDBState = &mDefaultDBState;
      InitDB();
      mDBState = &mPrivateDBState;
    } else {
      InitDB();
    }

  } else if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(aSubject);
    if (prefBranch)
      PrefChanged(prefBranch);
  } else if (!strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC)) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(aData)) {
      if (!mPrivateDBState.hostTable.IsInitialized() &&
          !mPrivateDBState.hostTable.Init())
        return NS_ERROR_OUT_OF_MEMORY;

      NS_ASSERTION(mDBState == &mDefaultDBState, "already in private state");
      NS_ASSERTION(mPrivateDBState.cookieCount == 0, "private count not 0");
      NS_ASSERTION(mPrivateDBState.cookieOldestTime == LL_MAXINT, "private time not reset");
      NS_ASSERTION(mPrivateDBState.hostTable.Count() == 0, "private table not empty");
      NS_ASSERTION(mPrivateDBState.dbConn == NULL, "private DB connection not null");

      
      mDBState = &mPrivateDBState;

      NotifyChanged(nsnull, NS_LITERAL_STRING("reload").get());

    } else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData)) {
      
      mDBState = &mDefaultDBState;

      NS_ASSERTION(!mPrivateDBState.dbConn, "private DB connection not null");

      mPrivateDBState.cookieCount = 0;
      mPrivateDBState.cookieOldestTime = LL_MAXINT;
      if (mPrivateDBState.hostTable.IsInitialized())
        mPrivateDBState.hostTable.Clear();

      NotifyChanged(nsnull, NS_LITERAL_STRING("reload").get());
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::GetCookieString(nsIURI     *aHostURI,
                                 nsIChannel *aChannel,
                                 char       **aCookie)
{
  GetCookieInternal(aHostURI, aChannel, PR_FALSE, aCookie);
  
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::GetCookieStringFromHttp(nsIURI     *aHostURI,
                                         nsIURI     *aFirstURI,
                                         nsIChannel *aChannel,
                                         char       **aCookie)
{
  GetCookieInternal(aHostURI, aChannel, PR_TRUE, aCookie);

  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::SetCookieString(nsIURI     *aHostURI,
                                 nsIPrompt  *aPrompt,
                                 const char *aCookieHeader,
                                 nsIChannel *aChannel)
{
  return SetCookieStringInternal(aHostURI, aPrompt, aCookieHeader, nsnull, aChannel, PR_FALSE);
}

NS_IMETHODIMP
nsCookieService::SetCookieStringFromHttp(nsIURI     *aHostURI,
                                         nsIURI     *aFirstURI,
                                         nsIPrompt  *aPrompt,
                                         const char *aCookieHeader,
                                         const char *aServerTime,
                                         nsIChannel *aChannel) 
{
  return SetCookieStringInternal(aHostURI, aPrompt, aCookieHeader, aServerTime, aChannel, PR_TRUE);
}

nsresult
nsCookieService::SetCookieStringInternal(nsIURI     *aHostURI,
                                         nsIPrompt  *aPrompt,
                                         const char *aCookieHeader,
                                         const char *aServerTime,
                                         nsIChannel *aChannel,
                                         PRBool      aFromHttp) 
{
  if (!aHostURI) {
    COOKIE_LOGFAILURE(SET_COOKIE, nsnull, aCookieHeader, "host URI is null");
    return NS_OK;
  }

  
  
  
  
  
  PRBool requireHostMatch;
  nsCAutoString baseDomain;
  nsresult rv = GetBaseDomain(aHostURI, baseDomain, requireHostMatch);
  if (NS_FAILED(rv)) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, 
                      "couldn't get base domain from URI");
    return NS_OK;
  }

  
  PRUint32 cookieStatus = CheckPrefs(aHostURI, aChannel, baseDomain,
                                     requireHostMatch, aCookieHeader);
  
  switch (cookieStatus) {
  case STATUS_REJECTED:
    NotifyRejected(aHostURI);
  case STATUS_REJECTED_WITH_ERROR:
    return NS_OK;
  }

  
  
  
  
  
  PRTime tempServerTime;
  PRInt64 serverTime;
  if (aServerTime &&
      PR_ParseTimeString(aServerTime, PR_TRUE, &tempServerTime) == PR_SUCCESS) {
    serverTime = tempServerTime / PR_USEC_PER_SEC;
  } else {
    serverTime = PR_Now() / PR_USEC_PER_SEC;
  }

  
  
  mozStorageTransaction transaction(mDBState->dbConn, PR_TRUE);
 
  
  nsDependentCString cookieHeader(aCookieHeader);
  while (SetCookieInternal(aHostURI, aChannel, baseDomain, requireHostMatch,
                           cookieHeader, serverTime, aFromHttp));

  return NS_OK;
}


void
nsCookieService::NotifyRejected(nsIURI *aHostURI)
{
  if (mObserverService)
    mObserverService->NotifyObservers(aHostURI, "cookie-rejected", nsnull);
}









void
nsCookieService::NotifyChanged(nsISupports     *aSubject,
                               const PRUnichar *aData)
{
  if (mObserverService)
    mObserverService->NotifyObservers(aSubject, "cookie-changed", aData);
}






void
nsCookieService::PrefChanged(nsIPrefBranch *aPrefBranch)
{
  PRInt32 val;
  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefCookiesPermissions, &val)))
    mCookiesPermissions = (PRUint8) LIMIT(val, 0, 2, 0);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefMaxNumberOfCookies, &val)))
    mMaxNumberOfCookies = (PRUint16) LIMIT(val, 1, 0xFFFF, kMaxNumberOfCookies);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefMaxCookiesPerHost, &val)))
    mMaxCookiesPerHost = (PRUint16) LIMIT(val, 1, 0xFFFF, kMaxCookiesPerHost);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefCookiePurgeAge, &val)))
    mCookiePurgeAge = LIMIT(val, 0, PR_INT32_MAX, PR_INT32_MAX) * PR_USEC_PER_SEC;
}






NS_IMETHODIMP
nsCookieService::RemoveAll()
{
  RemoveAllFromMemory();

  
  if (mDBState->dbConn) {
    NS_ASSERTION(mDBState == &mDefaultDBState, "not in default DB state");

    nsresult rv = mDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM moz_cookies"));
    if (NS_FAILED(rv)) {
      
      nsCOMPtr<nsIFile> dbFile;
      mDBState->dbConn->GetDatabaseFile(getter_AddRefs(dbFile));
      CloseDB();
      dbFile->Remove(PR_FALSE);

      InitDB();
    }
  }

  NotifyChanged(nsnull, NS_LITERAL_STRING("cleared").get());
  return NS_OK;
}


struct nsGetEnumeratorData
{
  nsGetEnumeratorData(nsCOMArray<nsICookie> *aArray, PRInt64 aTime)
   : array(aArray)
   , currentTime(aTime) {}

  nsCOMArray<nsICookie> *array;
  PRInt64 currentTime;
};

static PLDHashOperator
COMArrayCallback(nsCookieEntry *aEntry,
                 void          *aArg)
{
  nsGetEnumeratorData *data = static_cast<nsGetEnumeratorData *>(aArg);

  const nsCookieEntry::ArrayType &cookies = aEntry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    nsCookie *cookie = cookies[i];

    
    if (cookie->Expiry() > data->currentTime)
      data->array->AppendObject(cookie);
  }
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsCookieService::GetEnumerator(nsISimpleEnumerator **aEnumerator)
{
  nsCOMArray<nsICookie> cookieList(mDBState->cookieCount);
  nsGetEnumeratorData data(&cookieList, PR_Now() / PR_USEC_PER_SEC);

  mDBState->hostTable.EnumerateEntries(COMArrayCallback, &data);

  return NS_NewArrayEnumerator(aEnumerator, cookieList);
}

NS_IMETHODIMP
nsCookieService::Add(const nsACString &aHost,
                     const nsACString &aPath,
                     const nsACString &aName,
                     const nsACString &aValue,
                     PRBool            aIsSecure,
                     PRBool            aIsHttpOnly,
                     PRBool            aIsSession,
                     PRInt64           aExpiry)
{
  
  nsCAutoString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCAutoString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 currentTimeInUsec = PR_Now();

  nsRefPtr<nsCookie> cookie =
    nsCookie::Create(aName, aValue, host, aPath,
                     aExpiry,
                     currentTimeInUsec,
                     currentTimeInUsec,
                     aIsSession,
                     aIsSecure,
                     aIsHttpOnly);
  if (!cookie) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  AddInternal(baseDomain, cookie, currentTimeInUsec, nsnull, nsnull, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::Remove(const nsACString &aHost,
                        const nsACString &aName,
                        const nsACString &aPath,
                        PRBool           aBlocked)
{
  
  nsCAutoString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsListIter matchIter;
  if (FindCookie(baseDomain,
                 host,
                 PromiseFlatCString(aName),
                 PromiseFlatCString(aPath),
                 matchIter,
                 PR_Now() / PR_USEC_PER_SEC)) {
    nsRefPtr<nsCookie> cookie = matchIter.Cookie();
    RemoveCookieFromList(matchIter);
    NotifyChanged(cookie, NS_LITERAL_STRING("deleted").get());
  }

  
  if (aBlocked && mPermissionService) {
    
    if (!host.IsEmpty() && host.First() == '.')
      host.Cut(0, 1);

    host.Insert(NS_LITERAL_CSTRING("http://"), 0);

    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), host);

    if (uri)
      mPermissionService->SetAccess(uri, nsICookiePermission::ACCESS_DENY);
  }

  return NS_OK;
}






nsresult
nsCookieService::Read()
{
  nsresult rv;

  
  {
    
    nsCOMPtr<mozIStorageStatement> stmtDeleteExpired;
    rv = mDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_cookies WHERE expiry <= ?1"),
      getter_AddRefs(stmtDeleteExpired));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmtDeleteExpired->BindInt64Parameter(0, PR_Now() / PR_USEC_PER_SEC);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = stmtDeleteExpired->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT "
      "id, "
      "name, "
      "value, "
      "host, "
      "path, "
      "expiry, "
      "lastAccessed, "
      "isSecure, "
      "isHttpOnly "
    "FROM moz_cookies"), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString baseDomain, name, value, host, path;
  PRBool hasResult;
  while (NS_SUCCEEDED(rv = stmt->ExecuteStep(&hasResult)) && hasResult) {
    PRInt64 creationID = stmt->AsInt64(0);
    
    stmt->GetUTF8String(1, name);
    stmt->GetUTF8String(2, value);
    stmt->GetUTF8String(3, host);
    stmt->GetUTF8String(4, path);

    PRInt64 expiry = stmt->AsInt64(5);
    PRInt64 lastAccessed = stmt->AsInt64(6);
    PRBool isSecure = 0 != stmt->AsInt32(7);
    PRBool isHttpOnly = 0 != stmt->AsInt32(8);

    
    rv = GetBaseDomainFromHost(host, baseDomain);
    if (NS_FAILED(rv))
      continue;

    
    nsCookie* newCookie =
      nsCookie::Create(name, value, host, path,
                       expiry,
                       lastAccessed,
                       creationID,
                       PR_FALSE,
                       isSecure,
                       isHttpOnly);
    if (!newCookie)
      return NS_ERROR_OUT_OF_MEMORY;

    if (!AddCookieToList(baseDomain, newCookie, PR_FALSE))
      
      
      
      
      
      
      
      delete newCookie;
  }

  COOKIE_LOGSTRING(PR_LOG_DEBUG, ("Read(): %ld cookies read", mDBState->cookieCount));

  return rv;
}

NS_IMETHODIMP
nsCookieService::ImportCookies(nsIFile *aCookieFile)
{
  nsresult rv;
  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), aCookieFile);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
  if (NS_FAILED(rv)) return rv;

  
  
  mozStorageTransaction transaction(mDBState->dbConn, PR_TRUE);

  static const char kTrue[] = "TRUE";

  nsCAutoString buffer, baseDomain;
  PRBool isMore = PR_TRUE;
  PRInt32 hostIndex, isDomainIndex, pathIndex, secureIndex, expiresIndex, nameIndex, cookieIndex;
  nsASingleFragmentCString::char_iterator iter;
  PRInt32 numInts;
  PRInt64 expires;
  PRBool isDomain, isHttpOnly = PR_FALSE;
  PRUint32 originalCookieCount = mDBState->cookieCount;

  PRInt64 currentTimeInUsec = PR_Now();
  PRInt64 currentTime = currentTimeInUsec / PR_USEC_PER_SEC;
  
  
  PRInt64 lastAccessedCounter = currentTimeInUsec;

  












  









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
    if (numInts != 1 || expires < currentTime) {
      continue;
    }

    isDomain = Substring(buffer, isDomainIndex, pathIndex - isDomainIndex - 1).EqualsLiteral(kTrue);
    const nsASingleFragmentCString &host = Substring(buffer, hostIndex, isDomainIndex - hostIndex - 1);
    
    
    if ((isDomain && !host.IsEmpty() && host.First() != '.') ||
        host.FindChar(':') != kNotFound) {
      continue;
    }

    
    rv = GetBaseDomainFromHost(host, baseDomain);
    if (NS_FAILED(rv))
      continue;

    
    
    
    
    nsRefPtr<nsCookie> newCookie =
      nsCookie::Create(Substring(buffer, nameIndex, cookieIndex - nameIndex - 1),
                       Substring(buffer, cookieIndex, buffer.Length() - cookieIndex),
                       host,
                       Substring(buffer, pathIndex, secureIndex - pathIndex - 1),
                       expires,
                       lastAccessedCounter,
                       currentTimeInUsec,
                       PR_FALSE,
                       Substring(buffer, secureIndex, expiresIndex - secureIndex - 1).EqualsLiteral(kTrue),
                       isHttpOnly);
    if (!newCookie) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    
    
    lastAccessedCounter--;

    if (originalCookieCount == 0)
      AddCookieToList(baseDomain, newCookie);
    else
      AddInternal(baseDomain, newCookie, currentTimeInUsec, nsnull, nsnull, PR_TRUE);
  }

  COOKIE_LOGSTRING(PR_LOG_DEBUG, ("ImportCookies(): %ld cookies imported", mDBState->cookieCount));

  return NS_OK;
}







static inline PRBool ispathdelimiter(char c) { return c == '/' || c == '?' || c == '#' || c == ';'; }


class CompareCookiesForSending
{
public:
  PRBool Equals(const nsCookie* aCookie1, const nsCookie* aCookie2) const
  {
    
    return PR_FALSE;
  }

  PRBool LessThan(const nsCookie* aCookie1, const nsCookie* aCookie2) const
  {
    
    PRInt32 result = aCookie2->Path().Length() - aCookie1->Path().Length();
    if (result != 0)
      return result < 0;

    
    
    
    
    
    return aCookie1->CreationID() < aCookie2->CreationID();
  }
};

void
nsCookieService::GetCookieInternal(nsIURI      *aHostURI,
                                   nsIChannel  *aChannel,
                                   PRBool       aHttpBound,
                                   char       **aCookie)
{
  *aCookie = nsnull;

  if (!aHostURI) {
    COOKIE_LOGFAILURE(GET_COOKIE, nsnull, nsnull, "host URI is null");
    return;
  }

  
  
  
  
  
  PRBool requireHostMatch;
  nsCAutoString baseDomain, hostFromURI, pathFromURI;
  nsresult rv = GetBaseDomain(aHostURI, baseDomain, requireHostMatch);
  if (NS_SUCCEEDED(rv))
    rv = aHostURI->GetAsciiHost(hostFromURI);
  if (NS_SUCCEEDED(rv))
    rv = aHostURI->GetPath(pathFromURI);
  
  if (!hostFromURI.IsEmpty() && hostFromURI.Last() == '.')
    hostFromURI.Truncate(hostFromURI.Length() - 1);
  if (NS_FAILED(rv)) {
    COOKIE_LOGFAILURE(GET_COOKIE, aHostURI, nsnull, "invalid host/path from URI");
    return;
  }

  
  PRUint32 cookieStatus = CheckPrefs(aHostURI, aChannel, baseDomain,
                                     requireHostMatch, nsnull);
  
  switch (cookieStatus) {
  case STATUS_REJECTED:
  case STATUS_REJECTED_WITH_ERROR:
    return;
  }

  
  
  
  PRBool isSecure;
  if (NS_FAILED(aHostURI->SchemeIs("https", &isSecure))) {
    isSecure = PR_FALSE;
  }

  nsCookie *cookie;
  nsAutoTArray<nsCookie*, 8> foundCookieList;
  PRInt64 currentTimeInUsec = PR_Now();
  PRInt64 currentTime = currentTimeInUsec / PR_USEC_PER_SEC;
  PRBool stale = PR_FALSE;

  
  nsCookieEntry *entry = mDBState->hostTable.GetEntry(baseDomain);
  if (!entry)
    return;

  
  const nsCookieEntry::ArrayType &cookies = entry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    cookie = cookies[i];

    
    
    
    
    if (cookie->RawHost() != hostFromURI &&
        !(cookie->IsDomain() && StringEndsWith(hostFromURI, cookie->Host())))
      continue;

    
    if (cookie->IsSecure() && !isSecure)
      continue;

    
    
    if (cookie->IsHttpOnly() && !aHttpBound)
      continue;

    
    PRUint32 cookiePathLen = cookie->Path().Length();
    if (cookiePathLen > 0 && cookie->Path().Last() == '/')
      --cookiePathLen;

    
    if (!StringBeginsWith(pathFromURI, Substring(cookie->Path(), 0, cookiePathLen)))
      continue;

    if (pathFromURI.Length() > cookiePathLen &&
        !ispathdelimiter(pathFromURI.CharAt(cookiePathLen))) {
      







      continue;
    }

    
    if (cookie->Expiry() <= currentTime) {
      continue;
    }

    
    foundCookieList.AppendElement(cookie);
    if (currentTimeInUsec - cookie->LastAccessed() > kCookieStaleThreshold)
      stale = PR_TRUE;
  }

  PRInt32 count = foundCookieList.Length();
  if (count == 0)
    return;

  
  
  if (stale) {
    
    
    mozStorageTransaction transaction(mDBState->dbConn, PR_TRUE);

    for (PRInt32 i = 0; i < count; ++i) {
      cookie = foundCookieList.ElementAt(i);

      if (currentTimeInUsec - cookie->LastAccessed() > kCookieStaleThreshold)
        UpdateCookieInList(cookie, currentTimeInUsec);
    }
  }

  
  
  
  foundCookieList.Sort(CompareCookiesForSending());

  nsCAutoString cookieData;
  for (PRInt32 i = 0; i < count; ++i) {
    cookie = foundCookieList.ElementAt(i);

    
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
    COOKIE_LOGSUCCESS(GET_COOKIE, aHostURI, cookieData, nsnull, nsnull);
    *aCookie = ToNewCString(cookieData);
  }
}



PRBool
nsCookieService::SetCookieInternal(nsIURI             *aHostURI,
                                   nsIChannel         *aChannel,
                                   const nsCString    &aBaseDomain,
                                   PRBool              aRequireHostMatch,
                                   nsDependentCString &aCookieHeader,
                                   PRInt64             aServerTime,
                                   PRBool              aFromHttp)
{
  
  
  nsCookieAttributes cookieAttributes;

  
  cookieAttributes.expiryTime = LL_MAXINT;

  
  
  nsDependentCString savedCookieHeader(aCookieHeader);

  
  
  PRBool newCookie = ParseAttributes(aCookieHeader, cookieAttributes);

  PRInt64 currentTimeInUsec = PR_Now();

  
  cookieAttributes.isSession = GetExpiry(cookieAttributes, aServerTime,
                                         currentTimeInUsec / PR_USEC_PER_SEC);

  
  if ((cookieAttributes.name.Length() + cookieAttributes.value.Length()) > kMaxBytesPerCookie) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, savedCookieHeader, "cookie too big (> 4kb)");
    return newCookie;
  }

  if (cookieAttributes.name.FindChar('\t') != kNotFound) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, savedCookieHeader, "invalid name character");
    return newCookie;
  }

  
  if (!CheckDomain(cookieAttributes, aHostURI, aBaseDomain, aRequireHostMatch)) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, savedCookieHeader, "failed the domain tests");
    return newCookie;
  }
  if (!CheckPath(cookieAttributes, aHostURI)) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, savedCookieHeader, "failed the path tests");
    return newCookie;
  }

  
  nsRefPtr<nsCookie> cookie =
    nsCookie::Create(cookieAttributes.name,
                     cookieAttributes.value,
                     cookieAttributes.host,
                     cookieAttributes.path,
                     cookieAttributes.expiryTime,
                     currentTimeInUsec,
                     currentTimeInUsec,
                     cookieAttributes.isSession,
                     cookieAttributes.isSecure,
                     cookieAttributes.isHttpOnly);
  if (!cookie)
    return newCookie;

  
  
  if (mPermissionService) {
    PRBool permission;
    
    
    mPermissionService->CanSetCookie(aHostURI,
                                     aChannel,
                                     static_cast<nsICookie2*>(static_cast<nsCookie*>(cookie)),
                                     &cookieAttributes.isSession,
                                     &cookieAttributes.expiryTime,
                                     &permission);
    if (!permission) {
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, savedCookieHeader, "cookie rejected by permission manager");
      NotifyRejected(aHostURI);
      return newCookie;
    }

    
    cookie->SetIsSession(cookieAttributes.isSession);
    cookie->SetExpiry(cookieAttributes.expiryTime);
  }

  
  
  AddInternal(aBaseDomain, cookie, PR_Now(), aHostURI, savedCookieHeader.get(),
              aFromHttp);
  return newCookie;
}






void
nsCookieService::AddInternal(const nsCString &aBaseDomain,
                             nsCookie        *aCookie,
                             PRInt64          aCurrentTimeInUsec,
                             nsIURI          *aHostURI,
                             const char      *aCookieHeader,
                             PRBool           aFromHttp)
{
  PRInt64 currentTime = aCurrentTimeInUsec / PR_USEC_PER_SEC;

  
  if (!aFromHttp && aCookie->IsHttpOnly()) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "cookie is httponly; coming from script");
    return;
  }

  
  
  
  mozStorageTransaction transaction(mDBState->dbConn, PR_TRUE);

  nsListIter matchIter;
  PRBool foundCookie = FindCookie(aBaseDomain, aCookie->Host(),
    aCookie->Name(), aCookie->Path(), matchIter, currentTime);

  nsRefPtr<nsCookie> oldCookie;
  if (foundCookie) {
    oldCookie = matchIter.Cookie();

    
    if (!aFromHttp && oldCookie->IsHttpOnly()) {
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "previously stored cookie is httponly; coming from script");
      return;
    }

    RemoveCookieFromList(matchIter);

    
    if (aCookie->Expiry() <= currentTime) {
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "previously stored cookie was deleted");
      NotifyChanged(oldCookie, NS_LITERAL_STRING("deleted").get());
      return;
    }

    
    if (oldCookie)
      aCookie->SetCreationID(oldCookie->CreationID());

  } else {
    
    if (aCookie->Expiry() <= currentTime) {
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, "cookie has already expired");
      return;
    }

    
    nsEnumerationData data(currentTime, LL_MAXINT);
    if (CountCookiesFromHostInternal(aBaseDomain, data) >= mMaxCookiesPerHost) {
      
      oldCookie = data.iter.Cookie();
      COOKIE_LOGEVICTED(oldCookie);
      RemoveCookieFromList(data.iter);

      NotifyChanged(oldCookie, NS_LITERAL_STRING("deleted").get());

    } else if (mDBState->cookieCount >= ADD_TEN_PERCENT(mMaxNumberOfCookies)) {
      PRInt64 maxAge = aCurrentTimeInUsec - mDBState->cookieOldestTime;
      PRInt64 purgeAge = ADD_TEN_PERCENT(mCookiePurgeAge);
      if (maxAge >= purgeAge) {
        
        
        
        
        
        
        PurgeCookies(aCurrentTimeInUsec);
      }
    }
  }

  
  AddCookieToList(aBaseDomain, aCookie);
  NotifyChanged(aCookie, foundCookie ? NS_LITERAL_STRING("changed").get()
                                     : NS_LITERAL_STRING("added").get());

  COOKIE_LOGSUCCESS(SET_COOKIE, aHostURI, aCookieHeader, aCookie, foundCookie != nsnull);
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












nsresult
nsCookieService::GetBaseDomain(nsIURI    *aHostURI,
                               nsCString &aBaseDomain,
                               PRBool    &aRequireHostMatch)
{
  
  
  nsresult rv = mTLDService->GetBaseDomain(aHostURI, 0, aBaseDomain);
  aRequireHostMatch = rv == NS_ERROR_HOST_IS_IP_ADDRESS ||
                      rv == NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS;
  if (aRequireHostMatch) {
    
    
    
    rv = aHostURI->GetAsciiHost(aBaseDomain);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!aBaseDomain.IsEmpty() && aBaseDomain.Last() == '.')
    aBaseDomain.Truncate(aBaseDomain.Length() - 1);

  
  if (aBaseDomain.IsEmpty()) {
    PRBool isFileURI = PR_FALSE;
    aHostURI->SchemeIs("file", &isFileURI);
    if (!isFileURI)
      return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}








nsresult
nsCookieService::GetBaseDomainFromHost(const nsACString &aHost,
                                       nsCString        &aBaseDomain)
{
  
  if (!aHost.IsEmpty() && aHost.Last() == '.')
    return NS_ERROR_INVALID_ARG;

  
  nsDependentCString host(aHost);
  PRBool domain = !host.IsEmpty() && host.First() == '.';
  if (domain)
    host.Rebind(host.BeginReading() + 1, host.EndReading());

  
  
  nsresult rv = mTLDService->GetBaseDomainFromHost(host, 0, aBaseDomain);
  if (rv == NS_ERROR_HOST_IS_IP_ADDRESS ||
      rv == NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS) {
    
    
    
    
    if (domain)
      return NS_ERROR_INVALID_ARG;

    aBaseDomain = host;
    return NS_OK;
  }
  return rv;
}




nsresult
nsCookieService::NormalizeHost(nsCString &aHost)
{
  if (!IsASCII(aHost)) {
    nsCAutoString host;
    nsresult rv = mIDNService->ConvertUTF8toACE(aHost, host);
    if (NS_FAILED(rv))
      return rv;

    aHost = host;
  }

  
  
  if (aHost.Length() > 1 && aHost.Last() == '.')
    aHost.Truncate(aHost.Length() - 1);

  ToLowerCase(aHost);
  return NS_OK;
}



static inline PRBool IsSubdomainOf(const nsCString &a, const nsCString &b)
{
  if (a == b)
    return PR_TRUE;
  if (a.Length() > b.Length())
    return a[a.Length() - b.Length() - 1] == '.' && StringEndsWith(a, b);
  return PR_FALSE;
}

PRBool
nsCookieService::IsForeign(const nsCString &aBaseDomain,
                           PRBool           aRequireHostMatch,
                           nsIURI          *aFirstURI)
{
  nsCAutoString firstHost;
  if (NS_FAILED(aFirstURI->GetAsciiHost(firstHost))) {
    
    return PR_TRUE;
  }

  
  if (!firstHost.IsEmpty() && firstHost.Last() == '.')
    firstHost.Truncate(firstHost.Length() - 1);

  
  
  
  
  if (aRequireHostMatch)
    return !firstHost.Equals(aBaseDomain);

  
  return !IsSubdomainOf(firstHost, aBaseDomain);
}

PRUint32
nsCookieService::CheckPrefs(nsIURI          *aHostURI,
                            nsIChannel      *aChannel,
                            const nsCString &aBaseDomain,
                            PRBool           aRequireHostMatch,
                            const char      *aCookieHeader)
{
  nsresult rv;

  
  PRBool ftp;
  if (NS_SUCCEEDED(aHostURI->SchemeIs("ftp", &ftp)) && ftp) {
    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "ftp sites cannot read cookies");
    return STATUS_REJECTED_WITH_ERROR;
  }

  
  
  if (mPermissionService) {
    nsCookieAccess access;
    rv = mPermissionService->CanAccess(aHostURI, aChannel, &access);

    
    if (NS_SUCCEEDED(rv)) {
      switch (access) {
      case nsICookiePermission::ACCESS_DENY:
        COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "cookies are blocked for this site");
        return STATUS_REJECTED;

      case nsICookiePermission::ACCESS_ALLOW:
        return STATUS_ACCEPTED;
      }
    }
  }

  
  if (mCookiesPermissions == BEHAVIOR_REJECT) {
    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "cookies are disabled");
    return STATUS_REJECTED;

  } else if (mCookiesPermissions == BEHAVIOR_REJECTFOREIGN) {
    
    if (!mPermissionService) {
      NS_WARNING("Foreign cookie blocking enabled, but nsICookiePermission unavailable! Rejecting cookie");
      COOKIE_LOGSTRING(PR_LOG_WARNING, ("CheckPrefs(): foreign blocking enabled, but nsICookiePermission unavailable! Rejecting cookie"));
      return STATUS_REJECTED;
    }

    nsCOMPtr<nsIURI> firstURI;
    rv = mPermissionService->GetOriginatingURI(aChannel, getter_AddRefs(firstURI));

    if (NS_FAILED(rv) || IsForeign(aBaseDomain, aRequireHostMatch, firstURI)) {
      COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "originating server test failed");
      return STATUS_REJECTED;
    }
  }

  
  return STATUS_ACCEPTED;
}


PRBool
nsCookieService::CheckDomain(nsCookieAttributes &aCookieAttributes,
                             nsIURI             *aHostURI,
                             const nsCString    &aBaseDomain,
                             PRBool              aRequireHostMatch)
{
  
  nsCAutoString hostFromURI;
  aHostURI->GetAsciiHost(hostFromURI);

  
  if (!hostFromURI.IsEmpty() && hostFromURI.Last() == '.')
    hostFromURI.Truncate(hostFromURI.Length() - 1);

  
  if (!aCookieAttributes.host.IsEmpty()) {
    
    if (aCookieAttributes.host.First() == '.')
      aCookieAttributes.host.Cut(0, 1);

    
    ToLowerCase(aCookieAttributes.host);

    
    
    
    
    
    if (aRequireHostMatch)
      return hostFromURI.Equals(aCookieAttributes.host);

    
    
    if (IsSubdomainOf(aCookieAttributes.host, aBaseDomain) &&
        IsSubdomainOf(hostFromURI, aCookieAttributes.host)) {
      
      aCookieAttributes.host.Insert(NS_LITERAL_CSTRING("."), 0);
      return PR_TRUE;
    }

    






    return PR_FALSE;
  }

  
  aCookieAttributes.host = hostFromURI;
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

#if 0
  } else {
    





    
    nsCAutoString pathFromURI;
    if (NS_FAILED(aHostURI->GetPath(pathFromURI)) ||
        !StringBeginsWith(pathFromURI, aCookieAttributes.path)) {
      return PR_FALSE;
    }
#endif
  }

  if (aCookieAttributes.path.Length() > kMaxBytesPerPath ||
      aCookieAttributes.path.FindChar('\t') != kNotFound )
    return PR_FALSE;

  return PR_TRUE;
}

PRBool
nsCookieService::GetExpiry(nsCookieAttributes &aCookieAttributes,
                           PRInt64             aServerTime,
                           PRInt64             aCurrentTime)
{
  






  PRInt64 delta;

  
  if (!aCookieAttributes.maxage.IsEmpty()) {
    
    PRInt64 maxage;
    PRInt32 numInts = PR_sscanf(aCookieAttributes.maxage.get(), "%lld", &maxage);

    
    if (numInts != 1) {
      return PR_TRUE;
    }

    delta = maxage;

  
  } else if (!aCookieAttributes.expires.IsEmpty()) {
    PRTime tempExpires;
    PRInt64 expires;

    
    if (PR_ParseTimeString(aCookieAttributes.expires.get(), PR_TRUE, &tempExpires) == PR_SUCCESS) {
      expires = tempExpires / PR_USEC_PER_SEC;
    } else {
      return PR_TRUE;
    }

    delta = expires - aServerTime;

  
  } else {
    return PR_TRUE;
  }

  
  
  aCookieAttributes.expiryTime = aCurrentTime + delta;

  return PR_FALSE;
}






void
nsCookieService::RemoveAllFromMemory()
{
  
  
  mDBState->hostTable.Clear();
  mDBState->cookieCount = 0;
  mDBState->cookieOldestTime = LL_MAXINT;
}



struct nsPurgeData
{
  typedef nsTArray<nsListIter> ArrayType;

  nsPurgeData(PRInt64 aCurrentTime,
              PRInt64 aPurgeTime,
              ArrayType &aPurgeList,
              nsIMutableArray *aRemovedList)
   : currentTime(aCurrentTime)
   , purgeTime(aPurgeTime)
   , oldestTime(LL_MAXINT)
   , purgeList(aPurgeList)
   , removedList(aRemovedList) {}

  
  PRInt64 currentTime;

  
  PRInt64 purgeTime;

  
  PRInt64 oldestTime;

  
  ArrayType &purgeList;

  
  nsIMutableArray *removedList;
};


class CompareCookiesByAge {
public:
  PRBool Equals(const nsListIter &a, const nsListIter &b) const
  {
    
    return PR_FALSE;
  }

  PRBool LessThan(const nsListIter &a, const nsListIter &b) const
  {
    
    PRInt64 result = a.Cookie()->LastAccessed() - b.Cookie()->LastAccessed();
    if (result != 0)
      return result < 0;

    return a.Cookie()->CreationID() < b.Cookie()->CreationID();
  }
};


class CompareCookiesByIndex {
public:
  PRBool Equals(const nsListIter &a, const nsListIter &b) const
  {
    return PR_FALSE;
  }

  PRBool LessThan(const nsListIter &a, const nsListIter &b) const
  {
    
    if (a.entry != b.entry)
      return a.entry < b.entry;

    return a.index < b.index;
  }
};

PLDHashOperator
purgeCookiesCallback(nsCookieEntry *aEntry,
                     void          *aArg)
{
  nsPurgeData &data = *static_cast<nsPurgeData*>(aArg);

  const nsCookieEntry::ArrayType &cookies = aEntry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ) {
    nsListIter iter(aEntry, i);
    nsCookie *cookie = cookies[i];

    
    if (cookie->Expiry() <= data.currentTime) {
      data.removedList->AppendElement(cookie, PR_FALSE);
      COOKIE_LOGEVICTED(cookie);

      
      nsCookieService::gCookieService->RemoveCookieFromList(iter);

    } else {
      
      if (cookie->LastAccessed() <= data.purgeTime) {
        data.purgeList.AppendElement(iter);

      } else if (cookie->LastAccessed() < data.oldestTime) {
        
        data.oldestTime = cookie->LastAccessed();
      }

      ++i;
    }
  }
  return PL_DHASH_NEXT;
}


void
nsCookieService::PurgeCookies(PRInt64 aCurrentTimeInUsec)
{
  NS_ASSERTION(mDBState->hostTable.Count() > 0, "table is empty");
#ifdef PR_LOGGING
  PRUint32 initialCookieCount = mDBState->cookieCount;
  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("PurgeCookies(): beginning purge with %ld cookies and %lld age",
     mDBState->cookieCount, aCurrentTimeInUsec - mDBState->cookieOldestTime));
#endif

  nsAutoTArray<nsListIter, kMaxNumberOfCookies> purgeList;

  nsCOMPtr<nsIMutableArray> removedList = do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (!removedList)
    return;

  nsPurgeData data(aCurrentTimeInUsec / PR_USEC_PER_SEC,
    aCurrentTimeInUsec - mCookiePurgeAge, purgeList, removedList);
  mDBState->hostTable.EnumerateEntries(purgeCookiesCallback, &data);

#ifdef PR_LOGGING
  PRUint32 postExpiryCookieCount = mDBState->cookieCount;
#endif

  
  
  purgeList.Sort(CompareCookiesByAge());

  
  PRUint32 excess = mDBState->cookieCount - mMaxNumberOfCookies;
  if (purgeList.Length() > excess) {
    
    data.oldestTime = purgeList[excess].Cookie()->LastAccessed();

    purgeList.SetLength(excess);
  }

  
  
  
  purgeList.Sort(CompareCookiesByIndex());
  for (nsPurgeData::ArrayType::index_type i = purgeList.Length(); i--; ) {
    nsCookie *cookie = purgeList[i].Cookie();
    removedList->AppendElement(cookie, PR_FALSE);
    COOKIE_LOGEVICTED(cookie);

    RemoveCookieFromList(purgeList[i]);
  }

  
  NotifyChanged(removedList, NS_LITERAL_STRING("batch-deleted").get());

  
  mDBState->cookieOldestTime = data.oldestTime;

  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("PurgeCookies(): %ld expired; %ld purged; %ld remain; %lld oldest age",
     initialCookieCount - postExpiryCookieCount,
     mDBState->cookieCount - postExpiryCookieCount,
     mDBState->cookieCount,
     aCurrentTimeInUsec - mDBState->cookieOldestTime));
}



NS_IMETHODIMP
nsCookieService::CookieExists(nsICookie2 *aCookie,
                              PRBool     *aFoundCookie)
{
  NS_ENSURE_ARG_POINTER(aCookie);

  nsCAutoString host, name, path;
  nsresult rv = aCookie->GetHost(host);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCookie->GetName(name);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCookie->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsListIter iter;
  *aFoundCookie = FindCookie(baseDomain, host, name, path, iter,
                             PR_Now() / PR_USEC_PER_SEC);
  return NS_OK;
}



PRUint32
nsCookieService::CountCookiesFromHostInternal(const nsCString   &aBaseDomain,
                                              nsEnumerationData &aData)
{
  nsCookieEntry *entry = mDBState->hostTable.GetEntry(aBaseDomain);
  if (!entry)
    return 0;

  PRUint32 countFromHost = 0;
  const nsCookieEntry::ArrayType &cookies = entry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    nsCookie *cookie = cookies[i];

    
    if (cookie->Expiry() > aData.currentTime) {
      ++countFromHost;

      
      if (aData.oldestTime > cookie->LastAccessed()) {
        aData.oldestTime = cookie->LastAccessed();
        aData.iter = nsListIter(entry, i);
      }
    }
  }

  return countFromHost;
}



NS_IMETHODIMP
nsCookieService::CountCookiesFromHost(const nsACString &aHost,
                                      PRUint32         *aCountFromHost)
{
  
  nsCAutoString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsEnumerationData data(PR_Now() / PR_USEC_PER_SEC, LL_MININT);
  *aCountFromHost = CountCookiesFromHostInternal(baseDomain, data);
  return NS_OK;
}



NS_IMETHODIMP
nsCookieService::GetCookiesFromHost(const nsACString     &aHost,
                                    nsISimpleEnumerator **aEnumerator)
{
  
  nsCAutoString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMArray<nsICookie> cookieList(mMaxCookiesPerHost);
  PRInt64 currentTime = PR_Now() / PR_USEC_PER_SEC;

  nsCookieEntry *entry = mDBState->hostTable.GetEntry(baseDomain);
  if (!entry)
    return NS_NewEmptyEnumerator(aEnumerator);

  const nsCookieEntry::ArrayType &cookies = entry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    nsCookie *cookie = cookies[i];

    
    if (cookie->Expiry() > currentTime)
      cookieList.AppendObject(cookie);
  }

  return NS_NewArrayEnumerator(aEnumerator, cookieList);
}


PRBool
nsCookieService::FindCookie(const nsCString      &aBaseDomain,
                            const nsAFlatCString &aHost,
                            const nsAFlatCString &aName,
                            const nsAFlatCString &aPath,
                            nsListIter           &aIter,
                            PRInt64               aCurrentTime)
{
  nsCookieEntry *entry = mDBState->hostTable.GetEntry(aBaseDomain);
  if (!entry)
    return PR_FALSE;

  const nsCookieEntry::ArrayType &cookies = entry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    nsCookie *cookie = cookies[i];

    if (cookie->Expiry() > aCurrentTime &&
        aHost.Equals(cookie->Host()) &&
        aPath.Equals(cookie->Path()) &&
        aName.Equals(cookie->Name())) {
      aIter = nsListIter(entry, i);
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}


void
nsCookieService::RemoveCookieFromList(const nsListIter &aIter)
{
  
  if (!aIter.Cookie()->IsSession() && mDBState->dbConn) {
    
    mozStorageStatementScoper scoper(mDBState->stmtDelete);

    PRInt64 creationID = aIter.Cookie()->CreationID();
    nsresult rv = mDBState->stmtDelete->BindInt64Parameter(0, creationID);
    if (NS_SUCCEEDED(rv)) {
      PRBool hasResult;
      rv = mDBState->stmtDelete->ExecuteStep(&hasResult);
    }

    if (NS_FAILED(rv)) {
      NS_WARNING("db remove failed!");
      COOKIE_LOGSTRING(PR_LOG_WARNING, ("RemoveCookieFromList(): removing from db gave error %x", rv));
    }
  }

  if (aIter.entry->GetCookies().Length() == 1) {
    
    
    
    mDBState->hostTable.RawRemoveEntry(aIter.entry);

  } else {
    
    aIter.entry->GetCookies().RemoveElementAt(aIter.index);
  }

  --mDBState->cookieCount;
}

static nsresult
bindCookieParameters(mozIStorageStatement *aStmt, const nsCookie *aCookie)
{
  nsresult rv;

  rv = aStmt->BindInt64Parameter(0, aCookie->CreationID());
  if (NS_FAILED(rv)) return rv;

  rv = aStmt->BindUTF8StringParameter(1, aCookie->Name());
  if (NS_FAILED(rv)) return rv;
  
  rv = aStmt->BindUTF8StringParameter(2, aCookie->Value());
  if (NS_FAILED(rv)) return rv;
  
  rv = aStmt->BindUTF8StringParameter(3, aCookie->Host());
  if (NS_FAILED(rv)) return rv;
  
  rv = aStmt->BindUTF8StringParameter(4, aCookie->Path());
  if (NS_FAILED(rv)) return rv;
  
  rv = aStmt->BindInt64Parameter(5, aCookie->Expiry());
  if (NS_FAILED(rv)) return rv;
  
  rv = aStmt->BindInt64Parameter(6, aCookie->LastAccessed());
  if (NS_FAILED(rv)) return rv;
  
  rv = aStmt->BindInt32Parameter(7, aCookie->IsSecure());
  if (NS_FAILED(rv)) return rv;
  
  rv = aStmt->BindInt32Parameter(8, aCookie->IsHttpOnly());
  return rv;
}

PRBool
nsCookieService::AddCookieToList(const nsCString &aBaseDomain,
                                 nsCookie        *aCookie,
                                 PRBool           aWriteToDB)
{
  nsCookieEntry *entry = mDBState->hostTable.PutEntry(aBaseDomain);
  if (!entry) {
    NS_ERROR("can't insert element into a null entry!");
    return PR_FALSE;
  }

  entry->GetCookies().AppendElement(aCookie);
  ++mDBState->cookieCount;

  
  if (aCookie->LastAccessed() < mDBState->cookieOldestTime)
    mDBState->cookieOldestTime = aCookie->LastAccessed();

  
  if (aWriteToDB && !aCookie->IsSession() && mDBState->dbConn) {
    
    mozStorageStatementScoper scoper(mDBState->stmtInsert);

    nsresult rv = bindCookieParameters(mDBState->stmtInsert, aCookie);
    if (NS_SUCCEEDED(rv)) {
      PRBool hasResult;
      rv = mDBState->stmtInsert->ExecuteStep(&hasResult);
    }

    if (NS_FAILED(rv)) {
      NS_WARNING("db insert failed!");
      COOKIE_LOGSTRING(PR_LOG_WARNING, ("AddCookieToList(): adding to db gave error %x", rv));
    }
  }

  return PR_TRUE;
}

void
nsCookieService::UpdateCookieInList(nsCookie *aCookie, PRInt64 aLastAccessed)
{
  
  aCookie->SetLastAccessed(aLastAccessed);

  
  if (!aCookie->IsSession() && mDBState->dbConn) {
    
    mozStorageStatementScoper scoper(mDBState->stmtUpdate);

    nsresult rv = mDBState->stmtUpdate->BindInt64Parameter(0, aLastAccessed);
    if (NS_SUCCEEDED(rv)) {
      rv = mDBState->stmtUpdate->BindInt64Parameter(1, aCookie->CreationID());
      if (NS_SUCCEEDED(rv)) {
        PRBool hasResult;
        rv = mDBState->stmtUpdate->ExecuteStep(&hasResult);
      }
    }

    if (NS_FAILED(rv)) {
      NS_WARNING("db update failed!");
      COOKIE_LOGSTRING(PR_LOG_WARNING, ("UpdateCookieInList(): updating db gave error %x", rv));
    }
  }
}

