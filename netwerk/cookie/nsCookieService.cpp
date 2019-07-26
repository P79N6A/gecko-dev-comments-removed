





#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Likely.h"

#ifdef MOZ_LOGGING

#define FORCE_PR_LOG
#endif

#include "mozilla/net/CookieServiceChild.h"
#include "mozilla/net/NeckoCommon.h"

#include "nsCookieService.h"
#include "nsIServiceManager.h"

#include "nsIIOService.h"
#include "nsIPrefBranch.h"
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
#include "mozIThirdPartyUtil.h"

#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsIMutableArray.h"
#include "nsArrayEnumerator.h"
#include "nsEnumeratorUtils.h"
#include "nsAutoPtr.h"
#include "nsReadableUtils.h"
#include "nsCRT.h"
#include "prprf.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsNetCID.h"
#include "mozilla/storage.h"
#include "mozilla/AutoRestore.h"
#include "nsIAppsService.h"
#include "mozIApplication.h"

using namespace mozilla;
using namespace mozilla::net;





#define DEFAULT_APP_KEY(baseDomain) \
        nsCookieKey(baseDomain, NECKO_NO_APP_ID, false)






static nsCookieService *gCookieService;



static const char kHttpOnlyPrefix[] = "#HttpOnly_";

#define COOKIES_FILE "cookies.sqlite"
#define COOKIES_SCHEMA_VERSION 5

static const int64_t kCookieStaleThreshold = 60 * PR_USEC_PER_SEC; 
static const int64_t kCookiePurgeAge =
  int64_t(30 * 24 * 60 * 60) * PR_USEC_PER_SEC; 

static const char kOldCookieFileName[] = "cookies.txt";

#undef  LIMIT
#define LIMIT(x, low, high, default) ((x) >= (low) && (x) <= (high) ? (x) : (default))

#undef  ADD_TEN_PERCENT
#define ADD_TEN_PERCENT(i) static_cast<uint32_t>((i) + (i)/10)



static const uint32_t kMaxNumberOfCookies = 3000;
static const uint32_t kMaxCookiesPerHost  = 150;
static const uint32_t kMaxBytesPerCookie  = 4096;
static const uint32_t kMaxBytesPerPath    = 1024;


static const uint32_t BEHAVIOR_ACCEPT        = 0; 
static const uint32_t BEHAVIOR_REJECTFOREIGN = 1; 
static const uint32_t BEHAVIOR_REJECT        = 2; 
static const uint32_t BEHAVIOR_LIMITFOREIGN  = 3; 
                                                  


static const char kPrefCookieBehavior[]     = "network.cookie.cookieBehavior";
static const char kPrefMaxNumberOfCookies[] = "network.cookie.maxNumber";
static const char kPrefMaxCookiesPerHost[]  = "network.cookie.maxPerHost";
static const char kPrefCookiePurgeAge[]     = "network.cookie.purgeAge";
static const char kPrefThirdPartySession[]  = "network.cookie.thirdparty.sessionOnly";

static void
bindCookieParameters(mozIStorageBindingParamsArray *aParamsArray,
                     const nsCookieKey &aKey,
                     const nsCookie *aCookie);


struct nsCookieAttributes
{
  nsAutoCString name;
  nsAutoCString value;
  nsAutoCString host;
  nsAutoCString path;
  nsAutoCString expires;
  nsAutoCString maxage;
  int64_t expiryTime;
  bool isSession;
  bool isSecure;
  bool isHttpOnly;
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







#ifdef MOZ_LOGGING






#include "prlog.h"
#endif


#define SET_COOKIE true
#define GET_COOKIE false

#ifdef PR_LOGGING
static PRLogModuleInfo *
GetCookieLog()
{
  static PRLogModuleInfo *sCookieLog;
  if (!sCookieLog)
    sCookieLog = PR_NewLogModule("cookie");
  return sCookieLog;
}

#define COOKIE_LOGFAILURE(a, b, c, d)    LogFailure(a, b, c, d)
#define COOKIE_LOGSUCCESS(a, b, c, d, e) LogSuccess(a, b, c, d, e)

#define COOKIE_LOGEVICTED(a, details)          \
  PR_BEGIN_MACRO                               \
  if (PR_LOG_TEST(GetCookieLog(), PR_LOG_DEBUG))  \
      LogEvicted(a, details);                  \
  PR_END_MACRO

#define COOKIE_LOGSTRING(lvl, fmt)   \
  PR_BEGIN_MACRO                     \
    PR_LOG(GetCookieLog(), lvl, fmt);  \
    PR_LOG(GetCookieLog(), lvl, ("\n")); \
  PR_END_MACRO

static void
LogFailure(bool aSetCookie, nsIURI *aHostURI, const char *aCookieString, const char *aReason)
{
  
  if (!PR_LOG_TEST(GetCookieLog(), PR_LOG_WARNING))
    return;

  nsAutoCString spec;
  if (aHostURI)
    aHostURI->GetAsciiSpec(spec);

  PR_LOG(GetCookieLog(), PR_LOG_WARNING,
    ("===== %s =====\n", aSetCookie ? "COOKIE NOT ACCEPTED" : "COOKIE NOT SENT"));
  PR_LOG(GetCookieLog(), PR_LOG_WARNING,("request URL: %s\n", spec.get()));
  if (aSetCookie)
    PR_LOG(GetCookieLog(), PR_LOG_WARNING,("cookie string: %s\n", aCookieString));

  PRExplodedTime explodedTime;
  PR_ExplodeTime(PR_Now(), PR_GMTParameters, &explodedTime);
  char timeString[40];
  PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);

  PR_LOG(GetCookieLog(), PR_LOG_WARNING,("current time: %s", timeString));
  PR_LOG(GetCookieLog(), PR_LOG_WARNING,("rejected because %s\n", aReason));
  PR_LOG(GetCookieLog(), PR_LOG_WARNING,("\n"));
}

static void
LogCookie(nsCookie *aCookie)
{
  PRExplodedTime explodedTime;
  PR_ExplodeTime(PR_Now(), PR_GMTParameters, &explodedTime);
  char timeString[40];
  PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);

  PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("current time: %s", timeString));

  if (aCookie) {
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("----------------\n"));
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("name: %s\n", aCookie->Name().get()));
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("value: %s\n", aCookie->Value().get()));
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("%s: %s\n", aCookie->IsDomain() ? "domain" : "host", aCookie->Host().get()));
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("path: %s\n", aCookie->Path().get()));

    PR_ExplodeTime(aCookie->Expiry() * int64_t(PR_USEC_PER_SEC),
                   PR_GMTParameters, &explodedTime);
    PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,
      ("expires: %s%s", timeString, aCookie->IsSession() ? " (at end of session)" : ""));

    PR_ExplodeTime(aCookie->CreationTime(), PR_GMTParameters, &explodedTime);
    PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("created: %s", timeString));

    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("is secure: %s\n", aCookie->IsSecure() ? "true" : "false"));
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("is httpOnly: %s\n", aCookie->IsHttpOnly() ? "true" : "false"));
  }
}

static void
LogSuccess(bool aSetCookie, nsIURI *aHostURI, const char *aCookieString, nsCookie *aCookie, bool aReplacing)
{
  
  if (!PR_LOG_TEST(GetCookieLog(), PR_LOG_DEBUG)) {
    return;
  }

  nsAutoCString spec;
  if (aHostURI)
    aHostURI->GetAsciiSpec(spec);

  PR_LOG(GetCookieLog(), PR_LOG_DEBUG,
    ("===== %s =====\n", aSetCookie ? "COOKIE ACCEPTED" : "COOKIE SENT"));
  PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("request URL: %s\n", spec.get()));
  PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("cookie string: %s\n", aCookieString));
  if (aSetCookie)
    PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("replaces existing cookie: %s\n", aReplacing ? "true" : "false"));

  LogCookie(aCookie);

  PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("\n"));
}

static void
LogEvicted(nsCookie *aCookie, const char* details)
{
  PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("===== COOKIE EVICTED =====\n"));
  PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("%s\n", details));

  LogCookie(aCookie);

  PR_LOG(GetCookieLog(), PR_LOG_DEBUG,("\n"));
}


static inline void
LogFailure(bool aSetCookie, nsIURI *aHostURI, const nsAFlatCString &aCookieString, const char *aReason)
{
  LogFailure(aSetCookie, aHostURI, aCookieString.get(), aReason);
}

static inline void
LogSuccess(bool aSetCookie, nsIURI *aHostURI, const nsAFlatCString &aCookieString, nsCookie *aCookie, bool aReplacing)
{
  LogSuccess(aSetCookie, aHostURI, aCookieString.get(), aCookie, aReplacing);
}

#else
#define COOKIE_LOGFAILURE(a, b, c, d)    PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define COOKIE_LOGSUCCESS(a, b, c, d, e) PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define COOKIE_LOGEVICTED(a, b)          PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#define COOKIE_LOGSTRING(a, b)           PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#endif

