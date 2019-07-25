









































#ifdef MOZ_LOGGING

#define FORCE_PR_LOG
#endif

#include "mozilla/net/CookieServiceChild.h"
#include "mozilla/net/NeckoCommon.h"

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
#include "mozIThirdPartyUtil.h"

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
#include "nsIPrivateBrowsingService.h"
#include "nsNetCID.h"
#include "mozilla/storage.h"
#include "mozilla/FunctionTimer.h"

using namespace mozilla::net;






static nsCookieService *gCookieService;



static const char kHttpOnlyPrefix[] = "#HttpOnly_";

#define COOKIES_FILE "cookies.sqlite"
#define COOKIES_SCHEMA_VERSION 4

static const PRInt64 kCookieStaleThreshold = 60 * PR_USEC_PER_SEC; 
static const PRInt64 kCookiePurgeAge =
  PRInt64(30 * 24 * 60 * 60) * PR_USEC_PER_SEC; 

static const char kOldCookieFileName[] = "cookies.txt";

#undef  LIMIT
#define LIMIT(x, low, high, default) ((x) >= (low) && (x) <= (high) ? (x) : (default))

#undef  ADD_TEN_PERCENT
#define ADD_TEN_PERCENT(i) ((i) + (i)/10)



static const PRUint32 kMaxNumberOfCookies = 3000;
static const PRUint32 kMaxCookiesPerHost  = 150;
static const PRUint32 kMaxBytesPerCookie  = 4096;
static const PRUint32 kMaxBytesPerPath    = 1024;


static const PRUint32 BEHAVIOR_ACCEPT        = 0;
static const PRUint32 BEHAVIOR_REJECTFOREIGN = 1;
static const PRUint32 BEHAVIOR_REJECT        = 2;


static const char kPrefCookieBehavior[]     = "network.cookie.cookieBehavior";
static const char kPrefMaxNumberOfCookies[] = "network.cookie.maxNumber";
static const char kPrefMaxCookiesPerHost[]  = "network.cookie.maxPerHost";
static const char kPrefCookiePurgeAge[]     = "network.cookie.purgeAge";
static const char kPrefThirdPartySession[]  = "network.cookie.thirdparty.sessionOnly";

static void
bindCookieParameters(mozIStorageBindingParamsArray *aParamsArray,
                     const nsCString &aBaseDomain,
                     const nsCookie *aCookie);


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







#ifdef MOZ_LOGGING






#include "prlog.h"
#endif


#define SET_COOKIE PR_TRUE
#define GET_COOKIE PR_FALSE

#ifdef PR_LOGGING
static PRLogModuleInfo *sCookieLog = PR_NewLogModule("cookie");

#define COOKIE_LOGFAILURE(a, b, c, d)    LogFailure(a, b, c, d)
#define COOKIE_LOGSUCCESS(a, b, c, d, e) LogSuccess(a, b, c, d, e)