#ifdef DEBUG
#define NS_ASSERT_SUCCESS(res)                                               \
  PR_BEGIN_MACRO                                                             \
  nsresult __rv = res; /* Do not evaluate |res| more than once! */           \
  if (NS_FAILED(__rv)) {                                                     \
    char *msg = PR_smprintf("NS_ASSERT_SUCCESS(%s) failed with result 0x%X", \
                            #res, __rv);                                     \
    NS_ASSERTION(NS_SUCCEEDED(__rv), msg);                                   \
    PR_smprintf_free(msg);                                                   \
  }                                                                          \
  PR_END_MACRO
#else
#define NS_ASSERT_SUCCESS(res) PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#endif






class DBListenerErrorHandler : public mozIStorageStatementCallback
{
protected:
  DBListenerErrorHandler(DBState* dbState) : mDBState(dbState) { }
  nsRefPtr<DBState> mDBState;
  virtual const char *GetOpType() = 0;

public:
  NS_IMETHOD HandleError(mozIStorageError* aError)
  {
    int32_t result = -1;
    aError->GetResult(&result);

#ifdef PR_LOGGING
    nsAutoCString message;
    aError->GetMessage(message);
    COOKIE_LOGSTRING(PR_LOG_WARNING,
      ("DBListenerErrorHandler::HandleError(): Error %d occurred while "
       "performing operation '%s' with message '%s'; rebuilding database.",
       result, GetOpType(), message.get()));
#endif

    
    gCookieService->HandleCorruptDB(mDBState);

    return NS_OK;
  }
};





class InsertCookieDBListener MOZ_FINAL : public DBListenerErrorHandler
{
protected:
  virtual const char *GetOpType() { return "INSERT"; }

public:
  NS_DECL_ISUPPORTS

  InsertCookieDBListener(DBState* dbState) : DBListenerErrorHandler(dbState) { }
  NS_IMETHOD HandleResult(mozIStorageResultSet*)
  {
    NS_NOTREACHED("Unexpected call to InsertCookieDBListener::HandleResult");
    return NS_OK;
  }
  NS_IMETHOD HandleCompletion(uint16_t aReason)
  {
    
    
    if (mDBState->corruptFlag == DBState::REBUILDING &&
        aReason == mozIStorageStatementCallback::REASON_FINISHED) {
      COOKIE_LOGSTRING(PR_LOG_DEBUG,
        ("InsertCookieDBListener::HandleCompletion(): rebuild complete"));
      mDBState->corruptFlag = DBState::OK;
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(InsertCookieDBListener, mozIStorageStatementCallback)





class UpdateCookieDBListener MOZ_FINAL : public DBListenerErrorHandler
{
protected:
  virtual const char *GetOpType() { return "UPDATE"; }

public:
  NS_DECL_ISUPPORTS

  UpdateCookieDBListener(DBState* dbState) : DBListenerErrorHandler(dbState) { }
  NS_IMETHOD HandleResult(mozIStorageResultSet*)
  {
    NS_NOTREACHED("Unexpected call to UpdateCookieDBListener::HandleResult");
    return NS_OK;
  }
  NS_IMETHOD HandleCompletion(uint16_t aReason)
  {
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(UpdateCookieDBListener, mozIStorageStatementCallback)





class RemoveCookieDBListener MOZ_FINAL : public DBListenerErrorHandler
{
protected:
  virtual const char *GetOpType() { return "REMOVE"; }

public:
  NS_DECL_ISUPPORTS

  RemoveCookieDBListener(DBState* dbState) : DBListenerErrorHandler(dbState) { }
  NS_IMETHOD HandleResult(mozIStorageResultSet*)
  {
    NS_NOTREACHED("Unexpected call to RemoveCookieDBListener::HandleResult");
    return NS_OK;
  }
  NS_IMETHOD HandleCompletion(uint16_t aReason)
  {
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(RemoveCookieDBListener, mozIStorageStatementCallback)





class ReadCookieDBListener MOZ_FINAL : public DBListenerErrorHandler
{
protected:
  virtual const char *GetOpType() { return "READ"; }
  bool mCanceled;

public:
  NS_DECL_ISUPPORTS

  ReadCookieDBListener(DBState* dbState)
    : DBListenerErrorHandler(dbState)
    , mCanceled(false)
  {
  }

  void Cancel() { mCanceled = true; }

  NS_IMETHOD HandleResult(mozIStorageResultSet *aResult)
  {
    nsCOMPtr<mozIStorageRow> row;

    while (1) {
      DebugOnly<nsresult> rv = aResult->GetNextRow(getter_AddRefs(row));
      NS_ASSERT_SUCCESS(rv);

      if (!row)
        break;

      CookieDomainTuple *tuple = mDBState->hostArray.AppendElement();
      row->GetUTF8String(9, tuple->key.mBaseDomain);
      tuple->key.mAppId = static_cast<uint32_t>(row->AsInt32(10));
      tuple->key.mInBrowserElement = static_cast<bool>(row->AsInt32(11));
      tuple->cookie = gCookieService->GetCookieFromRow(row);
    }

    return NS_OK;
  }
  NS_IMETHOD HandleCompletion(uint16_t aReason)
  {
    
    
    
    
    
    

    if (mCanceled) {
      
      
      aReason = mozIStorageStatementCallback::REASON_CANCELED;
    }

    switch (aReason) {
    case mozIStorageStatementCallback::REASON_FINISHED:
      gCookieService->AsyncReadComplete();
      break;
    case mozIStorageStatementCallback::REASON_CANCELED:
      
      
      COOKIE_LOGSTRING(PR_LOG_DEBUG, ("Read canceled"));
      break;
    case mozIStorageStatementCallback::REASON_ERROR:
      
      
      COOKIE_LOGSTRING(PR_LOG_DEBUG, ("Read error"));
      break;
    default:
      NS_NOTREACHED("invalid reason");
    }
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(ReadCookieDBListener, mozIStorageStatementCallback)






class CloseCookieDBListener MOZ_FINAL :  public mozIStorageCompletionCallback
{
public:
  CloseCookieDBListener(DBState* dbState) : mDBState(dbState) { }
  nsRefPtr<DBState> mDBState;
  NS_DECL_ISUPPORTS

  NS_IMETHOD Complete()
  {
    gCookieService->HandleDBClosed(mDBState);
    return NS_OK;
  }
};

NS_IMPL_ISUPPORTS1(CloseCookieDBListener, mozIStorageCompletionCallback)

namespace {

class AppClearDataObserver MOZ_FINAL : public nsIObserver {
public:
  NS_DECL_ISUPPORTS

  
  NS_IMETHODIMP
  Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *data)
  {
    MOZ_ASSERT(!nsCRT::strcmp(aTopic, TOPIC_WEB_APP_CLEAR_DATA));

    uint32_t appId = NECKO_UNKNOWN_APP_ID;
    bool browserOnly = false;
    nsresult rv = NS_GetAppInfoFromClearDataNotification(aSubject, &appId,
                                                         &browserOnly);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsICookieManager2> cookieManager
      = do_GetService(NS_COOKIEMANAGER_CONTRACTID);
    MOZ_ASSERT(cookieManager);
    return cookieManager->RemoveCookiesForApp(appId, browserOnly);
  }
};

NS_IMPL_ISUPPORTS1(AppClearDataObserver, nsIObserver)

} 






nsICookieService*
nsCookieService::GetXPCOMSingleton()
{
  if (IsNeckoChild())
    return CookieServiceChild::GetSingleton();

  return GetSingleton();
}

nsCookieService*
nsCookieService::GetSingleton()
{
  NS_ASSERTION(!IsNeckoChild(), "not a parent process");

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

 void
nsCookieService::AppClearDataObserverInit()
{
  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1");
  nsCOMPtr<AppClearDataObserver> obs = new AppClearDataObserver();
  observerService->AddObserver(obs, TOPIC_WEB_APP_CLEAR_DATA,
                                false);
}






NS_IMPL_ISUPPORTS5(nsCookieService,
                   nsICookieService,
                   nsICookieManager,
                   nsICookieManager2,
                   nsIObserver,
                   nsISupportsWeakReference)

nsCookieService::nsCookieService()
 : mDBState(NULL)
 , mCookieBehavior(BEHAVIOR_ACCEPT)
 , mThirdPartySession(false)
 , mMaxNumberOfCookies(kMaxNumberOfCookies)
 , mMaxCookiesPerHost(kMaxCookiesPerHost)
 , mCookiePurgeAge(kCookiePurgeAge)
{
}

nsresult
nsCookieService::Init()
{
  nsresult rv;
  mTLDService = do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mIDNService = do_GetService(NS_IDNSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mThirdPartyUtil = do_GetService(THIRDPARTYUTIL_CONTRACTID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPrefBranch> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefBranch) {
    prefBranch->AddObserver(kPrefCookieBehavior,     this, true);
    prefBranch->AddObserver(kPrefMaxNumberOfCookies, this, true);
    prefBranch->AddObserver(kPrefMaxCookiesPerHost,  this, true);
    prefBranch->AddObserver(kPrefCookiePurgeAge,     this, true);
    prefBranch->AddObserver(kPrefThirdPartySession,  this, true);
    PrefChanged(prefBranch);
  }

  mStorageService = do_GetService("@mozilla.org/storage/service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  InitDBStates();

  mObserverService = mozilla::services::GetObserverService();
  NS_ENSURE_STATE(mObserverService);
  mObserverService->AddObserver(this, "profile-before-change", true);
  mObserverService->AddObserver(this, "profile-do-change", true);
  mObserverService->AddObserver(this, "last-pb-context-exited", true);

  mPermissionService = do_GetService(NS_COOKIEPERMISSION_CONTRACTID);
  if (!mPermissionService) {
    NS_WARNING("nsICookiePermission implementation not available - some features won't work!");
    COOKIE_LOGSTRING(PR_LOG_WARNING, ("Init(): nsICookiePermission implementation not available"));
  }

  return NS_OK;
}

void
nsCookieService::InitDBStates()
{
  NS_ASSERTION(!mDBState, "already have a DBState");
  NS_ASSERTION(!mDefaultDBState, "already have a default DBState");
  NS_ASSERTION(!mPrivateDBState, "already have a private DBState");

  
  mDefaultDBState = new DBState();
  mDBState = mDefaultDBState;

  mPrivateDBState = new DBState();

  
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
    getter_AddRefs(mDefaultDBState->cookieFile));
  if (NS_FAILED(rv)) {
    
    COOKIE_LOGSTRING(PR_LOG_WARNING,
      ("InitDBStates(): couldn't get cookie file"));
    return;
  }
  mDefaultDBState->cookieFile->AppendNative(NS_LITERAL_CSTRING(COOKIES_FILE));

  
  
  OpenDBResult result = TryInitDB(false);
  if (result == RESULT_RETRY) {
    
    
    COOKIE_LOGSTRING(PR_LOG_WARNING, ("InitDBStates(): retrying TryInitDB()"));

    CloseDefaultDBConnection();
    result = TryInitDB(true);
    if (result == RESULT_RETRY) {
      
      result = RESULT_FAILURE;
    }
  }

  if (result == RESULT_FAILURE) {
    COOKIE_LOGSTRING(PR_LOG_WARNING,
      ("InitDBStates(): TryInitDB() failed, closing connection"));

    
    
    CloseDefaultDBConnection();
  }
}













OpenDBResult
nsCookieService::TryInitDB(bool aRecreateDB)
{
  NS_ASSERTION(!mDefaultDBState->dbConn, "nonnull dbConn");
  NS_ASSERTION(!mDefaultDBState->stmtInsert, "nonnull stmtInsert");
  NS_ASSERTION(!mDefaultDBState->insertListener, "nonnull insertListener");
  NS_ASSERTION(!mDefaultDBState->syncConn, "nonnull syncConn");

  
  
  
  nsresult rv;
  if (aRecreateDB) {
    nsCOMPtr<nsIFile> backupFile;
    mDefaultDBState->cookieFile->Clone(getter_AddRefs(backupFile));
    rv = backupFile->MoveToNative(NULL,
      NS_LITERAL_CSTRING(COOKIES_FILE ".bak"));
    NS_ENSURE_SUCCESS(rv, RESULT_FAILURE);
  }

  
  
  
  rv = mStorageService->OpenUnsharedDatabase(mDefaultDBState->cookieFile,
    getter_AddRefs(mDefaultDBState->dbConn));
  NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  
  mDefaultDBState->insertListener = new InsertCookieDBListener(mDefaultDBState);
  mDefaultDBState->updateListener = new UpdateCookieDBListener(mDefaultDBState);
  mDefaultDBState->removeListener = new RemoveCookieDBListener(mDefaultDBState);
  mDefaultDBState->closeListener = new CloseCookieDBListener(mDefaultDBState);

  
  mDefaultDBState->dbConn->SetGrowthIncrement(512 * 1024, EmptyCString());

  bool tableExists = false;
  mDefaultDBState->dbConn->TableExists(NS_LITERAL_CSTRING("moz_cookies"),
    &tableExists);
  if (!tableExists) {
    rv = CreateTable();
    NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  } else {
    
    int32_t dbSchemaVersion;
    rv = mDefaultDBState->dbConn->GetSchemaVersion(&dbSchemaVersion);
    NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

    
    mozStorageTransaction transaction(mDefaultDBState->dbConn, true);

    switch (dbSchemaVersion) {
    
    
    
    
    
    
    case 1:
      {
        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "ALTER TABLE moz_cookies ADD lastAccessed INTEGER"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);
      }
      

    case 2:
      {
        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "ALTER TABLE moz_cookies ADD baseDomain TEXT"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        
        
        
        nsCOMPtr<mozIStorageStatement> select;
        rv = mDefaultDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT id, host FROM moz_cookies"), getter_AddRefs(select));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        nsCOMPtr<mozIStorageStatement> update;
        rv = mDefaultDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
          "UPDATE moz_cookies SET baseDomain = :baseDomain WHERE id = :id"),
          getter_AddRefs(update));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        nsCString baseDomain, host;
        bool hasResult;
        while (1) {
          rv = select->ExecuteStep(&hasResult);
          NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

          if (!hasResult)
            break;

          int64_t id = select->AsInt64(0);
          select->GetUTF8String(1, host);

          rv = GetBaseDomainFromHost(host, baseDomain);
          NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

          mozStorageStatementScoper scoper(update);

          rv = update->BindUTF8StringByName(NS_LITERAL_CSTRING("baseDomain"),
                                            baseDomain);
          NS_ASSERT_SUCCESS(rv);
          rv = update->BindInt64ByName(NS_LITERAL_CSTRING("id"),
                                       id);
          NS_ASSERT_SUCCESS(rv);

          rv = update->ExecuteStep(&hasResult);
          NS_ENSURE_SUCCESS(rv, RESULT_RETRY);
        }

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "CREATE INDEX moz_basedomain ON moz_cookies (baseDomain)"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);
      }
      

    case 3:
      {
        
        
        
        
        
        
        

        
        
        
        nsCOMPtr<mozIStorageStatement> select;
        rv = mDefaultDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT id, name, host, path FROM moz_cookies "
            "ORDER BY name ASC, host ASC, path ASC, expiry ASC"),
          getter_AddRefs(select));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        nsCOMPtr<mozIStorageStatement> deleteExpired;
        rv = mDefaultDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
          "DELETE FROM moz_cookies WHERE id = :id"),
          getter_AddRefs(deleteExpired));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        
        bool hasResult;
        rv = select->ExecuteStep(&hasResult);
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        if (hasResult) {
          nsCString name1, host1, path1;
          int64_t id1 = select->AsInt64(0);
          select->GetUTF8String(1, name1);
          select->GetUTF8String(2, host1);
          select->GetUTF8String(3, path1);

          nsCString name2, host2, path2;
          while (1) {
            
            rv = select->ExecuteStep(&hasResult);
            NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

            if (!hasResult)
              break;

            int64_t id2 = select->AsInt64(0);
            select->GetUTF8String(1, name2);
            select->GetUTF8String(2, host2);
            select->GetUTF8String(3, path2);

            
            
            if (name1 == name2 && host1 == host2 && path1 == path2) {
              mozStorageStatementScoper scoper(deleteExpired);

              rv = deleteExpired->BindInt64ByName(NS_LITERAL_CSTRING("id"),
                id1);
              NS_ASSERT_SUCCESS(rv);

              rv = deleteExpired->ExecuteStep(&hasResult);
              NS_ENSURE_SUCCESS(rv, RESULT_RETRY);
            }

            
            name1 = name2;
            host1 = host2;
            path1 = path2;
            id1 = id2;
          }
        }

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "ALTER TABLE moz_cookies ADD creationTime INTEGER"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "UPDATE moz_cookies SET creationTime = "
            "(SELECT id WHERE id = moz_cookies.id)"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "CREATE UNIQUE INDEX moz_uniqueid "
          "ON moz_cookies (name, host, path)"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);
      }
      

    case 4:
      {
        
        
        
        
        
        
        
        
        
        

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "ALTER TABLE moz_cookies RENAME TO moz_cookies_old"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "DROP INDEX moz_basedomain"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        
        rv = CreateTable();
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "INSERT INTO moz_cookies "
          "(baseDomain, appId, inBrowserElement, name, value, host, path, expiry,"
          " lastAccessed, creationTime, isSecure, isHttpOnly) "
          "SELECT baseDomain, 0, 0, name, value, host, path, expiry,"
          " lastAccessed, creationTime, isSecure, isHttpOnly "
          "FROM moz_cookies_old"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "DROP TABLE moz_cookies_old"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        COOKIE_LOGSTRING(PR_LOG_DEBUG, 
          ("Upgraded database to schema version 5"));
      }

      
      rv = mDefaultDBState->dbConn->SetSchemaVersion(COOKIES_SCHEMA_VERSION);
      NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

    case COOKIES_SCHEMA_VERSION:
      break;

    case 0:
      {
        NS_WARNING("couldn't get schema version!");
          
        
        
        
        
        
        rv = mDefaultDBState->dbConn->SetSchemaVersion(COOKIES_SCHEMA_VERSION);
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);
      }
      

    
    
    
    
    
    
    default:
      {
        
        nsCOMPtr<mozIStorageStatement> stmt;
        rv = mDefaultDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
          "SELECT "
            "id, "
            "baseDomain, "
            "appId, "
            "inBrowserElement, "
            "name, "
            "value, "
            "host, "
            "path, "
            "expiry, "
            "lastAccessed, "
            "creationTime, "
            "isSecure, "
            "isHttpOnly "
          "FROM moz_cookies"), getter_AddRefs(stmt));
        if (NS_SUCCEEDED(rv))
          break;

        
        rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "DROP TABLE moz_cookies"));
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        rv = CreateTable();
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);
      }
      break;
    }
  }

  
  mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "PRAGMA synchronous = OFF"));

  
  
  mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    MOZ_STORAGE_UNIQUIFY_QUERY_STR "PRAGMA journal_mode = WAL"));
  mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "PRAGMA wal_autocheckpoint = 16"));

  
  rv = mDefaultDBState->dbConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_cookies ("
      "baseDomain, "
      "appId, "
      "inBrowserElement, "
      "name, "
      "value, "
      "host, "
      "path, "
      "expiry, "
      "lastAccessed, "
      "creationTime, "
      "isSecure, "
      "isHttpOnly"
    ") VALUES ("
      ":baseDomain, "
      ":appId, "
      ":inBrowserElement, "
      ":name, "
      ":value, "
      ":host, "
      ":path, "
      ":expiry, "
      ":lastAccessed, "
      ":creationTime, "
      ":isSecure, "
      ":isHttpOnly"
    ")"),
    getter_AddRefs(mDefaultDBState->stmtInsert));
  NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  rv = mDefaultDBState->dbConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_cookies "
    "WHERE name = :name AND host = :host AND path = :path"),
    getter_AddRefs(mDefaultDBState->stmtDelete));
  NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  rv = mDefaultDBState->dbConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_cookies SET lastAccessed = :lastAccessed "
    "WHERE name = :name AND host = :host AND path = :path"),
    getter_AddRefs(mDefaultDBState->stmtUpdate));
  NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  
  if (aRecreateDB)
    return RESULT_OK;

  
  if (tableExists)
    return Read();

  nsCOMPtr<nsIFile> oldCookieFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
    getter_AddRefs(oldCookieFile));
  if (NS_FAILED(rv)) return RESULT_OK;

  
  
  
  DBState* initialState = mDBState;
  mDBState = mDefaultDBState;
  oldCookieFile->AppendNative(NS_LITERAL_CSTRING(kOldCookieFileName));
  ImportCookies(oldCookieFile);
  oldCookieFile->Remove(false);
  mDBState = initialState;

  return RESULT_OK;
}


nsresult
nsCookieService::CreateTable()
{
  
  nsresult rv = mDefaultDBState->dbConn->SetSchemaVersion(
    COOKIES_SCHEMA_VERSION);
  if (NS_FAILED(rv)) return rv;

  
  
  
  rv = mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE moz_cookies ("
      "id INTEGER PRIMARY KEY, "
      "baseDomain TEXT, "
      "appId INTEGER DEFAULT 0, "
      "inBrowserElement INTEGER DEFAULT 0, "
      "name TEXT, "
      "value TEXT, "
      "host TEXT, "
      "path TEXT, "
      "expiry INTEGER, "
      "lastAccessed INTEGER, "
      "creationTime INTEGER, "
      "isSecure INTEGER, "
      "isHttpOnly INTEGER, "
      "CONSTRAINT moz_uniqueid UNIQUE (name, host, path, appId, inBrowserElement)"
    ")"));
  if (NS_FAILED(rv)) return rv;

  
  return mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_basedomain ON moz_cookies (baseDomain, "
                                                "appId, "
                                                "inBrowserElement)"));
}

void
nsCookieService::CloseDBStates()
{
  
  mPrivateDBState = NULL;
  mDBState = NULL;

  
  if (!mDefaultDBState)
    return;

  if (mDefaultDBState->dbConn) {
    
    
    if (mDefaultDBState->pendingRead) {
      CancelAsyncRead(true);
    }

    
    mDefaultDBState->dbConn->AsyncClose(mDefaultDBState->closeListener);
  }

  CloseDefaultDBConnection();

  mDefaultDBState = NULL;
}




void
nsCookieService::CloseDefaultDBConnection()
{
  
  mDefaultDBState->stmtInsert = NULL;
  mDefaultDBState->stmtDelete = NULL;
  mDefaultDBState->stmtUpdate = NULL;

  
  
  
  mDefaultDBState->dbConn = NULL;
  mDefaultDBState->syncConn = NULL;

  
  
  
  mDefaultDBState->readListener = NULL;
  mDefaultDBState->insertListener = NULL;
  mDefaultDBState->updateListener = NULL;
  mDefaultDBState->removeListener = NULL;
  mDefaultDBState->closeListener = NULL;
}

void
nsCookieService::HandleDBClosed(DBState* aDBState)
{
  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("HandleDBClosed(): DBState %x closed", aDBState));

  switch (aDBState->corruptFlag) {
  case DBState::OK: {
    
    mObserverService->NotifyObservers(nullptr, "cookie-db-closed", nullptr);
    break;
  }
  case DBState::CLOSING_FOR_REBUILD: {
    
    RebuildCorruptDB(aDBState);
    break;
  }
  case DBState::REBUILDING: {
    
    
    
    
    nsCOMPtr<nsIFile> backupFile;
    aDBState->cookieFile->Clone(getter_AddRefs(backupFile));
    nsresult rv = backupFile->MoveToNative(NULL,
      NS_LITERAL_CSTRING(COOKIES_FILE ".bak-rebuild"));

    COOKIE_LOGSTRING(PR_LOG_WARNING,
      ("HandleDBClosed(): DBState %x encountered error rebuilding db; move to "
       "'cookies.sqlite.bak-rebuild' gave rv 0x%x", aDBState, rv));
    mObserverService->NotifyObservers(nullptr, "cookie-db-closed", nullptr);
    break;
  }
  }
}

void
nsCookieService::HandleCorruptDB(DBState* aDBState)
{
  if (mDefaultDBState != aDBState) {
    
    
    COOKIE_LOGSTRING(PR_LOG_WARNING,
      ("HandleCorruptDB(): DBState %x is already closed, aborting", aDBState));
    return;
  }

  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("HandleCorruptDB(): DBState %x has corruptFlag %u", aDBState,
      aDBState->corruptFlag));

  
  
  switch (mDefaultDBState->corruptFlag) {
  case DBState::OK: {
    
    mDefaultDBState->corruptFlag = DBState::CLOSING_FOR_REBUILD;

    
    
    
    
    mDefaultDBState->readSet.Clear();
    if (mDefaultDBState->pendingRead) {
      CancelAsyncRead(true);
      mDefaultDBState->syncConn = nullptr;
    }

    mDefaultDBState->dbConn->AsyncClose(mDefaultDBState->closeListener);
    CloseDefaultDBConnection();
    break;
  }
  case DBState::CLOSING_FOR_REBUILD: {
    
    
    return;
  }
  case DBState::REBUILDING: {
    
    
    if (mDefaultDBState->dbConn) {
      mDefaultDBState->dbConn->AsyncClose(mDefaultDBState->closeListener);
    }
    CloseDefaultDBConnection();
    break;
  }
  }
}

static PLDHashOperator
RebuildDBCallback(nsCookieEntry *aEntry,
                  void          *aArg)
{
  mozIStorageBindingParamsArray* paramsArray =
    static_cast<mozIStorageBindingParamsArray*>(aArg);

  const nsCookieEntry::ArrayType &cookies = aEntry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    nsCookie* cookie = cookies[i];

    if (!cookie->IsSession()) {
      bindCookieParameters(paramsArray, aEntry, cookie);
    }
  }

  return PL_DHASH_NEXT;
}

void
nsCookieService::RebuildCorruptDB(DBState* aDBState)
{
  NS_ASSERTION(!aDBState->dbConn, "shouldn't have an open db connection");
  NS_ASSERTION(aDBState->corruptFlag == DBState::CLOSING_FOR_REBUILD,
    "should be in CLOSING_FOR_REBUILD state");

  aDBState->corruptFlag = DBState::REBUILDING;

  if (mDefaultDBState != aDBState) {
    
    
    
    
    COOKIE_LOGSTRING(PR_LOG_WARNING,
      ("RebuildCorruptDB(): DBState %x is stale, aborting", aDBState));
    mObserverService->NotifyObservers(nullptr, "cookie-db-closed", nullptr);
    return;
  }

  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("RebuildCorruptDB(): creating new database"));

  
  
  OpenDBResult result = TryInitDB(true);
  if (result != RESULT_OK) {
    
    
    COOKIE_LOGSTRING(PR_LOG_WARNING,
      ("RebuildCorruptDB(): TryInitDB() failed with result %u", result));
    CloseDefaultDBConnection();
    mDefaultDBState->corruptFlag = DBState::OK;
    mObserverService->NotifyObservers(nullptr, "cookie-db-closed", nullptr);
    return;
  }

  
  mObserverService->NotifyObservers(nullptr, "cookie-db-rebuilding", nullptr);

  
  mozIStorageAsyncStatement* stmt = aDBState->stmtInsert;
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  aDBState->hostTable.EnumerateEntries(RebuildDBCallback, paramsArray.get());

  
  uint32_t length;
  paramsArray->GetLength(&length);
  if (length == 0) {
    COOKIE_LOGSTRING(PR_LOG_DEBUG,
      ("RebuildCorruptDB(): nothing to write, rebuild complete"));
    mDefaultDBState->corruptFlag = DBState::OK;
    return;
  }

  
  DebugOnly<nsresult> rv = stmt->BindParameters(paramsArray);
  NS_ASSERT_SUCCESS(rv);
  nsCOMPtr<mozIStoragePendingStatement> handle;
  rv = stmt->ExecuteAsync(aDBState->insertListener, getter_AddRefs(handle));
  NS_ASSERT_SUCCESS(rv);    
}

nsCookieService::~nsCookieService()
{
  CloseDBStates();

  gCookieService = nullptr;
}