#define COOKIE_LOGEVICTED(a, details)          \
  PR_BEGIN_MACRO                               \
    if (PR_LOG_TEST(sCookieLog, PR_LOG_DEBUG)) \
      LogEvicted(a, details);                  \
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

    PR_ExplodeTime(aCookie->CreationTime(), PR_GMTParameters, &explodedTime);
    PR_FormatTimeUSEnglish(timeString, 40, "%c GMT", &explodedTime);
    PR_LOG(sCookieLog, PR_LOG_DEBUG,("created: %s", timeString));

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
LogEvicted(nsCookie *aCookie, const char* details)
{
  PR_LOG(sCookieLog, PR_LOG_DEBUG,("===== COOKIE EVICTED =====\n"));
  PR_LOG(sCookieLog, PR_LOG_DEBUG,("%s\n", details));

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
  NS_DECL_ISUPPORTS

  NS_IMETHOD HandleError(mozIStorageError* aError)
  {
    PRInt32 result = -1;
    aError->GetResult(&result);

#ifdef PR_LOGGING
    nsCAutoString message;
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

NS_IMPL_ISUPPORTS1(DBListenerErrorHandler, mozIStorageStatementCallback)





class InsertCookieDBListener : public DBListenerErrorHandler
{
protected:
  virtual const char *GetOpType() { return "INSERT"; }

public:
  InsertCookieDBListener(DBState* dbState) : DBListenerErrorHandler(dbState) { }
  NS_IMETHOD HandleResult(mozIStorageResultSet*)
  {
    NS_NOTREACHED("Unexpected call to InsertCookieDBListener::HandleResult");
    return NS_OK;
  }
  NS_IMETHOD HandleCompletion(PRUint16 aReason)
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





class UpdateCookieDBListener : public DBListenerErrorHandler
{
protected:
  virtual const char *GetOpType() { return "UPDATE"; }

public:
  UpdateCookieDBListener(DBState* dbState) : DBListenerErrorHandler(dbState) { }
  NS_IMETHOD HandleResult(mozIStorageResultSet*)
  {
    NS_NOTREACHED("Unexpected call to UpdateCookieDBListener::HandleResult");
    return NS_OK;
  }
  NS_IMETHOD HandleCompletion(PRUint16 aReason)
  {
    return NS_OK;
  }
};





class RemoveCookieDBListener :  public DBListenerErrorHandler
{
protected:
  virtual const char *GetOpType() { return "REMOVE"; }

public:
  RemoveCookieDBListener(DBState* dbState) : DBListenerErrorHandler(dbState) { }
  NS_IMETHOD HandleResult(mozIStorageResultSet*)
  {
    NS_NOTREACHED("Unexpected call to RemoveCookieDBListener::HandleResult");
    return NS_OK;
  }
  NS_IMETHOD HandleCompletion(PRUint16 aReason)
  {
    return NS_OK;
  }
};





class ReadCookieDBListener :  public DBListenerErrorHandler
{
protected:
  virtual const char *GetOpType() { return "READ"; }
  bool mCanceled;

public:
  ReadCookieDBListener(DBState* dbState)
    : DBListenerErrorHandler(dbState)
    , mCanceled(false)
  {
  }

  void Cancel() { mCanceled = true; }

  NS_IMETHOD HandleResult(mozIStorageResultSet *aResult)
  {
    nsresult rv;
    nsCOMPtr<mozIStorageRow> row;

    while (1) {
      rv = aResult->GetNextRow(getter_AddRefs(row));
      NS_ASSERT_SUCCESS(rv);

      if (!row)
        break;

      CookieDomainTuple *tuple = mDBState->hostArray.AppendElement();
      row->GetUTF8String(9, tuple->baseDomain);
      tuple->cookie = gCookieService->GetCookieFromRow(row);
    }

    return NS_OK;
  }
  NS_IMETHOD HandleCompletion(PRUint16 aReason)
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






class CloseCookieDBListener :  public mozIStorageCompletionCallback
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






NS_IMPL_ISUPPORTS5(nsCookieService,
                   nsICookieService,
                   nsICookieManager,
                   nsICookieManager2,
                   nsIObserver,
                   nsISupportsWeakReference)

nsCookieService::nsCookieService()
 : mDBState(NULL)
 , mCookieBehavior(BEHAVIOR_ACCEPT)
 , mThirdPartySession(PR_FALSE)
 , mMaxNumberOfCookies(kMaxNumberOfCookies)
 , mMaxCookiesPerHost(kMaxCookiesPerHost)
 , mCookiePurgeAge(kCookiePurgeAge)
{
}

nsresult
nsCookieService::Init()
{
  NS_TIME_FUNCTION;

  nsresult rv;
  mTLDService = do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mIDNService = do_GetService(NS_IDNSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPrefBranch2> prefBranch = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefBranch) {
    prefBranch->AddObserver(kPrefCookieBehavior,     this, PR_TRUE);
    prefBranch->AddObserver(kPrefMaxNumberOfCookies, this, PR_TRUE);
    prefBranch->AddObserver(kPrefMaxCookiesPerHost,  this, PR_TRUE);
    prefBranch->AddObserver(kPrefCookiePurgeAge,     this, PR_TRUE);
    prefBranch->AddObserver(kPrefThirdPartySession,  this, PR_TRUE);
    PrefChanged(prefBranch);
  }

  mStorageService = do_GetService("@mozilla.org/storage/service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  InitDBStates();

  mObserverService = mozilla::services::GetObserverService();
  NS_ENSURE_STATE(mObserverService);
  mObserverService->AddObserver(this, "profile-before-change", PR_TRUE);
  mObserverService->AddObserver(this, "profile-do-change", PR_TRUE);
  mObserverService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_TRUE);

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

  
  nsCOMPtr<nsIPrivateBrowsingService> pbs =
    do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
  if (pbs) {
    PRBool inPrivateBrowsing = PR_FALSE;
    pbs->GetPrivateBrowsingEnabled(&inPrivateBrowsing);
    if (inPrivateBrowsing) {
      mPrivateDBState = new DBState();
      mDBState = mPrivateDBState;
    }
  }

  
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

  PRBool tableExists = PR_FALSE;
  mDefaultDBState->dbConn->TableExists(NS_LITERAL_CSTRING("moz_cookies"),
    &tableExists);
  if (!tableExists) {
    rv = CreateTable();
    NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  } else {
    
    PRInt32 dbSchemaVersion;
    rv = mDefaultDBState->dbConn->GetSchemaVersion(&dbSchemaVersion);
    NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

    
    mozStorageTransaction transaction(mDefaultDBState->dbConn, PR_TRUE);

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
        PRBool hasResult;
        while (1) {
          rv = select->ExecuteStep(&hasResult);
          NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

          if (!hasResult)
            break;

          PRInt64 id = select->AsInt64(0);
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

        
        PRBool hasResult;
        rv = select->ExecuteStep(&hasResult);
        NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

        if (hasResult) {
          nsCString name1, host1, path1;
          PRInt64 id1 = select->AsInt64(0);
          select->GetUTF8String(1, name1);
          select->GetUTF8String(2, host1);
          select->GetUTF8String(3, path1);

          nsCString name2, host2, path2;
          while (1) {
            
            rv = select->ExecuteStep(&hasResult);
            NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

            if (!hasResult)
              break;

            PRInt64 id2 = select->AsInt64(0);
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
    "PRAGMA journal_mode = WAL"));
  mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "PRAGMA wal_autocheckpoint = 16"));

  
  rv = mDefaultDBState->dbConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_cookies ("
      "baseDomain, "
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
  oldCookieFile->Remove(PR_FALSE);
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
      "name TEXT, "
      "value TEXT, "
      "host TEXT, "
      "path TEXT, "
      "expiry INTEGER, "
      "lastAccessed INTEGER, "
      "creationTime INTEGER, "
      "isSecure INTEGER, "
      "isHttpOnly INTEGER, "
      "CONSTRAINT moz_uniqueid UNIQUE (name, host, path)"
    ")"));
  if (NS_FAILED(rv)) return rv;

  
  return mDefaultDBState->dbConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_basedomain ON moz_cookies (baseDomain)"));
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
      CancelAsyncRead(PR_TRUE);
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
    
    mObserverService->NotifyObservers(nsnull, "cookie-db-closed", nsnull);
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
    mObserverService->NotifyObservers(nsnull, "cookie-db-closed", nsnull);
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
      CancelAsyncRead(PR_TRUE);
      mDefaultDBState->syncConn = nsnull;
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
      bindCookieParameters(paramsArray, aEntry->GetKey(), cookie);
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
    mObserverService->NotifyObservers(nsnull, "cookie-db-closed", nsnull);
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
    mObserverService->NotifyObservers(nsnull, "cookie-db-closed", nsnull);
    return;
  }

  
  mObserverService->NotifyObservers(nsnull, "cookie-db-rebuilding", nsnull);

  
  mozIStorageAsyncStatement* stmt = aDBState->stmtInsert;
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  aDBState->hostTable.EnumerateEntries(RebuildDBCallback, paramsArray.get());

  
  PRUint32 length;
  paramsArray->GetLength(&length);
  if (length == 0) {
    COOKIE_LOGSTRING(PR_LOG_DEBUG,
      ("RebuildCorruptDB(): nothing to write, rebuild complete"));
    mDefaultDBState->corruptFlag = DBState::OK;
    return;
  }

  
  nsresult rv = stmt->BindParameters(paramsArray);
  NS_ASSERT_SUCCESS(rv);
  nsCOMPtr<mozIStoragePendingStatement> handle;
  rv = stmt->ExecuteAsync(aDBState->insertListener, getter_AddRefs(handle));
  NS_ASSERT_SUCCESS(rv);    
}

nsCookieService::~nsCookieService()
{
  CloseDBStates();

  gCookieService = nsnull;
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

  } else if (!strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC)) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(aData)) {
      NS_ASSERTION(mDefaultDBState, "don't have a default state");
      NS_ASSERTION(mDBState == mDefaultDBState, "not in default state");
      NS_ASSERTION(!mPrivateDBState, "already have a private state");

      
      mPrivateDBState = new DBState();
      mDBState = mPrivateDBState;

    } else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData)) {
      NS_ASSERTION(mDefaultDBState, "don't have a default state");
      NS_ASSERTION(mDBState == mPrivateDBState, "not in private state");
      NS_ASSERTION(!mPrivateDBState->dbConn, "private DB connection not null");

      
      mPrivateDBState = NULL;
      mDBState = mDefaultDBState;
    }

    NotifyChanged(nsnull, NS_LITERAL_STRING("reload").get());
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

  
  PRBool isForeign = true;
  if (RequireThirdPartyCheck())
    mThirdPartyUtil->IsThirdPartyChannel(aChannel, aHostURI, &isForeign);

  nsCAutoString result;
  GetCookieStringInternal(aHostURI, isForeign, aHttpBound, result);
  *aCookie = result.IsEmpty() ? nsnull : ToNewCString(result);
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

  
  PRBool isForeign = true;
  if (RequireThirdPartyCheck())
    mThirdPartyUtil->IsThirdPartyChannel(aChannel, aHostURI, &isForeign);

  nsDependentCString cookieString(aCookieHeader);
  nsDependentCString serverTime(aServerTime ? aServerTime : "");
  SetCookieStringInternal(aHostURI, isForeign, cookieString,
                          serverTime, aFromHttp);
  return NS_OK;
}

void
nsCookieService::SetCookieStringInternal(nsIURI          *aHostURI,
                                         bool             aIsForeign,
                                         const nsCString &aCookieHeader,
                                         const nsCString &aServerTime,
                                         PRBool           aFromHttp) 
{
  NS_ASSERTION(aHostURI, "null host!");

  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return;
  }

  
  
  
  
  
  PRBool requireHostMatch;
  nsCAutoString baseDomain;
  nsresult rv = GetBaseDomain(aHostURI, baseDomain, requireHostMatch);
  if (NS_FAILED(rv)) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader, 
                      "couldn't get base domain from URI");
    return;
  }

  
  CookieStatus cookieStatus = CheckPrefs(aHostURI, aIsForeign, baseDomain,
                                         requireHostMatch, aCookieHeader.get());
  
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
  PRInt64 serverTime;
  PRStatus result = PR_ParseTimeString(aServerTime.get(), PR_TRUE,
                                       &tempServerTime);
  if (result == PR_SUCCESS) {
    serverTime = tempServerTime / PR_USEC_PER_SEC;
  } else {
    serverTime = PR_Now() / PR_USEC_PER_SEC;
  }

  
  nsDependentCString cookieHeader(aCookieHeader);
  while (SetCookieInternal(aHostURI, baseDomain, requireHostMatch,
                           cookieStatus, cookieHeader, serverTime, aFromHttp)) {
    
    if (!aFromHttp)
      break;
  }
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

already_AddRefed<nsIArray>
nsCookieService::CreatePurgeList(nsICookie2* aCookie)
{
  nsCOMPtr<nsIMutableArray> removedList =
    do_CreateInstance(NS_ARRAY_CONTRACTID);
  removedList->AppendElement(aCookie, PR_FALSE);
  return removedList.forget();
}