NS_IMETHODIMP
nsCookieService::Observe(nsISupports     *aSubject,
                         const char      *aTopic,
                         const PRUnichar *aData)
{
  
  if (!strcmp(aTopic, "profile-before-change")) {
    
    
    if (mDBState && mDBState->dbConn &&
        !nsCRT::strcmp(aData, NS_LITERAL_STRING("shutdown-cleanse").get())) {
      
      RemoveAll();
    }

    
    
    CloseDBStates();

  } else if (!strcmp(aTopic, "profile-do-change")) {
    NS_ASSERTION(!mDefaultDBState, "shouldn't have a default DBState");
    NS_ASSERTION(!mPrivateDBState, "shouldn't have a private DBState");

    
    
    
    
    InitDBStates();

  } else if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsCOMPtr<nsIPrefBranch> prefBranch = do_QueryInterface(aSubject);
    if (prefBranch)
      PrefChanged(prefBranch);

  } else if (!strcmp(aTopic, "last-pb-context-exited")) {
    
    mPrivateDBState = new DBState();
  }


  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::GetCookieString(nsIURI     *aHostURI,
                                 nsIChannel *aChannel,
                                 char       **aCookie)
{
  return GetCookieStringCommon(aHostURI, aChannel, false, aCookie);
}

NS_IMETHODIMP
nsCookieService::GetCookieStringFromHttp(nsIURI     *aHostURI,
                                         nsIURI     *aFirstURI,
                                         nsIChannel *aChannel,
                                         char       **aCookie)
{
  return GetCookieStringCommon(aHostURI, aChannel, true, aCookie);
}

nsresult
nsCookieService::GetCookieStringCommon(nsIURI *aHostURI,
                                       nsIChannel *aChannel,
                                       bool aHttpBound,
                                       char** aCookie)
{
  NS_ENSURE_ARG(aHostURI);
  NS_ENSURE_ARG(aCookie);

  
  bool isForeign = true;
  mThirdPartyUtil->IsThirdPartyChannel(aChannel, aHostURI, &isForeign);

  
  uint32_t appId = NECKO_NO_APP_ID;
  bool inBrowserElement = false;
  if (aChannel) {
    NS_GetAppInfo(aChannel, &appId, &inBrowserElement);
  }

  bool isPrivate = aChannel && NS_UsePrivateBrowsing(aChannel);

  nsAutoCString result;
  GetCookieStringInternal(aHostURI, isForeign, aHttpBound, appId,
                          inBrowserElement, isPrivate, result);
  *aCookie = result.IsEmpty() ? nullptr : ToNewCString(result);
  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::SetCookieString(nsIURI     *aHostURI,
                                 nsIPrompt  *aPrompt,
                                 const char *aCookieHeader,
                                 nsIChannel *aChannel)
{
  return SetCookieStringCommon(aHostURI, aCookieHeader, NULL, aChannel, false);
}

NS_IMETHODIMP
nsCookieService::SetCookieStringFromHttp(nsIURI     *aHostURI,
                                         nsIURI     *aFirstURI,
                                         nsIPrompt  *aPrompt,
                                         const char *aCookieHeader,
                                         const char *aServerTime,
                                         nsIChannel *aChannel) 
{
  return SetCookieStringCommon(aHostURI, aCookieHeader, aServerTime, aChannel,
                               true);
}

nsresult
nsCookieService::SetCookieStringCommon(nsIURI *aHostURI,
                                       const char *aCookieHeader,
                                       const char *aServerTime,
                                       nsIChannel *aChannel,
                                       bool aFromHttp) 
{
  NS_ENSURE_ARG(aHostURI);
  NS_ENSURE_ARG(aCookieHeader);

  
  bool isForeign = true;
  mThirdPartyUtil->IsThirdPartyChannel(aChannel, aHostURI, &isForeign);

  
  uint32_t appId = NECKO_NO_APP_ID;
  bool inBrowserElement = false;
  if (aChannel) {
    NS_GetAppInfo(aChannel, &appId, &inBrowserElement);
  }

  bool isPrivate = aChannel && NS_UsePrivateBrowsing(aChannel);

  nsDependentCString cookieString(aCookieHeader);
  nsDependentCString serverTime(aServerTime ? aServerTime : "");
  SetCookieStringInternal(aHostURI, isForeign, cookieString,
                          serverTime, aFromHttp, appId, inBrowserElement,
                          isPrivate, aChannel);
  return NS_OK;
}

void
nsCookieService::SetCookieStringInternal(nsIURI             *aHostURI,
                                         bool                aIsForeign,
                                         nsDependentCString &aCookieHeader,
                                         const nsCString    &aServerTime,
                                         bool                aFromHttp,
                                         uint32_t            aAppId,
                                         bool                aInBrowserElement,
                                         bool                aIsPrivate,
                                         nsIChannel         *aChannel)
{
  NS_ASSERTION(aHostURI, "null host!");

  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return;
  }

  AutoRestore<DBState*> savePrevDBState(mDBState);
  mDBState = aIsPrivate ? mPrivateDBState : mDefaultDBState;

  
  
  
  
  
  bool requireHostMatch;
  nsAutoCString baseDomain;
  nsresult rv = GetBaseDomain(aHostURI, baseDomain, requireHostMatch);
  if (NS_FAILED(rv)) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, 
                      "couldn't get base domain from URI");
    return;
  }

  nsCookieKey key(baseDomain, aAppId, aInBrowserElement);

  
  CookieStatus cookieStatus = CheckPrefs(aHostURI, aIsForeign, requireHostMatch,
                                         aCookieHeader.get());
  
  switch (cookieStatus) {
  case STATUS_REJECTED:
    NotifyRejected(aHostURI);
    return;
  case STATUS_REJECTED_WITH_ERROR:
    return;
  default:
    break;
  }

  
  
  
  
  
  PRTime tempServerTime;
  int64_t serverTime;
  PRStatus result = PR_ParseTimeString(aServerTime.get(), true,
                                       &tempServerTime);
  if (result == PR_SUCCESS) {
    serverTime = tempServerTime / int64_t(PR_USEC_PER_SEC);
  } else {
    serverTime = PR_Now() / PR_USEC_PER_SEC;
  }

  
  while (SetCookieInternal(aHostURI, key, requireHostMatch, cookieStatus,
                           aCookieHeader, serverTime, aFromHttp, aChannel)) {
    
    if (!aFromHttp)
      break;
  }
}


void
nsCookieService::NotifyRejected(nsIURI *aHostURI)
{
  if (mObserverService)
    mObserverService->NotifyObservers(aHostURI, "cookie-rejected", nullptr);
}









void
nsCookieService::NotifyChanged(nsISupports     *aSubject,
                               const PRUnichar *aData)
{
  const char* topic = mDBState == mPrivateDBState ?
      "private-cookie-changed" : "cookie-changed";
  if (mObserverService)
    mObserverService->NotifyObservers(aSubject, topic, aData);
}

already_AddRefed<nsIArray>
nsCookieService::CreatePurgeList(nsICookie2* aCookie)
{
  nsCOMPtr<nsIMutableArray> removedList =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  removedList->AppendElement(aCookie, false);
  return removedList.forget();
}






void
nsCookieService::PrefChanged(nsIPrefBranch *aPrefBranch)
{
  int32_t val;
  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefCookieBehavior, &val)))
    mCookieBehavior = (uint8_t) LIMIT(val, 0, 3, 0);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefMaxNumberOfCookies, &val)))
    mMaxNumberOfCookies = (uint16_t) LIMIT(val, 1, 0xFFFF, kMaxNumberOfCookies);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefMaxCookiesPerHost, &val)))
    mMaxCookiesPerHost = (uint16_t) LIMIT(val, 1, 0xFFFF, kMaxCookiesPerHost);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefCookiePurgeAge, &val))) {
    mCookiePurgeAge =
      int64_t(LIMIT(val, 0, INT32_MAX, INT32_MAX)) * PR_USEC_PER_SEC;
  }

  bool boolval;
  if (NS_SUCCEEDED(aPrefBranch->GetBoolPref(kPrefThirdPartySession, &boolval)))
    mThirdPartySession = boolval;
}






NS_IMETHODIMP
nsCookieService::RemoveAll()
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  RemoveAllFromMemory();

  
  if (mDBState->dbConn) {
    NS_ASSERTION(mDBState == mDefaultDBState, "not in default DB state");

    
    
    if (mDefaultDBState->pendingRead) {
      CancelAsyncRead(true);
    }

    nsCOMPtr<mozIStorageAsyncStatement> stmt;
    nsresult rv = mDefaultDBState->dbConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_cookies"), getter_AddRefs(stmt));
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<mozIStoragePendingStatement> handle;
      rv = stmt->ExecuteAsync(mDefaultDBState->removeListener,
        getter_AddRefs(handle));
      NS_ASSERT_SUCCESS(rv);
    } else {
      
      COOKIE_LOGSTRING(PR_LOG_DEBUG,
        ("RemoveAll(): corruption detected with rv 0x%x", rv));
      HandleCorruptDB(mDefaultDBState);
    }
  }

  NotifyChanged(nullptr, NS_LITERAL_STRING("cleared").get());
  return NS_OK;
}

static PLDHashOperator
COMArrayCallback(nsCookieEntry *aEntry,
                 void          *aArg)
{
  nsCOMArray<nsICookie> *data = static_cast<nsCOMArray<nsICookie> *>(aArg);

  const nsCookieEntry::ArrayType &cookies = aEntry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    data->AppendObject(cookies[i]);
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsCookieService::GetEnumerator(nsISimpleEnumerator **aEnumerator)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  EnsureReadComplete();

  nsCOMArray<nsICookie> cookieList(mDBState->cookieCount);
  mDBState->hostTable.EnumerateEntries(COMArrayCallback, &cookieList);

  return NS_NewArrayEnumerator(aEnumerator, cookieList);
}

NS_IMETHODIMP
nsCookieService::Add(const nsACString &aHost,
                     const nsACString &aPath,
                     const nsACString &aName,
                     const nsACString &aValue,
                     bool              aIsSecure,
                     bool              aIsHttpOnly,
                     bool              aIsSession,
                     int64_t           aExpiry)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsAutoCString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsAutoCString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  int64_t currentTimeInUsec = PR_Now();

  nsRefPtr<nsCookie> cookie =
    nsCookie::Create(aName, aValue, host, aPath,
                     aExpiry,
                     currentTimeInUsec,
                     nsCookie::GenerateUniqueCreationTime(currentTimeInUsec),
                     aIsSession,
                     aIsSecure,
                     aIsHttpOnly);
  if (!cookie) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  AddInternal(DEFAULT_APP_KEY(baseDomain), cookie, currentTimeInUsec, nullptr, nullptr, true);
  return NS_OK;
}


nsresult
nsCookieService::Remove(const nsACString& aHost, uint32_t aAppId,
                        bool aInBrowserElement, const nsACString& aName,
                        const nsACString& aPath, bool aBlocked)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsAutoCString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsListIter matchIter;
  nsRefPtr<nsCookie> cookie;
  if (FindCookie(nsCookieKey(baseDomain, aAppId, aInBrowserElement),
                 host,
                 PromiseFlatCString(aName),
                 PromiseFlatCString(aPath),
                 matchIter)) {
    cookie = matchIter.Cookie();
    RemoveCookieFromList(matchIter);
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

  if (cookie) {
    
    NotifyChanged(cookie, NS_LITERAL_STRING("deleted").get());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCookieService::Remove(const nsACString &aHost,
                        const nsACString &aName,
                        const nsACString &aPath,
                        bool             aBlocked)
{
  return Remove(aHost, NECKO_NO_APP_ID, false, aName, aPath, aBlocked);
}







OpenDBResult
nsCookieService::Read()
{
  
  
  nsCOMPtr<mozIStorageAsyncStatement> stmtRead;
  nsresult rv = mDefaultDBState->dbConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "SELECT "
      "name, "
      "value, "
      "host, "
      "path, "
      "expiry, "
      "lastAccessed, "
      "creationTime, "
      "isSecure, "
      "isHttpOnly, "
      "baseDomain, "
      "appId,  "
      "inBrowserElement "
    "FROM moz_cookies "
    "WHERE baseDomain NOTNULL"), getter_AddRefs(stmtRead));
  NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  
  
  
  
  nsCOMPtr<mozIStorageAsyncStatement> stmtDeleteNull;
  rv = mDefaultDBState->dbConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_cookies WHERE baseDomain ISNULL"),
    getter_AddRefs(stmtDeleteNull));
  NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  
  
  
  rv = mStorageService->OpenUnsharedDatabase(mDefaultDBState->cookieFile,
    getter_AddRefs(mDefaultDBState->syncConn));
  NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  
  
  
  mDefaultDBState->readSet.Init();
  mDefaultDBState->hostArray.SetCapacity(kMaxNumberOfCookies);

  mDefaultDBState->readListener = new ReadCookieDBListener(mDefaultDBState);
  rv = stmtRead->ExecuteAsync(mDefaultDBState->readListener,
    getter_AddRefs(mDefaultDBState->pendingRead));
  NS_ASSERT_SUCCESS(rv);

  nsCOMPtr<mozIStoragePendingStatement> handle;
  rv = stmtDeleteNull->ExecuteAsync(mDefaultDBState->removeListener,
    getter_AddRefs(handle));
  NS_ASSERT_SUCCESS(rv);

  return RESULT_OK;
}