void
nsCookieService::PrefChanged(nsIPrefBranch *aPrefBranch)
{
  PRInt32 val;
  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefCookieBehavior, &val)))
    mCookieBehavior = (PRUint8) LIMIT(val, 0, 2, 0);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefMaxNumberOfCookies, &val)))
    mMaxNumberOfCookies = (PRUint16) LIMIT(val, 1, 0xFFFF, kMaxNumberOfCookies);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefMaxCookiesPerHost, &val)))
    mMaxCookiesPerHost = (PRUint16) LIMIT(val, 1, 0xFFFF, kMaxCookiesPerHost);

  if (NS_SUCCEEDED(aPrefBranch->GetIntPref(kPrefCookiePurgeAge, &val))) {
    mCookiePurgeAge =
      PRInt64(LIMIT(val, 0, PR_INT32_MAX, PR_INT32_MAX)) * PR_USEC_PER_SEC;
  }

  PRBool boolval;
  if (NS_SUCCEEDED(aPrefBranch->GetBoolPref(kPrefThirdPartySession, &boolval)))
    mThirdPartySession = boolval;

  
  if (!mThirdPartyUtil && RequireThirdPartyCheck()) {
    mThirdPartyUtil = do_GetService(THIRDPARTYUTIL_CONTRACTID);
    NS_ABORT_IF_FALSE(mThirdPartyUtil, "require ThirdPartyUtil service");
  }
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
      CancelAsyncRead(PR_TRUE);
    }

    nsCOMPtr<mozIStorageStatement> stmt;
    nsresult rv = mDefaultDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
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

  NotifyChanged(nsnull, NS_LITERAL_STRING("cleared").get());
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
                     PRBool            aIsSecure,
                     PRBool            aIsHttpOnly,
                     PRBool            aIsSession,
                     PRInt64           aExpiry)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
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
                     nsCookie::GenerateUniqueCreationTime(currentTimeInUsec),
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
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsCAutoString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  nsListIter matchIter;
  nsRefPtr<nsCookie> cookie;
  if (FindCookie(baseDomain,
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







OpenDBResult
nsCookieService::Read()
{
  
  
  nsCOMPtr<mozIStorageStatement> stmtRead;
  nsresult rv = mDefaultDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
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
      "baseDomain "
    "FROM moz_cookies "
    "WHERE baseDomain NOTNULL"), getter_AddRefs(stmtRead));
  NS_ENSURE_SUCCESS(rv, RESULT_RETRY);

  
  
  
  
  nsCOMPtr<mozIStorageStatement> stmtDeleteNull;
  rv = mDefaultDBState->dbConn->CreateStatement(NS_LITERAL_CSTRING(
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
  nsresult rv = aRow->GetUTF8String(0, name);
  NS_ASSERT_SUCCESS(rv);
  rv = aRow->GetUTF8String(1, value);
  NS_ASSERT_SUCCESS(rv);
  rv = aRow->GetUTF8String(2, host);
  NS_ASSERT_SUCCESS(rv);
  rv = aRow->GetUTF8String(3, path);
  NS_ASSERT_SUCCESS(rv);

  PRInt64 expiry = aRow->AsInt64(4);
  PRInt64 lastAccessed = aRow->AsInt64(5);
  PRInt64 creationTime = aRow->AsInt64(6);
  PRBool isSecure = 0 != aRow->AsInt32(7);
  PRBool isHttpOnly = 0 != aRow->AsInt32(8);

  
  return nsCookie::Create(name, value, host, path,
                          expiry,
                          lastAccessed,
                          creationTime,
                          PR_FALSE,
                          isSecure,
                          isHttpOnly);
}

void
nsCookieService::AsyncReadComplete()
{
  
  
  
  NS_ASSERTION(mDefaultDBState, "no default DBState");
  NS_ASSERTION(mDefaultDBState->pendingRead, "no pending read");
  NS_ASSERTION(mDefaultDBState->readListener, "no read listener");

  
  
  
  for (PRUint32 i = 0; i < mDefaultDBState->hostArray.Length(); ++i) {
    const CookieDomainTuple &tuple = mDefaultDBState->hostArray[i];

    
    
    
    if (mDefaultDBState->readSet.GetEntry(tuple.baseDomain))
      continue;

    AddCookieToList(tuple.baseDomain, tuple.cookie, mDefaultDBState, NULL,
      PR_FALSE);
  }

  mDefaultDBState->stmtReadDomain = nsnull;
  mDefaultDBState->pendingRead = nsnull;
  mDefaultDBState->readListener = nsnull;
  mDefaultDBState->syncConn = nsnull;
  mDefaultDBState->hostArray.Clear();
  mDefaultDBState->readSet.Clear();

  COOKIE_LOGSTRING(PR_LOG_DEBUG, ("Read(): %ld cookies read",
                                  mDefaultDBState->cookieCount));

  mObserverService->NotifyObservers(nsnull, "cookie-db-read", nsnull);
}

void
nsCookieService::CancelAsyncRead(PRBool aPurgeReadSet)
{
  
  
  
  NS_ASSERTION(mDefaultDBState, "no default DBState");
  NS_ASSERTION(mDefaultDBState->pendingRead, "no pending read");
  NS_ASSERTION(mDefaultDBState->readListener, "no read listener");

  
  
  mDefaultDBState->readListener->Cancel();
  nsresult rv = mDefaultDBState->pendingRead->Cancel();
  NS_ASSERT_SUCCESS(rv);

  mDefaultDBState->stmtReadDomain = nsnull;
  mDefaultDBState->pendingRead = nsnull;
  mDefaultDBState->readListener = nsnull;
  mDefaultDBState->hostArray.Clear();

  
  
  if (aPurgeReadSet)
    mDefaultDBState->readSet.Clear();
}

void
nsCookieService::EnsureReadDomain(const nsCString &aBaseDomain)
{
  NS_ASSERTION(!mDBState->dbConn || mDBState == mDefaultDBState,
    "not in default db state");

  
  if (NS_LIKELY(!mDBState->dbConn || !mDefaultDBState->pendingRead))
    return;

  
  if (NS_LIKELY(mDefaultDBState->readSet.GetEntry(aBaseDomain)))
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
      "WHERE baseDomain = :baseDomain"),
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
    NS_LITERAL_CSTRING("baseDomain"), aBaseDomain);
  NS_ASSERT_SUCCESS(rv);

  PRBool hasResult;
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

  
  
  for (PRUint32 i = 0; i < array.Length(); ++i) {
    AddCookieToList(aBaseDomain, array[i], mDefaultDBState, NULL, PR_FALSE);
  }

  
  mDefaultDBState->readSet.PutEntry(aBaseDomain);

  COOKIE_LOGSTRING(PR_LOG_DEBUG,
    ("EnsureReadDomain(): %ld cookies read for base domain %s",
     array.Length(), aBaseDomain.get()));
}