template<class T> nsCookie*
nsCookieService::GetCookieFromRow(T &aRow)
{
  
  nsCString name, value, host, path;
  DebugOnly<nsresult> rv = aRow->GetUTF8String(0, name);
  NS_ASSERT_SUCCESS(rv);
  rv = aRow->GetUTF8String(1, value);
  NS_ASSERT_SUCCESS(rv);
  rv = aRow->GetUTF8String(2, host);
  NS_ASSERT_SUCCESS(rv);
  rv = aRow->GetUTF8String(3, path);
  NS_ASSERT_SUCCESS(rv);

  int64_t expiry = aRow->AsInt64(4);
  int64_t lastAccessed = aRow->AsInt64(5);
  int64_t creationTime = aRow->AsInt64(6);
  bool isSecure = 0 != aRow->AsInt32(7);
  bool isHttpOnly = 0 != aRow->AsInt32(8);

  
  return nsCookie::Create(name, value, host, path,
                          expiry,
                          lastAccessed,
                          creationTime,
                          false,
                          isSecure,
                          isHttpOnly);
}

void
nsCookieService::AsyncReadComplete()
{
  
  
  
  NS_ASSERTION(mDefaultDBState, "no default DBState");
  NS_ASSERTION(mDefaultDBState->pendingRead, "no pending read");
  NS_ASSERTION(mDefaultDBState->readListener, "no read listener");

  
  
  
  for (uint32_t i = 0; i < mDefaultDBState->hostArray.Length(); ++i) {
    const CookieDomainTuple &tuple = mDefaultDBState->hostArray[i];

    
    
    
    if (mDefaultDBState->readSet.GetEntry(tuple.key))
      continue;

    AddCookieToList(tuple.key, tuple.cookie, mDefaultDBState, NULL, false);
  }

  mDefaultDBState->stmtReadDomain = nullptr;
  mDefaultDBState->pendingRead = nullptr;
  mDefaultDBState->readListener = nullptr;
  mDefaultDBState->syncConn = nullptr;
  mDefaultDBState->hostArray.Clear();
  mDefaultDBState->readSet.Clear();

  COOKIE_LOGSTRING(PR_LOG_DEBUG, ("Read(): %ld cookies read",
                                  mDefaultDBState->cookieCount));

  mObserverService->NotifyObservers(nullptr, "cookie-db-read", nullptr);
}

void
nsCookieService::CancelAsyncRead(bool aPurgeReadSet)
{
  
  
  
  NS_ASSERTION(mDefaultDBState, "no default DBState");
  NS_ASSERTION(mDefaultDBState->pendingRead, "no pending read");
  NS_ASSERTION(mDefaultDBState->readListener, "no read listener");

  
  
  mDefaultDBState->readListener->Cancel();
  DebugOnly<nsresult> rv = mDefaultDBState->pendingRead->Cancel();
  NS_ASSERT_SUCCESS(rv);

  mDefaultDBState->stmtReadDomain = nullptr;
  mDefaultDBState->pendingRead = nullptr;
  mDefaultDBState->readListener = nullptr;
  mDefaultDBState->hostArray.Clear();

  
  
  if (aPurgeReadSet)
    mDefaultDBState->readSet.Clear();
}

void
nsCookieService::EnsureReadDomain(const nsCookieKey &aKey)
{
  NS_ASSERTION(!mDBState->dbConn || mDBState == mDefaultDBState,
    "not in default db state");

  
  if (MOZ_LIKELY(!mDBState->dbConn || !mDefaultDBState->pendingRead))
    return;

  
  if (MOZ_LIKELY(mDefaultDBState->readSet.GetEntry(aKey)))
    return;

  
  nsresult rv;
  if (!mDefaultDBState->stmtReadDomain) {
    
    rv = mDefaultDBState->syncConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT "
        "name, "
        "value, "
        "host, "
        "path, "
        "expiry, "
        "lastAccessed, "
        "creationTime, "
        "isSecure, "
        "isHttpOnly "
      "FROM moz_cookies "
      "WHERE baseDomain = :baseDomain "
      "  AND appId = :appId "
      "  AND inBrowserElement = :inBrowserElement"),
      getter_AddRefs(mDefaultDBState->stmtReadDomain));

    if (NS_FAILED(rv)) {
      
      COOKIE_LOGSTRING(PR_LOG_DEBUG,
        ("EnsureReadDomain(): corruption detected when creating statement "
         "with rv 0x%x", rv));
      HandleCorruptDB(mDefaultDBState);
      return;
    }
  }

  NS_ASSERTION(mDefaultDBState->syncConn, "should have a sync db connection");

  mozStorageStatementScoper scoper(mDefaultDBState->stmtReadDomain);

  rv = mDefaultDBState->stmtReadDomain->BindUTF8StringByName(
    NS_LITERAL_CSTRING("baseDomain"), aKey.mBaseDomain);
  NS_ASSERT_SUCCESS(rv);
  rv = mDefaultDBState->stmtReadDomain->BindInt32ByName(
    NS_LITERAL_CSTRING("appId"), aKey.mAppId);
  NS_ASSERT_SUCCESS(rv);
  rv = mDefaultDBState->stmtReadDomain->BindInt32ByName(
    NS_LITERAL_CSTRING("inBrowserElement"), aKey.mInBrowserElement ? 1 : 0);
  NS_ASSERT_SUCCESS(rv);


  bool hasResult;
  nsCString name, value, host, path;
  nsAutoTArray<nsRefPtr<nsCookie>, kMaxCookiesPerHost> array;
  while (1) {
    rv = mDefaultDBState->stmtReadDomain->ExecuteStep(&hasResult);
    if (NS_FAILED(rv)) {
      
      COOKIE_LOGSTRING(PR_LOG_DEBUG,
        ("EnsureReadDomain(): corruption detected when reading result "
         "with rv 0x%x", rv));
      HandleCorruptDB(mDefaultDBState);
      return;
    }

    if (!hasResult)
      break;

    array.AppendElement(GetCookieFromRow(mDefaultDBState->stmtReadDomain));
  }

  
  
  for (uint32_t i = 0; i < array.Length(); ++i) {
    AddCookieToList(aKey, array[i], mDefaultDBState, NULL, false);
  }

  
  mDefaultDBState->readSet.PutEntry(aKey);

  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("EnsureReadDomain(): %ld cookies read for base domain %s, "
     " appId=%u, inBrowser=%d", array.Length(), aKey.mBaseDomain.get(),
     (unsigned)aKey.mAppId, (int)aKey.mInBrowserElement));
}

void
nsCookieService::EnsureReadComplete()
{
  NS_ASSERTION(!mDBState->dbConn || mDBState == mDefaultDBState,
    "not in default db state");

  
  if (MOZ_LIKELY(!mDBState->dbConn || !mDefaultDBState->pendingRead))
    return;

  
  CancelAsyncRead(false);

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mDefaultDBState->syncConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT "
      "name, "
      "value, "
      "host, "
      "path, "
      "expiry, "
      "lastAccessed, "
      "creationTime, "
      "isSecure, "
      "isHttpOnly, "
      "baseDomain, "
      "appId,  "
      "inBrowserElement "
    "FROM moz_cookies "
    "WHERE baseDomain NOTNULL"), getter_AddRefs(stmt));

  if (NS_FAILED(rv)) {
    
    COOKIE_LOGSTRING(PR_LOG_DEBUG,
      ("EnsureReadComplete(): corruption detected when creating statement "
       "with rv 0x%x", rv));
    HandleCorruptDB(mDefaultDBState);
    return;
  }

  nsCString baseDomain, name, value, host, path;
  uint32_t appId;
  bool inBrowserElement, hasResult;
  nsAutoTArray<CookieDomainTuple, kMaxNumberOfCookies> array;
  while (1) {
    rv = stmt->ExecuteStep(&hasResult);
    if (NS_FAILED(rv)) {
      
      COOKIE_LOGSTRING(PR_LOG_DEBUG,
        ("EnsureReadComplete(): corruption detected when reading result "
         "with rv 0x%x", rv));
      HandleCorruptDB(mDefaultDBState);
      return;
    }

    if (!hasResult)
      break;

    
    stmt->GetUTF8String(9, baseDomain);
    appId = static_cast<uint32_t>(stmt->AsInt32(10));
    inBrowserElement = static_cast<bool>(stmt->AsInt32(11));
    nsCookieKey key(baseDomain, appId, inBrowserElement);
    if (mDefaultDBState->readSet.GetEntry(key))
      continue;

    CookieDomainTuple* tuple = array.AppendElement();
    tuple->key = key;
    tuple->cookie = GetCookieFromRow(stmt);
  }

  
  
  for (uint32_t i = 0; i < array.Length(); ++i) {
    CookieDomainTuple& tuple = array[i];
    AddCookieToList(tuple.key, tuple.cookie, mDefaultDBState, NULL,
      false);
  }

  mDefaultDBState->syncConn = nullptr;
  mDefaultDBState->readSet.Clear();

  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("EnsureReadComplete(): %ld cookies read", array.Length()));
}

NS_IMETHODIMP
nsCookieService::ImportCookies(nsIFile *aCookieFile)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  if (mDBState != mDefaultDBState) {
    NS_WARNING("Trying to import cookies in a private browsing session!");
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv;
  nsCOMPtr<nsIInputStream> fileInputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(fileInputStream), aCookieFile);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsILineInputStream> lineInputStream = do_QueryInterface(fileInputStream, &rv);
  if (NS_FAILED(rv)) return rv;

  
  EnsureReadComplete();

  static const char kTrue[] = "TRUE";

  nsAutoCString buffer, baseDomain;
  bool isMore = true;
  int32_t hostIndex, isDomainIndex, pathIndex, secureIndex, expiresIndex, nameIndex, cookieIndex;
  nsASingleFragmentCString::char_iterator iter;
  int32_t numInts;
  int64_t expires;
  bool isDomain, isHttpOnly = false;
  uint32_t originalCookieCount = mDefaultDBState->cookieCount;

  int64_t currentTimeInUsec = PR_Now();
  int64_t currentTime = currentTimeInUsec / PR_USEC_PER_SEC;
  
  
  int64_t lastAccessedCounter = currentTimeInUsec;

  












  









  
  
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  if (originalCookieCount == 0 && mDefaultDBState->dbConn) {
    mDefaultDBState->stmtInsert->NewBindingParamsArray(getter_AddRefs(paramsArray));
  }

  while (isMore && NS_SUCCEEDED(lineInputStream->ReadLine(buffer, &isMore))) {
    if (StringBeginsWith(buffer, NS_LITERAL_CSTRING(kHttpOnlyPrefix))) {
      isHttpOnly = true;
      hostIndex = sizeof(kHttpOnlyPrefix) - 1;
    } else if (buffer.IsEmpty() || buffer.First() == '#') {
      continue;
    } else {
      isHttpOnly = false;
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

    
    nsCookieKey key = DEFAULT_APP_KEY(baseDomain);

    
    
    nsRefPtr<nsCookie> newCookie =
      nsCookie::Create(Substring(buffer, nameIndex, cookieIndex - nameIndex - 1),
                       Substring(buffer, cookieIndex, buffer.Length() - cookieIndex),
                       host,
                       Substring(buffer, pathIndex, secureIndex - pathIndex - 1),
                       expires,
                       lastAccessedCounter,
                       nsCookie::GenerateUniqueCreationTime(currentTimeInUsec),
                       false,
                       Substring(buffer, secureIndex, expiresIndex - secureIndex - 1).EqualsLiteral(kTrue),
                       isHttpOnly);
    if (!newCookie) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    
    
    lastAccessedCounter--;

    if (originalCookieCount == 0) {
      AddCookieToList(key, newCookie, mDefaultDBState, paramsArray);
    }
    else {
      AddInternal(key, newCookie, currentTimeInUsec,
                  NULL, NULL, true);
    }
  }

  
  if (paramsArray) {
    uint32_t length;
    paramsArray->GetLength(&length);
    if (length) {
      rv = mDefaultDBState->stmtInsert->BindParameters(paramsArray);
      NS_ASSERT_SUCCESS(rv);
      nsCOMPtr<mozIStoragePendingStatement> handle;
      rv = mDefaultDBState->stmtInsert->ExecuteAsync(
        mDefaultDBState->insertListener, getter_AddRefs(handle));
      NS_ASSERT_SUCCESS(rv);
    }
  }


  COOKIE_LOGSTRING(PR_LOG_DEBUG, ("ImportCookies(): %ld cookies imported",
    mDefaultDBState->cookieCount));

  return NS_OK;
}







static inline bool ispathdelimiter(char c) { return c == '/' || c == '?' || c == '#' || c == ';'; }


class CompareCookiesForSending
{
public:
  bool Equals(const nsCookie* aCookie1, const nsCookie* aCookie2) const
  {
    return aCookie1->CreationTime() == aCookie2->CreationTime() &&
           aCookie2->Path().Length() == aCookie1->Path().Length();
  }

  bool LessThan(const nsCookie* aCookie1, const nsCookie* aCookie2) const
  {
    
    int32_t result = aCookie2->Path().Length() - aCookie1->Path().Length();
    if (result != 0)
      return result < 0;

    
    
    
    
    return aCookie1->CreationTime() < aCookie2->CreationTime();
  }
};

void
nsCookieService::GetCookieStringInternal(nsIURI *aHostURI,
                                         bool aIsForeign,
                                         bool aHttpBound,
                                         uint32_t aAppId,
                                         bool aInBrowserElement,
                                         bool aIsPrivate,
                                         nsCString &aCookieString)
{
  NS_ASSERTION(aHostURI, "null host!");

  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return;
  }

  AutoRestore<DBState*> savePrevDBState(mDBState);
  mDBState = aIsPrivate ? mPrivateDBState : mDefaultDBState;

  
  
  
  
  
  bool requireHostMatch;
  nsAutoCString baseDomain, hostFromURI, pathFromURI;
  nsresult rv = GetBaseDomain(aHostURI, baseDomain, requireHostMatch);
  if (NS_SUCCEEDED(rv))
    rv = aHostURI->GetAsciiHost(hostFromURI);
  if (NS_SUCCEEDED(rv))
    rv = aHostURI->GetPath(pathFromURI);
  if (NS_FAILED(rv)) {
    COOKIE_LOGFAILURE(GET_COOKIE, aHostURI, nullptr, "invalid host/path from URI");
    return;
  }

  
  CookieStatus cookieStatus = CheckPrefs(aHostURI, aIsForeign, requireHostMatch,
                                         nullptr);
  
  switch (cookieStatus) {
  case STATUS_REJECTED:
  case STATUS_REJECTED_WITH_ERROR:
    return;
  default:
    break;
  }

  
  
  
  bool isSecure;
  if (NS_FAILED(aHostURI->SchemeIs("https", &isSecure))) {
    isSecure = false;
  }

  nsCookie *cookie;
  nsAutoTArray<nsCookie*, 8> foundCookieList;
  int64_t currentTimeInUsec = PR_Now();
  int64_t currentTime = currentTimeInUsec / PR_USEC_PER_SEC;
  bool stale = false;

  nsCookieKey key(baseDomain, aAppId, aInBrowserElement);
  EnsureReadDomain(key);

  
  nsCookieEntry *entry = mDBState->hostTable.GetEntry(key);
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

    
    uint32_t cookiePathLen = cookie->Path().Length();
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
      stale = true;
  }

  int32_t count = foundCookieList.Length();
  if (count == 0)
    return;

  
  
  if (stale) {
    
    
    nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
    mozIStorageAsyncStatement* stmt = mDBState->stmtUpdate;
    if (mDBState->dbConn) {
      stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
    }

    for (int32_t i = 0; i < count; ++i) {
      cookie = foundCookieList.ElementAt(i);

      if (currentTimeInUsec - cookie->LastAccessed() > kCookieStaleThreshold)
        UpdateCookieInList(cookie, currentTimeInUsec, paramsArray);
    }
    
    if (paramsArray) {
      uint32_t length;
      paramsArray->GetLength(&length);
      if (length) {
        DebugOnly<nsresult> rv = stmt->BindParameters(paramsArray);
        NS_ASSERT_SUCCESS(rv);
        nsCOMPtr<mozIStoragePendingStatement> handle;
        rv = stmt->ExecuteAsync(mDBState->updateListener,
          getter_AddRefs(handle));
        NS_ASSERT_SUCCESS(rv);
      }
    }
  }

  
  
  
  foundCookieList.Sort(CompareCookiesForSending());

  for (int32_t i = 0; i < count; ++i) {
    cookie = foundCookieList.ElementAt(i);

    
    if (!cookie->Name().IsEmpty() || !cookie->Value().IsEmpty()) {
      
      
      if (!aCookieString.IsEmpty()) {
        aCookieString.AppendLiteral("; ");
      }

      if (!cookie->Name().IsEmpty()) {
        
        aCookieString += cookie->Name() + NS_LITERAL_CSTRING("=") + cookie->Value();
      } else {
        
        aCookieString += cookie->Value();
      }
    }
  }

  if (!aCookieString.IsEmpty())
    COOKIE_LOGSUCCESS(GET_COOKIE, aHostURI, aCookieString, nullptr, false);
}



bool
nsCookieService::SetCookieInternal(nsIURI                        *aHostURI,
                                   const nsCookieKey             &aKey,
                                   bool                           aRequireHostMatch,
                                   CookieStatus                   aStatus,
                                   nsDependentCString            &aCookieHeader,
                                   int64_t                        aServerTime,
                                   bool                           aFromHttp,
                                   nsIChannel                    *aChannel)
{
  NS_ASSERTION(aHostURI, "null host!");

  
  
  nsCookieAttributes cookieAttributes;

  
  cookieAttributes.expiryTime = INT64_MAX;

  
  
  nsDependentCString savedCookieHeader(aCookieHeader);

  
  
  bool newCookie = ParseAttributes(aCookieHeader, cookieAttributes);

  int64_t currentTimeInUsec = PR_Now();

  
  cookieAttributes.isSession = GetExpiry(cookieAttributes, aServerTime,
                                         currentTimeInUsec / PR_USEC_PER_SEC);
  if (aStatus == STATUS_ACCEPT_SESSION) {
    
    
    cookieAttributes.isSession = true;
  }

  
  if ((cookieAttributes.name.Length() + cookieAttributes.value.Length()) > kMaxBytesPerCookie) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, savedCookieHeader, "cookie too big (> 4kb)");
    return newCookie;
  }

  if (cookieAttributes.name.FindChar('\t') != kNotFound) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, savedCookieHeader, "invalid name character");
    return newCookie;
  }

  
  if (!CheckDomain(cookieAttributes, aHostURI, aKey.mBaseDomain, aRequireHostMatch)) {
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
                     nsCookie::GenerateUniqueCreationTime(currentTimeInUsec),
                     cookieAttributes.isSession,
                     cookieAttributes.isSecure,
                     cookieAttributes.isHttpOnly);
  if (!cookie)
    return newCookie;

  
  
  if (mPermissionService) {
    bool permission;
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

  
  
  AddInternal(aKey, cookie, PR_Now(), aHostURI, savedCookieHeader.get(),
              aFromHttp);
  return newCookie;
}






void
nsCookieService::AddInternal(const nsCookieKey             &aKey,
                             nsCookie                      *aCookie,
                             int64_t                        aCurrentTimeInUsec,
                             nsIURI                        *aHostURI,
                             const char                    *aCookieHeader,
                             bool                           aFromHttp)
{
  int64_t currentTime = aCurrentTimeInUsec / PR_USEC_PER_SEC;

  
  if (!aFromHttp && aCookie->IsHttpOnly()) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader,
      "cookie is httponly; coming from script");
    return;
  }

  nsListIter matchIter;
  bool foundCookie = FindCookie(aKey, aCookie->Host(),
    aCookie->Name(), aCookie->Path(), matchIter);

  nsRefPtr<nsCookie> oldCookie;
  nsCOMPtr<nsIArray> purgedList;
  if (foundCookie) {
    oldCookie = matchIter.Cookie();

    
    
    
    
    if (oldCookie->Expiry() <= currentTime) {
      if (aCookie->Expiry() <= currentTime) {
        
        COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader,
          "cookie has already expired");
        return;
      }

      
      
      RemoveCookieFromList(matchIter);
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader,
        "stale cookie was purged");
      purgedList = CreatePurgeList(oldCookie);

      
      
      
      foundCookie = false;

    } else {
      
      if (!aFromHttp && oldCookie->IsHttpOnly()) {
        COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader,
          "previously stored cookie is httponly; coming from script");
        return;
      }

      
      RemoveCookieFromList(matchIter);

      
      
      if (aCookie->Expiry() <= currentTime) {
        COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader,
          "previously stored cookie was deleted");
        NotifyChanged(oldCookie, NS_LITERAL_STRING("deleted").get());
        return;
      }

      
      aCookie->SetCreationTime(oldCookie->CreationTime());
    }

  } else {
    
    if (aCookie->Expiry() <= currentTime) {
      COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader,
        "cookie has already expired");
      return;
    }

    
    nsCookieEntry *entry = mDBState->hostTable.GetEntry(aKey);
    if (entry && entry->GetCookies().Length() >= mMaxCookiesPerHost) {
      nsListIter iter;
      FindStaleCookie(entry, currentTime, iter);
      oldCookie = iter.Cookie();

      
      RemoveCookieFromList(iter);
      COOKIE_LOGEVICTED(oldCookie, "Too many cookies for this domain");
      purgedList = CreatePurgeList(oldCookie);

    } else if (mDBState->cookieCount >= ADD_TEN_PERCENT(mMaxNumberOfCookies)) {
      int64_t maxAge = aCurrentTimeInUsec - mDBState->cookieOldestTime;
      int64_t purgeAge = ADD_TEN_PERCENT(mCookiePurgeAge);
      if (maxAge >= purgeAge) {
        
        
        
        
        
        
        purgedList = PurgeCookies(aCurrentTimeInUsec);
      }
    }
  }

  
  
  AddCookieToList(aKey, aCookie, mDBState, NULL);
  COOKIE_LOGSUCCESS(SET_COOKIE, aHostURI, aCookieHeader, aCookie, foundCookie);

  
  
  if (purgedList) {
    NotifyChanged(purgedList, NS_LITERAL_STRING("batch-deleted").get());
  }

  NotifyChanged(aCookie, foundCookie ? NS_LITERAL_STRING("changed").get()
                                     : NS_LITERAL_STRING("added").get());
}












































































static inline bool iswhitespace     (char c) { return c == ' '  || c == '\t'; }
static inline bool isterminator     (char c) { return c == '\n' || c == '\r'; }
static inline bool isvalueseparator (char c) { return isterminator(c) || c == ';'; }
static inline bool istokenseparator (char c) { return isvalueseparator(c) || c == '='; }



bool
nsCookieService::GetTokenValue(nsASingleFragmentCString::const_char_iterator &aIter,
                               nsASingleFragmentCString::const_char_iterator &aEndIter,
                               nsDependentCSubstring                         &aTokenString,
                               nsDependentCSubstring                         &aTokenValue,
                               bool                                          &aEqualsFound)
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
    while (--lastSpace != start && iswhitespace(*lastSpace))
      continue;
    ++lastSpace;
  }
  aTokenString.Rebind(start, lastSpace);

  aEqualsFound = (*aIter == '=');
  if (aEqualsFound) {
    
    while (++aIter != aEndIter && iswhitespace(*aIter))
      continue;

    start = aIter;

    
    
    while (aIter != aEndIter && !isvalueseparator(*aIter))
      ++aIter;

    
    if (aIter != start) {
      lastSpace = aIter;
      while (--lastSpace != start && iswhitespace(*lastSpace))
        continue;
      aTokenValue.Rebind(start, ++lastSpace);
    }
  }

  
  if (aIter != aEndIter) {
    
    if (isterminator(*aIter)) {
      ++aIter;
      return true;
    }
    
    ++aIter;
  }
  return false;
}



bool
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

  aCookieAttributes.isSecure = false;
  aCookieAttributes.isHttpOnly = false;
  
  nsDependentCSubstring tokenString(cookieStart, cookieStart);
  nsDependentCSubstring tokenValue (cookieStart, cookieStart);
  bool newCookie, equalsFound;

  
  
  
  
  
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
      aCookieAttributes.isSecure = true;
      
    
    
    else if (tokenString.LowerCaseEqualsLiteral(kHttpOnly))
      aCookieAttributes.isHttpOnly = true;
  }

  
  aCookieHeader.Rebind(cookieStart, cookieEnd);
  return newCookie;
}