void
nsCookieService::EnsureReadComplete()
{
  NS_ASSERTION(!mDBState->dbConn || mDBState == mDefaultDBState,
    "not in default db state");

  
  if (NS_LIKELY(!mDBState->dbConn || !mDefaultDBState->pendingRead))
    return;

  
  CancelAsyncRead(PR_FALSE);

  
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
      "baseDomain "
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
  PRBool hasResult;
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
    if (mDefaultDBState->readSet.GetEntry(baseDomain))
      continue;

    CookieDomainTuple* tuple = array.AppendElement();
    tuple->baseDomain = baseDomain;
    tuple->cookie = GetCookieFromRow(stmt);
  }

  
  
  for (PRUint32 i = 0; i < array.Length(); ++i) {
    CookieDomainTuple& tuple = array[i];
    AddCookieToList(tuple.baseDomain, tuple.cookie, mDefaultDBState, NULL,
      PR_FALSE);
  }

  mDefaultDBState->syncConn = nsnull;
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

  nsCAutoString buffer, baseDomain;
  PRBool isMore = PR_TRUE;
  PRInt32 hostIndex, isDomainIndex, pathIndex, secureIndex, expiresIndex, nameIndex, cookieIndex;
  nsASingleFragmentCString::char_iterator iter;
  PRInt32 numInts;
  PRInt64 expires;
  PRBool isDomain, isHttpOnly = PR_FALSE;
  PRUint32 originalCookieCount = mDefaultDBState->cookieCount;

  PRInt64 currentTimeInUsec = PR_Now();
  PRInt64 currentTime = currentTimeInUsec / PR_USEC_PER_SEC;
  
  
  PRInt64 lastAccessedCounter = currentTimeInUsec;

  












  









  
  
  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  if (originalCookieCount == 0 && mDefaultDBState->dbConn) {
    mDefaultDBState->stmtInsert->NewBindingParamsArray(getter_AddRefs(paramsArray));
  }

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
                       nsCookie::GenerateUniqueCreationTime(currentTimeInUsec),
                       PR_FALSE,
                       Substring(buffer, secureIndex, expiresIndex - secureIndex - 1).EqualsLiteral(kTrue),
                       isHttpOnly);
    if (!newCookie) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    
    
    lastAccessedCounter--;

    if (originalCookieCount == 0) {
      AddCookieToList(baseDomain, newCookie, mDefaultDBState, paramsArray);
    }
    else {
      AddInternal(baseDomain, newCookie, currentTimeInUsec, NULL, NULL, PR_TRUE);
    }
  }

  
  if (paramsArray) {
    PRUint32 length;
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







static inline PRBool ispathdelimiter(char c) { return c == '/' || c == '?' || c == '#' || c == ';'; }


class CompareCookiesForSending
{
public:
  PRBool Equals(const nsCookie* aCookie1, const nsCookie* aCookie2) const
  {
    return aCookie1->CreationTime() == aCookie2->CreationTime() &&
           aCookie2->Path().Length() == aCookie1->Path().Length();
  }

  PRBool LessThan(const nsCookie* aCookie1, const nsCookie* aCookie2) const
  {
    
    PRInt32 result = aCookie2->Path().Length() - aCookie1->Path().Length();
    if (result != 0)
      return result < 0;

    
    
    
    
    return aCookie1->CreationTime() < aCookie2->CreationTime();
  }
};

void
nsCookieService::GetCookieStringInternal(nsIURI *aHostURI,
                                         bool aIsForeign,
                                         PRBool aHttpBound,
                                         nsCString &aCookieString)
{
  NS_ASSERTION(aHostURI, "null host!");

  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return;
  }

  
  
  
  
  
  PRBool requireHostMatch;
  nsCAutoString baseDomain, hostFromURI, pathFromURI;
  nsresult rv = GetBaseDomain(aHostURI, baseDomain, requireHostMatch);
  if (NS_SUCCEEDED(rv))
    rv = aHostURI->GetAsciiHost(hostFromURI);
  if (NS_SUCCEEDED(rv))
    rv = aHostURI->GetPath(pathFromURI);
  if (NS_FAILED(rv)) {
    COOKIE_LOGFAILURE(GET_COOKIE, aHostURI, nsnull, "invalid host/path from URI");
    return;
  }

  
  CookieStatus cookieStatus = CheckPrefs(aHostURI, aIsForeign, baseDomain,
                                         requireHostMatch, nsnull);
  
  switch (cookieStatus) {
  case STATUS_REJECTED:
  case STATUS_REJECTED_WITH_ERROR:
    return;
  default:
    break;
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

  EnsureReadDomain(baseDomain);

  
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
    
    
    nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
    mozIStorageAsyncStatement* stmt = mDBState->stmtUpdate;
    if (mDBState->dbConn) {
      stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
    }

    for (PRInt32 i = 0; i < count; ++i) {
      cookie = foundCookieList.ElementAt(i);

      if (currentTimeInUsec - cookie->LastAccessed() > kCookieStaleThreshold)
        UpdateCookieInList(cookie, currentTimeInUsec, paramsArray);
    }
    
    if (paramsArray) {
      PRUint32 length;
      paramsArray->GetLength(&length);
      if (length) {
        nsresult rv = stmt->BindParameters(paramsArray);
        NS_ASSERT_SUCCESS(rv);
        nsCOMPtr<mozIStoragePendingStatement> handle;
        rv = stmt->ExecuteAsync(mDBState->updateListener,
          getter_AddRefs(handle));
        NS_ASSERT_SUCCESS(rv);
      }
    }
  }

  
  
  
  foundCookieList.Sort(CompareCookiesForSending());

  for (PRInt32 i = 0; i < count; ++i) {
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
    COOKIE_LOGSUCCESS(GET_COOKIE, aHostURI, aCookieString, nsnull, nsnull);
}



PRBool
nsCookieService::SetCookieInternal(nsIURI                        *aHostURI,
                                   const nsCString               &aBaseDomain,
                                   PRBool                         aRequireHostMatch,
                                   CookieStatus                   aStatus,
                                   nsDependentCString            &aCookieHeader,
                                   PRInt64                        aServerTime,
                                   PRBool                         aFromHttp)
{
  NS_ASSERTION(aHostURI, "null host!");

  
  
  nsCookieAttributes cookieAttributes;

  
  cookieAttributes.expiryTime = LL_MAXINT;

  
  
  nsDependentCString savedCookieHeader(aCookieHeader);

  
  
  PRBool newCookie = ParseAttributes(aCookieHeader, cookieAttributes);

  PRInt64 currentTimeInUsec = PR_Now();

  
  cookieAttributes.isSession = GetExpiry(cookieAttributes, aServerTime,
                                         currentTimeInUsec / PR_USEC_PER_SEC);
  if (aStatus == STATUS_ACCEPT_SESSION) {
    
    
    cookieAttributes.isSession = PR_TRUE;
  }

  
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
                     nsCookie::GenerateUniqueCreationTime(currentTimeInUsec),
                     cookieAttributes.isSession,
                     cookieAttributes.isSecure,
                     cookieAttributes.isHttpOnly);
  if (!cookie)
    return newCookie;

  
  
  if (mPermissionService) {
    PRBool permission;
    
    
    
    mPermissionService->CanSetCookie(aHostURI,
                                     nsnull,
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
nsCookieService::AddInternal(const nsCString               &aBaseDomain,
                             nsCookie                      *aCookie,
                             PRInt64                        aCurrentTimeInUsec,
                             nsIURI                        *aHostURI,
                             const char                    *aCookieHeader,
                             PRBool                         aFromHttp)
{
  PRInt64 currentTime = aCurrentTimeInUsec / PR_USEC_PER_SEC;

  
  if (!aFromHttp && aCookie->IsHttpOnly()) {
    COOKIE_LOGFAILURE(SET_COOKIE, aHostURI, aCookieHeader,
      "cookie is httponly; coming from script");
    return;
  }

  nsListIter matchIter;
  PRBool foundCookie = FindCookie(aBaseDomain, aCookie->Host(),
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

      
      
      
      foundCookie = PR_FALSE;

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

    
    nsCookieEntry *entry = mDBState->hostTable.GetEntry(aBaseDomain);
    if (entry && entry->GetCookies().Length() >= mMaxCookiesPerHost) {
      nsListIter iter;
      FindStaleCookie(entry, currentTime, iter);
      oldCookie = iter.Cookie();

      
      RemoveCookieFromList(iter);
      COOKIE_LOGEVICTED(oldCookie, "Too many cookies for this domain");
      purgedList = CreatePurgeList(oldCookie);

    } else if (mDBState->cookieCount >= ADD_TEN_PERCENT(mMaxNumberOfCookies)) {
      PRInt64 maxAge = aCurrentTimeInUsec - mDBState->cookieOldestTime;
      PRInt64 purgeAge = ADD_TEN_PERCENT(mCookiePurgeAge);
      if (maxAge >= purgeAge) {
        
        
        
        
        
        
        purgedList = PurgeCookies(aCurrentTimeInUsec);
      }
    }
  }

  
  
  AddCookieToList(aBaseDomain, aCookie, mDBState, NULL);
  COOKIE_LOGSUCCESS(SET_COOKIE, aHostURI, aCookieHeader, aCookie, foundCookie);

  
  
  if (purgedList) {
    NotifyChanged(purgedList, NS_LITERAL_STRING("batch-deleted").get());
  }

  NotifyChanged(aCookie, foundCookie ? NS_LITERAL_STRING("changed").get()
                                     : NS_LITERAL_STRING("added").get());
}












































































static inline PRBool iswhitespace     (char c) { return c == ' '  || c == '\t'; }
static inline PRBool isterminator     (char c) { return c == '\n' || c == '\r'; }
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

    
    
    while (aIter != aEndIter && !isvalueseparator(*aIter))
      ++aIter;

    
    if (aIter != start) {
      lastSpace = aIter;
      while (--lastSpace != start && iswhitespace(*lastSpace));
      aTokenValue.Rebind(start, ++lastSpace);
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

  
  if (aBaseDomain.Length() == 1 && aBaseDomain.Last() == '.')
    return NS_ERROR_INVALID_ARG;

  
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
  
  if (aHost.Length() == 1 && aHost.Last() == '.')
    return NS_ERROR_INVALID_ARG;

  
  PRBool domain = !aHost.IsEmpty() && aHost.First() == '.';

  
  
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
    nsCAutoString host;
    nsresult rv = mIDNService->ConvertUTF8toACE(aHost, host);
    if (NS_FAILED(rv))
      return rv;

    aHost = host;
  }

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

bool
nsCookieService::RequireThirdPartyCheck()
{
  
  return mCookieBehavior == BEHAVIOR_REJECTFOREIGN || mThirdPartySession;
}

CookieStatus
nsCookieService::CheckPrefs(nsIURI          *aHostURI,
                            bool             aIsForeign,
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
    
    
    rv = mPermissionService->CanAccess(aHostURI, nsnull, &access);

    
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

  
  if (mCookieBehavior == BEHAVIOR_REJECT) {
    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "cookies are disabled");
    return STATUS_REJECTED;
  }

  if (RequireThirdPartyCheck() && aIsForeign) {
    
    if (mCookieBehavior == BEHAVIOR_ACCEPT && mThirdPartySession)
      return STATUS_ACCEPT_SESSION;

    COOKIE_LOGFAILURE(aCookieHeader ? SET_COOKIE : GET_COOKIE, aHostURI, aCookieHeader, "context is third party");
    return STATUS_REJECTED;
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
    PRTime expires;

    
    if (PR_ParseTimeString(aCookieAttributes.expires.get(), PR_TRUE, &expires) != PR_SUCCESS) {
      return PR_TRUE;
    }

    delta = expires / PR_USEC_PER_SEC - aServerTime;

  
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
              nsIMutableArray *aRemovedList,
              mozIStorageBindingParamsArray *aParamsArray)
   : currentTime(aCurrentTime)
   , purgeTime(aPurgeTime)
   , oldestTime(LL_MAXINT)
   , purgeList(aPurgeList)
   , removedList(aRemovedList)
   , paramsArray(aParamsArray)
  {
  }

  
  PRInt64 currentTime;

  
  PRInt64 purgeTime;

  
  PRInt64 oldestTime;

  
  ArrayType &purgeList;

  
  nsIMutableArray *removedList;

  
  mozIStorageBindingParamsArray *paramsArray;
};


class CompareCookiesByAge {
public:
  PRBool Equals(const nsListIter &a, const nsListIter &b) const
  {
    return a.Cookie()->LastAccessed() == b.Cookie()->LastAccessed() &&
           a.Cookie()->CreationTime() == b.Cookie()->CreationTime();
  }

  PRBool LessThan(const nsListIter &a, const nsListIter &b) const
  {
    
    PRInt64 result = a.Cookie()->LastAccessed() - b.Cookie()->LastAccessed();
    if (result != 0)
      return result < 0;

    return a.Cookie()->CreationTime() < b.Cookie()->CreationTime();
  }
};


class CompareCookiesByIndex {
public:
  PRBool Equals(const nsListIter &a, const nsListIter &b) const
  {
    NS_ASSERTION(a.entry != b.entry || a.index != b.index,
      "cookie indexes should never be equal");
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
  mozIStorageBindingParamsArray *array = data.paramsArray;
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ) {
    nsListIter iter(aEntry, i);
    nsCookie *cookie = cookies[i];

    
    if (cookie->Expiry() <= data.currentTime) {
      data.removedList->AppendElement(cookie, PR_FALSE);
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
nsCookieService::PurgeCookies(PRInt64 aCurrentTimeInUsec)
{
  NS_ASSERTION(mDBState->hostTable.Count() > 0, "table is empty");
  EnsureReadComplete();

#ifdef PR_LOGGING
  PRUint32 initialCookieCount = mDBState->cookieCount;
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
  PRUint32 postExpiryCookieCount = mDBState->cookieCount;
#endif

  
  
  purgeList.Sort(CompareCookiesByAge());

  
  PRUint32 excess = mDBState->cookieCount > mMaxNumberOfCookies ?
    mDBState->cookieCount - mMaxNumberOfCookies : 0;
  if (purgeList.Length() > excess) {
    
    data.oldestTime = purgeList[excess].Cookie()->LastAccessed();

    purgeList.SetLength(excess);
  }

  
  
  
  purgeList.Sort(CompareCookiesByIndex());
  for (nsPurgeData::ArrayType::index_type i = purgeList.Length(); i--; ) {
    nsCookie *cookie = purgeList[i].Cookie();
    removedList->AppendElement(cookie, PR_FALSE);
    COOKIE_LOGEVICTED(cookie, "Cookie too old");

    RemoveCookieFromList(purgeList[i], paramsArray);
  }

  
  if (paramsArray) {
    PRUint32 length;
    paramsArray->GetLength(&length);
    if (length) {
      nsresult rv = stmt->BindParameters(paramsArray);
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
                              PRBool     *aFoundCookie)
{
  NS_ENSURE_ARG_POINTER(aCookie);

  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

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
  *aFoundCookie = FindCookie(baseDomain, host, name, path, iter);
  return NS_OK;
}



void
nsCookieService::FindStaleCookie(nsCookieEntry *aEntry,
                                 PRInt64 aCurrentTime,
                                 nsListIter &aIter)
{
  aIter.entry = NULL;

  PRInt64 oldestTime;
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
                                      PRUint32         *aCountFromHost)
{
  if (!mDBState) {
    NS_WARNING("No DBState! Profile already closed?");
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsCAutoString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  EnsureReadDomain(baseDomain);

  
  nsCookieEntry *entry = mDBState->hostTable.GetEntry(baseDomain);
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

  
  nsCAutoString host(aHost);
  nsresult rv = NormalizeHost(host);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString baseDomain;
  rv = GetBaseDomainFromHost(host, baseDomain);
  NS_ENSURE_SUCCESS(rv, rv);

  EnsureReadDomain(baseDomain);

  nsCookieEntry *entry = mDBState->hostTable.GetEntry(baseDomain);
  if (!entry)
    return NS_NewEmptyEnumerator(aEnumerator);

  nsCOMArray<nsICookie> cookieList(mMaxCookiesPerHost);
  const nsCookieEntry::ArrayType &cookies = entry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    cookieList.AppendObject(cookies[i]);
  }

  return NS_NewArrayEnumerator(aEnumerator, cookieList);
}


PRBool
nsCookieService::FindCookie(const nsCString      &aBaseDomain,
                            const nsAFlatCString &aHost,
                            const nsAFlatCString &aName,
                            const nsAFlatCString &aPath,
                            nsListIter           &aIter)
{
  EnsureReadDomain(aBaseDomain);

  nsCookieEntry *entry = mDBState->hostTable.GetEntry(aBaseDomain);
  if (!entry)
    return PR_FALSE;

  const nsCookieEntry::ArrayType &cookies = entry->GetCookies();
  for (nsCookieEntry::IndexType i = 0; i < cookies.Length(); ++i) {
    nsCookie *cookie = cookies[i];

    if (aHost.Equals(cookie->Host()) &&
        aPath.Equals(cookie->Path()) &&
        aName.Equals(cookie->Name())) {
      aIter = nsListIter(entry, i);
      return PR_TRUE;
    }
  }

  return PR_FALSE;
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

    nsresult rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("name"),
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
                     const nsCString &aBaseDomain,
                     const nsCookie *aCookie)
{
  NS_ASSERTION(aParamsArray, "Null params array passed to bindCookieParameters!");
  NS_ASSERTION(aCookie, "Null cookie passed to bindCookieParameters!");
  nsresult rv;

  
  
  nsCOMPtr<mozIStorageBindingParams> params;
  rv = aParamsArray->NewBindingParams(getter_AddRefs(params));
  NS_ASSERT_SUCCESS(rv);

  
  rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("baseDomain"),
                                    aBaseDomain);
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
nsCookieService::AddCookieToList(const nsCString               &aBaseDomain,
                                 nsCookie                      *aCookie,
                                 DBState                       *aDBState,
                                 mozIStorageBindingParamsArray *aParamsArray,
                                 PRBool                         aWriteToDB)
{
  NS_ASSERTION(!(aDBState->dbConn && !aWriteToDB && aParamsArray),
               "Not writing to the DB but have a params array?");
  NS_ASSERTION(!(!aDBState->dbConn && aParamsArray),
               "Do not have a DB connection but have a params array?");

  nsCookieEntry *entry = aDBState->hostTable.PutEntry(aBaseDomain);
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
    bindCookieParameters(paramsArray, aBaseDomain, aCookie);

    
    
    if (!aParamsArray) {
      nsresult rv = stmt->BindParameters(paramsArray);
      NS_ASSERT_SUCCESS(rv);
      nsCOMPtr<mozIStoragePendingStatement> handle;
      rv = stmt->ExecuteAsync(mDBState->insertListener, getter_AddRefs(handle));
      NS_ASSERT_SUCCESS(rv);
    }
  }
}

void
nsCookieService::UpdateCookieInList(nsCookie                      *aCookie,
                                    PRInt64                        aLastAccessed,
                                    mozIStorageBindingParamsArray *aParamsArray)
{
  NS_ASSERTION(aCookie, "Passing a null cookie to UpdateCookieInList!");

  
  aCookie->SetLastAccessed(aLastAccessed);

  
  if (!aCookie->IsSession() && aParamsArray) {
    
    nsCOMPtr<mozIStorageBindingParams> params;
    aParamsArray->NewBindingParams(getter_AddRefs(params));

    
    nsresult rv = params->BindInt64ByName(NS_LITERAL_CSTRING("lastAccessed"),
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