nsresult
nsCookieService::GetBaseDomain(nsIURI    *aHostURI,
                               nsCString &aBaseDomain,
                               bool      &aRequireHostMatch)
{
  
  
  nsresult rv = mTLDService->GetBaseDomain(aHostURI, 0, aBaseDomain);
  aRequireHostMatch = rv == NS_ERROR_HOST_IS_IP_ADDRESS ||
                      rv == NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS;
  if (aRequireHostMatch) {
    
    
    
    rv = aHostURI->GetAsciiHost(aBaseDomain);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aBaseDomain.Length() == 1 && aBaseDomain.Last() == '.')
    return NS_ERROR_INVALID_ARG;

  
  if (aBaseDomain.IsEmpty()) {
    bool isFileURI = false;
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
  
  if (aHost.Length() == 1 && aHost.Last() == '.')
    return NS_ERROR_INVALID_ARG;

  
  bool domain = !aHost.IsEmpty() && aHost.First() == '.';

  
  
  nsresult rv = mTLDService->GetBaseDomainFromHost(Substring(aHost, domain), 0, aBaseDomain);
  if (rv == NS_ERROR_HOST_IS_IP_ADDRESS ||
      rv == NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS) {
    
    
    
    
    if (domain)
      return NS_ERROR_INVALID_ARG;

    aBaseDomain = aHost;
    return NS_OK;
  }
  return rv;
}




nsresult
nsCookieService::NormalizeHost(nsCString &aHost)
{
  if (!IsASCII(aHost)) {
    nsAutoCString host;
    nsresult rv = mIDNService->ConvertUTF8toACE(aHost, host);
    if (NS_FAILED(rv))
      return rv;

    aHost = host;
  }

  ToLowerCase(aHost);
  return NS_OK;
}



static inline bool IsSubdomainOf(const nsCString &a, const nsCString &b)
{
  if (a == b)
    return true;
  if (a.Length() > b.Length())
    return a[a.Length() - b.Length() - 1] == '.' && StringEndsWith(a, b);
  return false;
}

CookieStatus
nsCookieService::CheckPrefs(nsIURI          *aHostURI,
                            bool             aIsForeign,
                            bool             aRequireHostMatch,
                            const char      *aCookieHeader)
{
  nsresult rv;

  
  bool ftp;
  if (NS_SUCCEEDED(aHostURI->SchemeIs("ftp", &ftp)) && ftp) {
    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "ftp sites cannot read cookies");
    return STATUS_REJECTED_WITH_ERROR;
  }

  
  
  if (mPermissionService) {
    nsCookieAccess access;
    
    
    rv = mPermissionService->CanAccess(aHostURI, nullptr, &access);

    
    if (NS_SUCCEEDED(rv)) {
      switch (access) {
      case nsICookiePermission::ACCESS_DENY:
        COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI,
                          aCookieHeader, "cookies are blocked for this site");
        return STATUS_REJECTED;

      case nsICookiePermission::ACCESS_ALLOW:
        return STATUS_ACCEPTED;

      case nsICookiePermission::ACCESS_ALLOW_FIRST_PARTY_ONLY:
        if (aIsForeign) {
          COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI,
                            aCookieHeader, "third party cookies are blocked "
                            "for this site");
          return STATUS_REJECTED;

        }
        return STATUS_ACCEPTED;

      case nsICookiePermission::ACCESS_LIMIT_THIRD_PARTY:
        if (!aIsForeign)
          return STATUS_ACCEPTED;
        uint32_t priorCookieCount = 0;
        nsAutoCString hostFromURI;
        aHostURI->GetHost(hostFromURI);
        CountCookiesFromHost(hostFromURI, &priorCookieCount);
        if (priorCookieCount == 0) {
          COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI,
                            aCookieHeader, "third party cookies are blocked "
                            "for this site");
          return STATUS_REJECTED;
        }
        return STATUS_ACCEPTED;
      }
    }
  }

  
  if (mCookieBehavior == BEHAVIOR_REJECT) {
    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "cookies are disabled");
    return STATUS_REJECTED;
  }

  
  if (aIsForeign) {
    if (mCookieBehavior == BEHAVIOR_ACCEPT && mThirdPartySession)
      return STATUS_ACCEPT_SESSION;

    if (mCookieBehavior == BEHAVIOR_REJECTFOREIGN) {
      COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "context is third party");
      return STATUS_REJECTED;
    }

    if (mCookieBehavior == BEHAVIOR_LIMITFOREIGN) {
      uint32_t priorCookieCount = 0;
      nsAutoCString hostFromURI;
      aHostURI->GetHost(hostFromURI);
      CountCookiesFromHost(hostFromURI, &priorCookieCount);
      if (priorCookieCount == 0) {
        COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "context is third party");
        return STATUS_REJECTED;
      }
      if (mThirdPartySession)
        return STATUS_ACCEPT_SESSION;
    }
  }

  
  return STATUS_ACCEPTED;
}


bool
nsCookieService::CheckDomain(nsCookieAttributes &aCookieAttributes,
                             nsIURI             *aHostURI,
                             const nsCString    &aBaseDomain,
                             bool                aRequireHostMatch)
{
  
  nsAutoCString hostFromURI;
  aHostURI->GetAsciiHost(hostFromURI);

  
  if (!aCookieAttributes.host.IsEmpty()) {
    
    if (aCookieAttributes.host.Length() > 1 &&
        aCookieAttributes.host.First() == '.') {
      aCookieAttributes.host.Cut(0, 1);
    }

    
    ToLowerCase(aCookieAttributes.host);

    
    
    
    
    
    if (aRequireHostMatch)
      return hostFromURI.Equals(aCookieAttributes.host);

    
    
    if (IsSubdomainOf(aCookieAttributes.host, aBaseDomain) &&
        IsSubdomainOf(hostFromURI, aCookieAttributes.host)) {
      
      aCookieAttributes.host.Insert(NS_LITERAL_CSTRING("."), 0);
      return true;
    }

    






    return false;
  }

  
  aCookieAttributes.host = hostFromURI;
  return true;
}

bool
nsCookieService::CheckPath(nsCookieAttributes &aCookieAttributes,
                           nsIURI             *aHostURI)
{
  
  if (aCookieAttributes.path.IsEmpty() || aCookieAttributes.path.First() != '/') {
    
    
    
    
    nsCOMPtr<nsIURL> hostURL = do_QueryInterface(aHostURI);
    if (hostURL) {
      hostURL->GetDirectory(aCookieAttributes.path);
    } else {
      aHostURI->GetPath(aCookieAttributes.path);
      int32_t slash = aCookieAttributes.path.RFindChar('/');
      if (slash != kNotFound) {
        aCookieAttributes.path.Truncate(slash + 1);
      }
    }

#if 0
  } else {
    





    
    nsAutoCString pathFromURI;
    if (NS_FAILED(aHostURI->GetPath(pathFromURI)) ||
        !StringBeginsWith(pathFromURI, aCookieAttributes.path)) {
      return false;
    }
#endif
  }

  if (aCookieAttributes.path.Length() > kMaxBytesPerPath ||
      aCookieAttributes.path.FindChar('\t') != kNotFound )
    return false;

  return true;
}

bool
nsCookieService::GetExpiry(nsCookieAttributes &aCookieAttributes,
                           int64_t             aServerTime,
                           int64_t             aCurrentTime)
{
  






  int64_t delta;

  
  if (!aCookieAttributes.maxage.IsEmpty()) {
    
    int64_t maxage;
    int32_t numInts = PR_sscanf(aCookieAttributes.maxage.get(), "%lld", &maxage);

    
    if (numInts != 1) {
      return true;
    }

    delta = maxage;

  
  } else if (!aCookieAttributes.expires.IsEmpty()) {
    PRTime expires;

    
    if (PR_ParseTimeString(aCookieAttributes.expires.get(), true, &expires) != PR_SUCCESS) {
      return true;
    }

    delta = expires / int64_t(PR_USEC_PER_SEC) - aServerTime;

  
  } else {
    return true;
  }

  
  
  aCookieAttributes.expiryTime = aCurrentTime + delta;

  return false;
}






void
nsCookieService::RemoveAllFromMemory()
{
  
  
  mDBState->hostTable.Clear();
  mDBState->cookieCount = 0;
  mDBState->cookieOldestTime = INT64_MAX;
}



struct nsPurgeData
{
  typedef nsTArray<nsListIter> ArrayType;

  nsPurgeData(int64_t aCurrentTime,
              int64_t aPurgeTime,
              ArrayType &aPurgeList,
              nsIMutableArray *aRemovedList,
              mozIStorageBindingParamsArray *aParamsArray)
   : currentTime(aCurrentTime)
   , purgeTime(aPurgeTime)
   , oldestTime(INT64_MAX)
   , purgeList(aPurgeList)
   , removedList(aRemovedList)
   , paramsArray(aParamsArray)
  {
  }

  
  int64_t currentTime;

  
  int64_t purgeTime;

  
  int64_t oldestTime;

  
  ArrayType &purgeList;

  
  nsIMutableArray *removedList;

  
  mozIStorageBindingParamsArray *paramsArray;
};


class CompareCookiesByAge {
public:
  bool Equals(const nsListIter &a, const nsListIter &b) const
  {
    return a.Cookie()->LastAccessed() == b.Cookie()->LastAccessed() &&
           a.Cookie()->CreationTime() == b.Cookie()->CreationTime();
  }

  bool LessThan(const nsListIter &a, const nsListIter &b) const
  {
    
    int64_t result = a.Cookie()->LastAccessed() - b.Cookie()->LastAccessed();
    if (result != 0)
      return result < 0;

    return a.Cookie()->CreationTime() < b.Cookie()->CreationTime();
  }
};


class CompareCookiesByIndex {
public:
  bool Equals(const nsListIter &a, const nsListIter &b) const
  {
    NS_ASSERTION(a.entry != b.entry || a.index != b.index,
      "cookie indexes should never be equal");
    return false;
  }

  bool LessThan(const nsListIter &a, const nsListIter &b) const
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
  mozIStorageBindingParamsArray *array = data.paramsArray;
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ) {
    nsListIter iter(aEntry, i);
    nsCookie *cookie = cookies[i];

    
    if (cookie->Expiry() <= data.currentTime) {
      data.removedList->AppendElement(cookie, false);
      COOKIE_LOGEVICTED(cookie, "Cookie expired");

      
      gCookieService->RemoveCookieFromList(iter, array);

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


already_AddRefed<nsIArray>
nsCookieService::PurgeCookies(int64_t aCurrentTimeInUsec)
{
  NS_ASSERTION(mDBState->hostTable.Count() > 0, "table is empty");
  EnsureReadComplete();

#ifdef PR_LOGGING
  uint32_t initialCookieCount = mDBState->cookieCount;
  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("PurgeCookies(): beginning purge with %ld cookies and %lld oldest age",
     mDBState->cookieCount, aCurrentTimeInUsec - mDBState->cookieOldestTime));
#endif

  nsAutoTArray<nsListIter, kMaxNumberOfCookies> purgeList;

  nsCOMPtr<nsIMutableArray> removedList = do_CreateInstance(NS_ARRAY_CONTRACTID);

  
  
  mozIStorageAsyncStatement *stmt = mDBState->stmtDelete;
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  if (mDBState->dbConn) {
    stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  }

  nsPurgeData data(aCurrentTimeInUsec / PR_USEC_PER_SEC,
    aCurrentTimeInUsec - mCookiePurgeAge, purgeList, removedList, paramsArray);
  mDBState->hostTable.EnumerateEntries(purgeCookiesCallback, &data);

#ifdef PR_LOGGING
  uint32_t postExpiryCookieCount = mDBState->cookieCount;
#endif

  
  
  purgeList.Sort(CompareCookiesByAge());

  
  uint32_t excess = mDBState->cookieCount > mMaxNumberOfCookies ?
    mDBState->cookieCount - mMaxNumberOfCookies : 0;
  if (purgeList.Length() > excess) {
    
    data.oldestTime = purgeList[excess].Cookie()->LastAccessed();

    purgeList.SetLength(excess);
  }

  
  
  
  purgeList.Sort(CompareCookiesByIndex());
  for (nsPurgeData::ArrayType::index_type i = purgeList.Length(); i--; ) {
    nsCookie *cookie = purgeList[i].Cookie();
    removedList->AppendElement(cookie, false);
    COOKIE_LOGEVICTED(cookie, "Cookie too old");

    RemoveCookieFromList(purgeList[i], paramsArray);
  }

  
  if (paramsArray) {
    uint32_t length;
    paramsArray->GetLength(&length);
    if (length) {
      DebugOnly<nsresult> rv = stmt->BindParameters(paramsArray);
      NS_ASSERT_SUCCESS(rv);
      nsCOMPtr<mozIStoragePendingStatement> handle;
      rv = stmt->ExecuteAsync(mDBState->removeListener, getter_AddRefs(handle));
      NS_ASSERT_SUCCESS(rv);
    }
  }

  
  mDBState->cookieOldestTime = data.oldestTime;

  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("PurgeCookies(): %ld expired; %ld purged; %ld remain; %lld oldest age",
     initialCookieCount - postExpiryCookieCount,
     postExpiryCookieCount - mDBState->cookieCount,
     mDBState->cookieCount,
     aCurrentTimeInUsec - mDBState->cookieOldestTime));

  return removedList.forget();
}



NS_IMETHODIMP
nsCookieService::CookieExists(nsICookie2 *aCookie,
                              bool       *aFoundCookie)
{
  NS_ENSURE_ARG_POINTER(aCookie);

  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoCString host, name, path;
  nsresult rv = aCookie->GetHost(host);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCookie->GetName(name);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aCookie->GetPath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsListIter iter;
  *aFoundCookie = FindCookie(DEFAULT_APP_KEY(baseDomain), host, name, path, iter);
  return NS_OK;
}



void
nsCookieService::FindStaleCookie(nsCookieEntry *aEntry,
                                 int64_t aCurrentTime,
                                 nsListIter &aIter)
{
  aIter.entry = NULL;

  int64_t oldestTime = 0;
  const nsCookieEntry::ArrayType &cookies = aEntry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    nsCookie *cookie = cookies[i];

    
    if (cookie->Expiry() <= aCurrentTime) {
      aIter.entry = aEntry;
      aIter.index = i;
      return;
    }

    
    if (!aIter.entry || oldestTime > cookie->LastAccessed()) {
      oldestTime = cookie->LastAccessed();
      aIter.entry = aEntry;
      aIter.index = i;
    }
  }
}



NS_IMETHODIMP
nsCookieService::CountCookiesFromHost(const nsACString &aHost,
                                      uint32_t         *aCountFromHost)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsAutoCString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCookieKey key = DEFAULT_APP_KEY(baseDomain);
  EnsureReadDomain(key);

  
  nsCookieEntry *entry = mDBState->hostTable.GetEntry(key);
  *aCountFromHost = entry ? entry->GetCookies().Length() : 0;
  return NS_OK;
}



NS_IMETHODIMP
nsCookieService::GetCookiesFromHost(const nsACString     &aHost,
                                    nsISimpleEnumerator **aEnumerator)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsAutoCString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCookieKey key = DEFAULT_APP_KEY(baseDomain);
  EnsureReadDomain(key);

  nsCookieEntry *entry = mDBState->hostTable.GetEntry(key);
  if (!entry)
    return NS_NewEmptyEnumerator(aEnumerator);

  nsCOMArray<nsICookie> cookieList(mMaxCookiesPerHost);
  const nsCookieEntry::ArrayType &cookies = entry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    cookieList.AppendObject(cookies[i]);
  }

  return NS_NewArrayEnumerator(aEnumerator, cookieList);
}

namespace {







struct GetCookiesForAppStruct {
  uint32_t              appId;
  bool                  onlyBrowserElement;
  nsCOMArray<nsICookie> cookies;

  GetCookiesForAppStruct() MOZ_DELETE;
  GetCookiesForAppStruct(uint32_t aAppId, bool aOnlyBrowserElement)
    : appId(aAppId)
    , onlyBrowserElement(aOnlyBrowserElement)
  {}
};

} 

 PLDHashOperator
nsCookieService::GetCookiesForApp(nsCookieEntry* entry, void* arg)
{
  GetCookiesForAppStruct* data = static_cast<GetCookiesForAppStruct*>(arg);

  if (entry->mAppId != data->appId ||
      (data->onlyBrowserElement && !entry->mInBrowserElement)) {
    return PL_DHASH_NEXT;
  }

  const nsCookieEntry::ArrayType& cookies = entry->GetCookies();

  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    data->cookies.AppendObject(cookies[i]);
  }

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsCookieService::GetCookiesForApp(uint32_t aAppId, bool aOnlyBrowserElement,
                                  nsISimpleEnumerator** aEnumerator)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ENSURE_TRUE(aAppId != NECKO_UNKNOWN_APP_ID, NS_ERROR_INVALID_ARG);

  GetCookiesForAppStruct data(aAppId, aOnlyBrowserElement);
  mDBState->hostTable.EnumerateEntries(GetCookiesForApp, &data);

  return NS_NewArrayEnumerator(aEnumerator, data.cookies);
}

NS_IMETHODIMP
nsCookieService::RemoveCookiesForApp(uint32_t aAppId, bool aOnlyBrowserElement)
{
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  nsresult rv = GetCookiesForApp(aAppId, aOnlyBrowserElement,
                                 getter_AddRefs(enumerator));

  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsICookie> cookie;
    rv = enumerator->GetNext(getter_AddRefs(cookie));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoCString host;
    cookie->GetHost(host);

    nsAutoCString name;
    cookie->GetName(name);

    nsAutoCString path;
    cookie->GetPath(path);

    
    
    
    
    
    
    
    
    
    
    
    Remove(host, aAppId, true, name, path, false);
    if (!aOnlyBrowserElement) {
      Remove(host, aAppId, false, name, path, false);
    }
  }

  return NS_OK;
}


bool
nsCookieService::FindCookie(const nsCookieKey    &aKey,
                            const nsAFlatCString &aHost,
                            const nsAFlatCString &aName,
                            const nsAFlatCString &aPath,
                            nsListIter           &aIter)
{
  EnsureReadDomain(aKey);

  nsCookieEntry *entry = mDBState->hostTable.GetEntry(aKey);
  if (!entry)
    return false;

  const nsCookieEntry::ArrayType &cookies = entry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    nsCookie *cookie = cookies[i];

    if (aHost.Equals(cookie->Host()) &&
        aPath.Equals(cookie->Path()) &&
        aName.Equals(cookie->Name())) {
      aIter = nsListIter(entry, i);
      return true;
    }
  }

  return false;
}


void
nsCookieService::RemoveCookieFromList(const nsListIter              &aIter,
                                      mozIStorageBindingParamsArray *aParamsArray)
{
  
  if (!aIter.Cookie()->IsSession() && mDBState->dbConn) {
    
    
    mozIStorageAsyncStatement *stmt = mDBState->stmtDelete;
    nsCOMPtr<mozIStorageBindingParamsArray> paramsArray(aParamsArray);
    if (!paramsArray) {
      stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
    }

    nsCOMPtr<mozIStorageBindingParams> params;
    paramsArray->NewBindingParams(getter_AddRefs(params));

    DebugOnly<nsresult> rv =
      params->BindUTF8StringByName(NS_LITERAL_CSTRING("name"),
                                   aIter.Cookie()->Name());
    NS_ASSERT_SUCCESS(rv);

    rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("host"),
                                      aIter.Cookie()->Host());
    NS_ASSERT_SUCCESS(rv);

    rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("path"),
                                      aIter.Cookie()->Path());
    NS_ASSERT_SUCCESS(rv);

    rv = paramsArray->AddParams(params);
    NS_ASSERT_SUCCESS(rv);

    
    if (!aParamsArray) {
      rv = stmt->BindParameters(paramsArray);
      NS_ASSERT_SUCCESS(rv);
      nsCOMPtr<mozIStoragePendingStatement> handle;
      rv = stmt->ExecuteAsync(mDBState->removeListener, getter_AddRefs(handle));
      NS_ASSERT_SUCCESS(rv);
    }
  }

  if (aIter.entry->GetCookies().Length() == 1) {
    
    
    
    mDBState->hostTable.RawRemoveEntry(aIter.entry);

  } else {
    
    aIter.entry->GetCookies().RemoveElementAt(aIter.index);
  }

  --mDBState->cookieCount;
}

void
bindCookieParameters(mozIStorageBindingParamsArray *aParamsArray,
                     const nsCookieKey &aKey,
                     const nsCookie *aCookie)
{
  NS_ASSERTION(aParamsArray, "Null params array passed to bindCookieParameters!");
  NS_ASSERTION(aCookie, "Null cookie passed to bindCookieParameters!");

  
  
  nsCOMPtr<mozIStorageBindingParams> params;
  DebugOnly<nsresult> rv =
    aParamsArray->NewBindingParams(getter_AddRefs(params));
  NS_ASSERT_SUCCESS(rv);

  
  rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("baseDomain"),
                                    aKey.mBaseDomain);
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindInt32ByName(NS_LITERAL_CSTRING("appId"),
                               aKey.mAppId);
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindInt32ByName(NS_LITERAL_CSTRING("inBrowserElement"),
                               aKey.mInBrowserElement ? 1 : 0);
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("name"),
                                    aCookie->Name());
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("value"),
                                    aCookie->Value());
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("host"),
                                    aCookie->Host());
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("path"),
                                    aCookie->Path());
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindInt64ByName(NS_LITERAL_CSTRING("expiry"),
                               aCookie->Expiry());
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindInt64ByName(NS_LITERAL_CSTRING("lastAccessed"),
                               aCookie->LastAccessed());
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindInt64ByName(NS_LITERAL_CSTRING("creationTime"),
                               aCookie->CreationTime());
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindInt32ByName(NS_LITERAL_CSTRING("isSecure"),
                               aCookie->IsSecure());
  NS_ASSERT_SUCCESS(rv);

  rv = params->BindInt32ByName(NS_LITERAL_CSTRING("isHttpOnly"),
                               aCookie->IsHttpOnly());
  NS_ASSERT_SUCCESS(rv);

  
  rv = aParamsArray->AddParams(params);
  NS_ASSERT_SUCCESS(rv);
}

void
nsCookieService::AddCookieToList(const nsCookieKey             &aKey,
                                 nsCookie                      *aCookie,
                                 DBState                       *aDBState,
                                 mozIStorageBindingParamsArray *aParamsArray,
                                 bool                           aWriteToDB)
{
  NS_ASSERTION(!(aDBState->dbConn && !aWriteToDB && aParamsArray),
               "Not writing to the DB but have a params array?");
  NS_ASSERTION(!(!aDBState->dbConn && aParamsArray),
               "Do not have a DB connection but have a params array?");

  nsCookieEntry *entry = aDBState->hostTable.PutEntry(aKey);
  NS_ASSERTION(entry, "can't insert element into a null entry!");

  entry->GetCookies().AppendElement(aCookie);
  ++aDBState->cookieCount;

  
  if (aCookie->LastAccessed() < aDBState->cookieOldestTime)
    aDBState->cookieOldestTime = aCookie->LastAccessed();

  
  if (aWriteToDB && !aCookie->IsSession() && aDBState->dbConn) {
    mozIStorageAsyncStatement *stmt = aDBState->stmtInsert;
    nsCOMPtr<mozIStorageBindingParamsArray> paramsArray(aParamsArray);
    if (!paramsArray) {
      stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
    }
    bindCookieParameters(paramsArray, aKey, aCookie);

    
    
    if (!aParamsArray) {
      DebugOnly<nsresult> rv = stmt->BindParameters(paramsArray);
      NS_ASSERT_SUCCESS(rv);
      nsCOMPtr<mozIStoragePendingStatement> handle;
      rv = stmt->ExecuteAsync(mDBState->insertListener, getter_AddRefs(handle));
      NS_ASSERT_SUCCESS(rv);
    }
  }
}

void
nsCookieService::UpdateCookieInList(nsCookie                      *aCookie,
                                    int64_t                        aLastAccessed,
                                    mozIStorageBindingParamsArray *aParamsArray)
{
  NS_ASSERTION(aCookie, "Passing a null cookie to UpdateCookieInList!");

  
  aCookie->SetLastAccessed(aLastAccessed);

  
  if (!aCookie->IsSession() && aParamsArray) {
    
    nsCOMPtr<mozIStorageBindingParams> params;
    aParamsArray->NewBindingParams(getter_AddRefs(params));

    
    DebugOnly<nsresult> rv =
      params->BindInt64ByName(NS_LITERAL_CSTRING("lastAccessed"),
                              aLastAccessed);
    NS_ASSERT_SUCCESS(rv);

    rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("name"),
                                      aCookie->Name());
    NS_ASSERT_SUCCESS(rv);

    rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("host"),
                                      aCookie->Host());
    NS_ASSERT_SUCCESS(rv);

    rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("path"),
                                      aCookie->Path());
    NS_ASSERT_SUCCESS(rv);

    
    rv = aParamsArray->AddParams(params);
    NS_ASSERT_SUCCESS(rv);
  }
}

