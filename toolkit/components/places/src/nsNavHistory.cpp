











































#include <stdio.h>
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsAnnotationService.h"
#include "nsPlacesTables.h"

#include "nsIArray.h"
#include "nsTArray.h"
#include "nsArrayEnumerator.h"
#include "nsCollationCID.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsDebug.h"
#include "nsEnumeratorUtils.h"
#include "nsFaviconService.h"
#include "nsIChannelEventSink.h"
#include "nsIComponentManager.h"
#include "nsILocaleService.h"
#include "nsILocalFile.h"
#include "nsIPrefBranch2.h"
#include "nsIServiceManager.h"
#include "nsISimpleEnumerator.h"
#include "nsISupportsPrimitives.h"
#include "nsIURI.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "nsPromiseFlatString.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "prsystem.h"
#include "prtime.h"
#include "prprf.h"
#include "nsEscape.h"
#include "nsIVariant.h"
#include "nsVariant.h"
#include "nsIEffectiveTLDService.h"
#include "nsIIDNService.h"
#include "nsIClassInfoImpl.h"
#include "nsThreadUtils.h"

#include "mozIStorageConnection.h"
#include "mozIStorageFunction.h"
#include "mozIStoragePendingStatement.h"
#include "mozIStorageService.h"
#include "mozIStorageStatement.h"
#include "mozIStorageValueArray.h"
#include "mozStorageCID.h"
#include "mozStorageHelper.h"
#include "nsPlacesTriggers.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIIdleService.h"
#include "nsILivemarkService.h"

#include "nsMathUtils.h" 




#define RECENT_EVENT_THRESHOLD (15 * 60 * PR_USEC_PER_SEC)



#define BOOKMARK_REDIRECT_TIME_THRESHOLD (2 * 60 * PR_USEC_PER_SEC)






#define RECENT_EVENT_QUEUE_MAX_LENGTH 128


#define PREF_BRANCH_BASE                        "browser."
#define PREF_BROWSER_HISTORY_EXPIRE_DAYS_MIN    "history_expire_days_min"
#define PREF_BROWSER_HISTORY_EXPIRE_DAYS_MAX    "history_expire_days"
#define PREF_BROWSER_HISTORY_EXPIRE_SITES       "history_expire_sites"
#define PREF_AUTOCOMPLETE_ENABLED               "urlbar.autocomplete.enabled"
#define PREF_AUTOCOMPLETE_MATCH_BEHAVIOR        "urlbar.matchBehavior"
#define PREF_AUTOCOMPLETE_FILTER_JAVASCRIPT     "urlbar.filter.javascript"
#define PREF_AUTOCOMPLETE_ENABLED               "urlbar.autocomplete.enabled"
#define PREF_AUTOCOMPLETE_MAX_RICH_RESULTS      "urlbar.maxRichResults"
#define PREF_AUTOCOMPLETE_DEFAULT_BEHAVIOR      "urlbar.default.behavior"
#define PREF_AUTOCOMPLETE_RESTRICT_HISTORY      "urlbar.restrict.history"
#define PREF_AUTOCOMPLETE_RESTRICT_BOOKMARK     "urlbar.restrict.bookmark"
#define PREF_AUTOCOMPLETE_RESTRICT_TAG          "urlbar.restrict.tag"
#define PREF_AUTOCOMPLETE_MATCH_TITLE           "urlbar.match.title"
#define PREF_AUTOCOMPLETE_MATCH_URL             "urlbar.match.url"
#define PREF_AUTOCOMPLETE_RESTRICT_TYPED        "urlbar.restrict.typed"
#define PREF_AUTOCOMPLETE_SEARCH_CHUNK_SIZE     "urlbar.search.chunkSize"
#define PREF_AUTOCOMPLETE_SEARCH_TIMEOUT        "urlbar.search.timeout"
#define PREF_DB_CACHE_PERCENTAGE                "history_cache_percentage"
#define PREF_FRECENCY_NUM_VISITS                "places.frecency.numVisits"
#define PREF_FRECENCY_CALC_ON_IDLE              "places.frecency.numCalcOnIdle"
#define PREF_FRECENCY_CALC_ON_MIGRATE           "places.frecency.numCalcOnMigrate"
#define PREF_FRECENCY_UPDATE_IDLE_TIME          "places.frecency.updateIdleTime"
#define PREF_FRECENCY_FIRST_BUCKET_CUTOFF       "places.frecency.firstBucketCutoff"
#define PREF_FRECENCY_SECOND_BUCKET_CUTOFF      "places.frecency.secondBucketCutoff"
#define PREF_FRECENCY_THIRD_BUCKET_CUTOFF       "places.frecency.thirdBucketCutoff"
#define PREF_FRECENCY_FOURTH_BUCKET_CUTOFF      "places.frecency.fourthBucketCutoff"
#define PREF_FRECENCY_FIRST_BUCKET_WEIGHT       "places.frecency.firstBucketWeight"
#define PREF_FRECENCY_SECOND_BUCKET_WEIGHT      "places.frecency.secondBucketWeight"
#define PREF_FRECENCY_THIRD_BUCKET_WEIGHT       "places.frecency.thirdBucketWeight"
#define PREF_FRECENCY_FOURTH_BUCKET_WEIGHT      "places.frecency.fourthBucketWeight"
#define PREF_FRECENCY_DEFAULT_BUCKET_WEIGHT     "places.frecency.defaultBucketWeight"
#define PREF_FRECENCY_EMBED_VISIT_BONUS         "places.frecency.embedVisitBonus"
#define PREF_FRECENCY_LINK_VISIT_BONUS          "places.frecency.linkVisitBonus"
#define PREF_FRECENCY_TYPED_VISIT_BONUS         "places.frecency.typedVisitBonus"
#define PREF_FRECENCY_BOOKMARK_VISIT_BONUS      "places.frecency.bookmarkVisitBonus"
#define PREF_FRECENCY_DOWNLOAD_VISIT_BONUS      "places.frecency.downloadVisitBonus"
#define PREF_FRECENCY_PERM_REDIRECT_VISIT_BONUS "places.frecency.permRedirectVisitBonus"
#define PREF_FRECENCY_TEMP_REDIRECT_VISIT_BONUS "places.frecency.tempRedirectVisitBonus"
#define PREF_FRECENCY_DEFAULT_VISIT_BONUS       "places.frecency.defaultVisitBonus"
#define PREF_FRECENCY_UNVISITED_BOOKMARK_BONUS  "places.frecency.unvisitedBookmarkBonus"
#define PREF_FRECENCY_UNVISITED_TYPED_BONUS     "places.frecency.unvisitedTypedBonus"






#define DEFAULT_DB_CACHE_PERCENTAGE 6






#define DEFAULT_DB_PAGE_SIZE 4096


#define HISTORY_EXPIRE_NOW_TIMEOUT (3 * PR_MSEC_PER_SEC)



#define HISTORY_URI_LENGTH_MAX 65536
#define HISTORY_TITLE_LENGTH_MAX 4096


#define DB_FILENAME NS_LITERAL_STRING("places.sqlite")


#define DB_CORRUPT_FILENAME NS_LITERAL_STRING("places.sqlite.corrupt")



#ifdef LAZY_ADD


#define LAZY_MESSAGE_TIMEOUT (3 * PR_MSEC_PER_SEC)



#define MAX_LAZY_TIMER_DEFERMENTS 2

#endif 


#define EXPIRE_IDLE_TIME_IN_MSECS (5 * 60 * PR_MSEC_PER_SEC)


#define MAX_EXPIRE_RECORDS_ON_IDLE 200


#define EXPIRATION_CAP_SITES 40000


#define CHARSET_ANNO NS_LITERAL_CSTRING("URIProperties/characterSet")

NS_IMPL_THREADSAFE_ADDREF(nsNavHistory)
NS_IMPL_THREADSAFE_RELEASE(nsNavHistory)

NS_INTERFACE_MAP_BEGIN(nsNavHistory)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryService)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIGlobalHistory2, nsIGlobalHistory3)
  NS_INTERFACE_MAP_ENTRY(nsIGlobalHistory3)
  NS_INTERFACE_MAP_ENTRY(nsIDownloadHistory)
  NS_INTERFACE_MAP_ENTRY(nsIBrowserHistory)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsICharsetResolver)
  NS_INTERFACE_MAP_ENTRY(nsPIPlacesDatabase)
  NS_INTERFACE_MAP_ENTRY(nsPIPlacesHistoryListenersNotifier)
#ifdef MOZ_XUL
  NS_INTERFACE_MAP_ENTRY(nsIAutoCompleteSearch)
  NS_INTERFACE_MAP_ENTRY(nsIAutoCompleteSimpleResultListener)
#endif
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryService)
  NS_IMPL_QUERY_CLASSINFO(nsNavHistory)
NS_INTERFACE_MAP_END


NS_IMPL_CI_INTERFACE_GETTER5(
  nsNavHistory
, nsINavHistoryService
, nsIGlobalHistory3
, nsIGlobalHistory2
, nsIDownloadHistory
, nsIBrowserHistory
)

static nsresult GetReversedHostname(nsIURI* aURI, nsAString& host);
static void GetReversedHostname(const nsString& aForward, nsAString& aReversed);
static nsresult GenerateTitleFromURI(nsIURI* aURI, nsAString& aTitle);
static PRInt64 GetSimpleBookmarksQueryFolder(
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions);
static void ParseSearchTermsFromQueries(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                        nsTArray<nsTArray<nsString>*>* aTerms);

inline void ReverseString(const nsString& aInput, nsAString& aReversed)
{
  aReversed.Truncate(0);
  for (PRInt32 i = aInput.Length() - 1; i >= 0; i --)
    aReversed.Append(aInput[i]);
}






class UpdateBatchScoper
{
public:
  UpdateBatchScoper(nsNavHistory& aNavHistory) : mNavHistory(aNavHistory)
  {
    mNavHistory.BeginUpdateBatch();
  }
  ~UpdateBatchScoper()
  {
    mNavHistory.EndUpdateBatch();
  }
protected:
  nsNavHistory& mNavHistory;
};

class PlacesEvent : public nsRunnable {
  public:
  PlacesEvent(const char* aTopic) {
    mTopic = aTopic;
  }

  NS_IMETHOD Run() {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->NotifyObservers(nsnull, mTopic, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
  protected:
  const char* mTopic;
};



const PRInt32 nsNavHistory::kGetInfoIndex_PageID = 0;
const PRInt32 nsNavHistory::kGetInfoIndex_URL = 1;
const PRInt32 nsNavHistory::kGetInfoIndex_Title = 2;
const PRInt32 nsNavHistory::kGetInfoIndex_RevHost = 3;
const PRInt32 nsNavHistory::kGetInfoIndex_VisitCount = 4;
const PRInt32 nsNavHistory::kGetInfoIndex_VisitDate = 5;
const PRInt32 nsNavHistory::kGetInfoIndex_FaviconURL = 6;
const PRInt32 nsNavHistory::kGetInfoIndex_SessionId = 7;
const PRInt32 nsNavHistory::kGetInfoIndex_ItemId = 8;
const PRInt32 nsNavHistory::kGetInfoIndex_ItemDateAdded = 9;
const PRInt32 nsNavHistory::kGetInfoIndex_ItemLastModified = 10;

const PRInt32 nsNavHistory::kAutoCompleteIndex_URL = 0;
const PRInt32 nsNavHistory::kAutoCompleteIndex_Title = 1;
const PRInt32 nsNavHistory::kAutoCompleteIndex_FaviconURL = 2;
const PRInt32 nsNavHistory::kAutoCompleteIndex_ParentId = 3;
const PRInt32 nsNavHistory::kAutoCompleteIndex_BookmarkTitle = 4;
const PRInt32 nsNavHistory::kAutoCompleteIndex_Tags = 5;
const PRInt32 nsNavHistory::kAutoCompleteIndex_VisitCount = 6;
const PRInt32 nsNavHistory::kAutoCompleteIndex_Typed = 7;

const PRInt32 nsNavHistory::kAutoCompleteBehaviorHistory = 1 << 0;
const PRInt32 nsNavHistory::kAutoCompleteBehaviorBookmark = 1 << 1;
const PRInt32 nsNavHistory::kAutoCompleteBehaviorTag = 1 << 2;
const PRInt32 nsNavHistory::kAutoCompleteBehaviorTitle = 1 << 3;
const PRInt32 nsNavHistory::kAutoCompleteBehaviorUrl = 1 << 4;
const PRInt32 nsNavHistory::kAutoCompleteBehaviorTyped = 1 << 5;

static const char* gQuitApplicationGrantedMessage = "quit-application-granted";
static const char* gXpcomShutdown = "xpcom-shutdown";
static const char* gAutoCompleteFeedback = "autocomplete-will-enter-text";
static const char* gIdleDaily = "idle-daily";


const char nsNavHistory::kAnnotationPreviousEncoding[] = "history/encoding";






static const PRInt64 USECS_PER_DAY = LL_INIT(20, 500654080);

nsNavHistory *nsNavHistory::gHistoryService = nsnull;

nsNavHistory *
nsNavHistory::GetSingleton()
{
  if (gHistoryService) {
    NS_ADDREF(gHistoryService);
    return gHistoryService;
  }

  gHistoryService = new nsNavHistory();
  if (gHistoryService) {
    NS_ADDREF(gHistoryService);
    if (NS_FAILED(gHistoryService->Init()))
      NS_RELEASE(gHistoryService);
  }

  return gHistoryService;
}



nsNavHistory::nsNavHistory() : mBatchLevel(0),
                               mBatchHasTransaction(PR_FALSE),
                               mNowValid(PR_FALSE),
                               mExpireNowTimer(nsnull),
                               mExpire(this),
                               mAutoCompleteEnabled(PR_TRUE),
                               mAutoCompleteMatchBehavior(MATCH_BOUNDARY_ANYWHERE),
                               mAutoCompleteMaxResults(25),
                               mAutoCompleteRestrictHistory(NS_LITERAL_STRING("^")),
                               mAutoCompleteRestrictBookmark(NS_LITERAL_STRING("*")),
                               mAutoCompleteRestrictTag(NS_LITERAL_STRING("+")),
                               mAutoCompleteMatchTitle(NS_LITERAL_STRING("#")),
                               mAutoCompleteMatchUrl(NS_LITERAL_STRING("@")),
                               mAutoCompleteRestrictTyped(NS_LITERAL_STRING("~")),
                               mAutoCompleteSearchChunkSize(100),
                               mAutoCompleteSearchTimeout(100),
                               mAutoCompleteDefaultBehavior(0),
                               mAutoCompleteCurrentBehavior(0),
                               mPreviousChunkOffset(-1),
                               mAutoCompleteFinishedSearch(PR_FALSE),
                               mExpireDaysMin(0),
                               mExpireDaysMax(0),
                               mExpireSites(0),
                               mNumVisitsForFrecency(10),
                               mTagsFolder(-1),
                               mInPrivateBrowsing(PRIVATEBROWSING_NOTINITED),
                               mDatabaseStatus(DATABASE_STATUS_OK)
{
#ifdef LAZY_ADD
  mLazyTimerSet = PR_TRUE;
  mLazyTimerDeferments = 0;
#endif
  NS_ASSERTION(! gHistoryService, "YOU ARE CREATING 2 COPIES OF THE HISTORY SERVICE. Everything will break.");
  gHistoryService = this;
}




nsNavHistory::~nsNavHistory()
{
  
  
  NS_ASSERTION(gHistoryService == this, "YOU CREATED 2 COPIES OF THE HISTORY SERVICE.");
  gHistoryService = nsnull;
}




nsresult
nsNavHistory::Init()
{
  nsresult rv;

  
  nsCOMPtr<nsIPrefService> prefService =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prefService->GetBranch(PREF_BRANCH_BASE, getter_AddRefs(mPrefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  
  LoadPrefs(PR_TRUE);

  
  rv = InitDBFile(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = InitDB();
  if (NS_FAILED(rv)) {
    
    
    rv = InitDBFile(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = InitDB();
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCOMPtr<PlacesEvent> completeEvent = new PlacesEvent(PLACES_INIT_COMPLETE_EVENT_TOPIC);
  rv = NS_DispatchToMainThread(completeEvent);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef MOZ_XUL
  rv = InitAutoComplete();
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  
  
  
  
  
  
  
  
  {
    nsCOMPtr<mozIStorageStatement> selectSession;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT MAX(session) FROM moz_historyvisits "
        "WHERE visit_date = "
        "(SELECT MAX(visit_date) from moz_historyvisits)"),
      getter_AddRefs(selectSession));
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hasSession;
    if (NS_SUCCEEDED(selectSession->ExecuteStep(&hasSession)) && hasSession)
      mLastSessionID = selectSession->AsInt64(0);
    else
      mLastSessionID = 1;
  }

  
  InitializeIdleTimer();

  
  NS_ENSURE_TRUE(mRecentTyped.Init(128), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentBookmark.Init(128), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentRedirects.Init(128), NS_ERROR_OUT_OF_MEMORY);

  







  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch2> pbi = do_QueryInterface(mPrefBranch);
  if (pbi) {
    pbi->AddObserver(PREF_AUTOCOMPLETE_ENABLED, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_MATCH_BEHAVIOR, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_FILTER_JAVASCRIPT, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_MAX_RICH_RESULTS, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_DEFAULT_BEHAVIOR, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_RESTRICT_HISTORY, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_RESTRICT_BOOKMARK, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_RESTRICT_TAG, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_MATCH_TITLE, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_MATCH_URL, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_RESTRICT_TYPED, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_SEARCH_CHUNK_SIZE, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_SEARCH_TIMEOUT, this, PR_FALSE);
    pbi->AddObserver(PREF_BROWSER_HISTORY_EXPIRE_DAYS_MAX, this, PR_FALSE);
    pbi->AddObserver(PREF_BROWSER_HISTORY_EXPIRE_DAYS_MIN, this, PR_FALSE);
    pbi->AddObserver(PREF_BROWSER_HISTORY_EXPIRE_SITES, this, PR_FALSE);
  }

  observerService->AddObserver(this, gQuitApplicationGrantedMessage, PR_FALSE);
  observerService->AddObserver(this, gXpcomShutdown, PR_FALSE);
  observerService->AddObserver(this, gAutoCompleteFeedback, PR_FALSE);
  observerService->AddObserver(this, gIdleDaily, PR_FALSE);
  observerService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_FALSE);
  
  
  
  if (mDatabaseStatus == DATABASE_STATUS_CREATE ||
      mDatabaseStatus == DATABASE_STATUS_UPGRADED) {
    (void)observerService->AddObserver(this, PLACES_INIT_COMPLETE_EVENT_TOPIC,
                                       PR_FALSE);
  }

  







  if (mDatabaseStatus == DATABASE_STATUS_CREATE) {
    nsCOMPtr<nsIFile> historyFile;
    rv = NS_GetSpecialDirectory(NS_APP_HISTORY_50_FILE,
                                getter_AddRefs(historyFile));
    if (NS_SUCCEEDED(rv) && historyFile) {
      ImportHistory(historyFile);
    }
  }

  
  

  return NS_OK;
}


nsresult
nsNavHistory::InitDBFile(PRBool aForceInit)
{
  if (aForceInit) {
    NS_ASSERTION(mDBConn,
                 "When forcing initialization, a database connection must exist!");
    NS_ASSERTION(mDBService,
                 "When forcing initialization, the database service must exist!");
  }

  
  nsCOMPtr<nsIFile> profDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(profDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = profDir->Clone(getter_AddRefs(mDBFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBFile->Append(DB_FILENAME);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aForceInit) {
    
    nsCOMPtr<nsIFile> backup;
    rv = mDBService->BackupDatabaseFile(mDBFile, DB_CORRUPT_FILENAME, profDir,
                                        getter_AddRefs(backup));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    mDatabaseStatus = DATABASE_STATUS_CORRUPT;
  }
  else {
    
    PRBool dbExists = PR_TRUE;
    rv = mDBFile->Exists(&dbExists);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!dbExists)
      mDatabaseStatus = DATABASE_STATUS_CREATE;
  }

  
  mDBService = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBService->OpenUnsharedDatabase(mDBFile, getter_AddRefs(mDBConn));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    mDatabaseStatus = DATABASE_STATUS_CORRUPT;

    
    nsCOMPtr<nsIFile> backup;
    rv = mDBService->BackupDatabaseFile(mDBFile, DB_CORRUPT_FILENAME, profDir,
                                        getter_AddRefs(backup));
    NS_ENSURE_SUCCESS(rv, rv);
 
    
    rv = mDBFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = profDir->Clone(getter_AddRefs(mDBFile));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBFile->Append(DB_FILENAME);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBService->OpenUnsharedDatabase(mDBFile, getter_AddRefs(mDBConn));
  }
 
  if (rv != NS_OK && rv != NS_ERROR_FILE_CORRUPTED) {
    
    
    
    nsCOMPtr<PlacesEvent> lockedEvent = new PlacesEvent(PLACES_DB_LOCKED_EVENT_TOPIC);
    (void)NS_DispatchToMainThread(lockedEvent);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




#define PLACES_SCHEMA_VERSION 8

nsresult
nsNavHistory::InitDB()
{
  nsresult rv;
  PRBool tableExists;

  
  
  
  
  
  
  nsCAutoString pageSizePragma("PRAGMA page_size=");
  pageSizePragma.AppendInt(DEFAULT_DB_PAGE_SIZE);
  rv = mDBConn->ExecuteSimpleSQL(pageSizePragma);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef IN_MEMORY_SQLITE_TEMP_STORE
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "PRAGMA temp_store = MEMORY"));
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  
  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "PRAGMA synchronous = FULL"));
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  
  
  rv = nsNavBookmarks::InitTables(mDBConn);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = nsFaviconService::InitTables(mDBConn);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = nsAnnotationService::InitTables(mDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->TableExists(NS_LITERAL_CSTRING("moz_places"), &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!tableExists) {
    rv = UpdateSchemaVersion();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRInt32 DBSchemaVersion;
  rv = mDBConn->GetSchemaVersion(&DBSchemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);
   
  if (PLACES_SCHEMA_VERSION != DBSchemaVersion) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (DBSchemaVersion < PLACES_SCHEMA_VERSION) {
      
      mDatabaseStatus = DATABASE_STATUS_UPGRADED;

      
      if (DBSchemaVersion < 3) {
        rv = MigrateV3Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      if (DBSchemaVersion < 5) {
        rv = ForceMigrateBookmarksDB(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      if (DBSchemaVersion < 6) {
        rv = MigrateV6Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      if (DBSchemaVersion < 7) {
        rv = MigrateV7Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      if (DBSchemaVersion < 8) {
        rv = MigrateV8Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

    } else {
      

      
      

      

      
      
      if (DBSchemaVersion > 2 && DBSchemaVersion < 6) {
        
        rv = ForceMigrateBookmarksDB(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    
    rv = UpdateSchemaVersion();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  PRInt32 pageSize;
  {
    nsCOMPtr<mozIStorageStatement> statement;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("PRAGMA page_size"),
                                  getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = statement->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(hasResult, NS_ERROR_FAILURE);
    pageSize = statement->AsInt32(0);
  }

  
  PRInt32 cachePercentage;
  if (NS_FAILED(mPrefBranch->GetIntPref(PREF_DB_CACHE_PERCENTAGE,
                                        &cachePercentage)))
    cachePercentage = DEFAULT_DB_CACHE_PERCENTAGE;
  if (cachePercentage > 50)
    cachePercentage = 50; 
  if (cachePercentage < 0)
    cachePercentage = 0;
  PRInt64 cacheSize = PR_GetPhysicalMemorySize() * cachePercentage / 100;
  PRInt64 cachePages = cacheSize / pageSize;

  
  nsCAutoString cacheSizePragma("PRAGMA cache_size=");
  cacheSizePragma.AppendInt(cachePages);
  rv = mDBConn->ExecuteSimpleSQL(cacheSizePragma);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("PRAGMA locking_mode = EXCLUSIVE"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("PRAGMA journal_mode = TRUNCATE"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!tableExists) {
    rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_PLACES);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE UNIQUE INDEX moz_places_url_uniqueindex ON moz_places (url)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_places_faviconindex ON moz_places (favicon_id)"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_places_hostindex ON moz_places (rev_host)"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_places_visitcount ON moz_places (visit_count)"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_places_frecencyindex ON moz_places (frecency)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = mDBConn->TableExists(NS_LITERAL_CSTRING("moz_historyvisits"), &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! tableExists) {
    rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_HISTORYVISITS);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_historyvisits_placedateindex "
        "ON moz_historyvisits (place_id, visit_date)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_historyvisits_fromindex ON moz_historyvisits (from_visit)"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_historyvisits_dateindex ON moz_historyvisits (visit_date)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = mDBConn->TableExists(NS_LITERAL_CSTRING("moz_inputhistory"), &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!tableExists) {
    rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_INPUTHISTORY);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  

  

  rv = InitTempTables();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = InitViews();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = InitFunctions();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = InitStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::InitializeIdleTimer()
{
  if (mIdleTimer) {
    mIdleTimer->Cancel();
    mIdleTimer = nsnull;
  }
  nsresult rv;
  mIdleTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 idleTimerTimeout = EXPIRE_IDLE_TIME_IN_MSECS;
  rv = mIdleTimer->InitWithFuncCallback(IdleTimerCallback, this,
                                        idleTimerTimeout,
                                        nsITimer::TYPE_REPEATING_SLACK);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::GetDatabaseStatus(PRUint16 *aDatabaseStatus)
{
  *aDatabaseStatus = mDatabaseStatus;
  return NS_OK;
}




nsresult
nsNavHistory::UpdateSchemaVersion()
{
  return mDBConn->SetSchemaVersion(PLACES_SCHEMA_VERSION);
}





class mozStorageFunctionGetUnreversedHost: public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION
};

NS_IMPL_ISUPPORTS1(mozStorageFunctionGetUnreversedHost, mozIStorageFunction)

NS_IMETHODIMP mozStorageFunctionGetUnreversedHost::OnFunctionCall(
  mozIStorageValueArray* aFunctionArguments,
  nsIVariant** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoString src;
  aFunctionArguments->GetString(0, src);

  nsresult rv;
  nsCOMPtr<nsIWritableVariant> result(do_CreateInstance(
      "@mozilla.org/variant;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  if (src.Length()>1) {
    src.Truncate(src.Length() - 1);
    nsAutoString dest;
    ReverseString(src, dest);
    result->SetAsAString(dest);
  } else {
    result->SetAsAString(NS_LITERAL_STRING(""));
  }
  NS_ADDREF(*_retval = result);
  return NS_OK;
}

nsresult
nsNavHistory::InitTempTables()
{
  nsresult rv;

  
  rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_PLACES_TEMP);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE UNIQUE INDEX moz_places_temp_url_uniqueindex ON moz_places_temp (url)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_places_temp_faviconindex ON moz_places_temp (favicon_id)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_places_temp_hostindex ON moz_places_temp (rev_host)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_places_temp_visitcount ON moz_places_temp (visit_count)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_places_temp_frecencyindex ON moz_places_temp (frecency)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_PLACES_SYNC_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);


  
  rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_HISTORYVISITS_TEMP);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_historyvisits_temp_placedateindex "
    "ON moz_historyvisits_temp (place_id, visit_date)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_historyvisits_temp_fromindex "
    "ON moz_historyvisits_temp (from_visit)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE INDEX moz_historyvisits_temp_dateindex "
    "ON moz_historyvisits_temp (visit_date)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_HISTORYVISITS_SYNC_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::InitViews()
{
  nsresult rv;

  
  rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_PLACES_VIEW);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(CREATE_PLACES_VIEW_INSERT_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(CREATE_PLACES_VIEW_DELETE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(CREATE_PLACES_VIEW_UPDATE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_HISTORYVISITS_VIEW);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->ExecuteSimpleSQL(CREATE_HISTORYVISITS_VIEW_INSERT_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(CREATE_HISTORYVISITS_VIEW_DELETE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(CREATE_HISTORYVISITS_VIEW_UPDATE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::InitFunctions()
{
  nsresult rv;

  rv = mDBConn->CreateFunction(
      NS_LITERAL_CSTRING("get_unreversed_host"), 1, 
      new mozStorageFunctionGetUnreversedHost);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





nsresult
nsNavHistory::InitStatements()
{
  nsresult rv;

  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, url, title, rev_host, visit_count "
    "FROM moz_places_temp "
    "WHERE url = ?1 "
    "UNION ALL "
    "SELECT id, url, title, rev_host, visit_count "
    "FROM moz_places "
    "WHERE url = ?1 "
    "LIMIT 1"),
    getter_AddRefs(mDBGetURLPageInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id, url, title, rev_host, visit_count "
      "FROM moz_places_temp "
      "WHERE id = ?1 "
      "UNION ALL "
      "SELECT id, url, title, rev_host, visit_count "
      "FROM moz_places "
      "WHERE id = ?1 "
      "LIMIT 1"),
    getter_AddRefs(mDBGetIdPageInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT * FROM ( "
        "SELECT v.id, v.session "
        "FROM moz_historyvisits_temp v "
        "WHERE v.place_id = IFNULL((SELECT id FROM moz_places_temp WHERE url = ?1), "
                                  "(SELECT id FROM moz_places WHERE url = ?1)) "
        "ORDER BY v.visit_date DESC LIMIT 1 "
      ") "
      "UNION ALL "
      "SELECT * FROM ( "
        "SELECT v.id, v.session "
        "FROM moz_historyvisits v "
        "WHERE v.place_id = IFNULL((SELECT id FROM moz_places_temp WHERE url = ?1), "
                                  "(SELECT id FROM moz_places WHERE url = ?1)) "
        "ORDER BY v.visit_date DESC LIMIT 1 "
      ") "
      "LIMIT 1"),
    getter_AddRefs(mDBRecentVisitOfURL));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id FROM moz_historyvisits_temp "
      "WHERE place_id = ?1 "
        "AND visit_date = ?2 "
        "AND session = ?3 "
      "UNION ALL "
      "SELECT id FROM moz_historyvisits "
      "WHERE place_id = ?1 "
        "AND visit_date = ?2 "
        "AND session = ?3 "
      "LIMIT 1"),
    getter_AddRefs(mDBRecentVisitOfPlace));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_historyvisits_view "
        "(from_visit, place_id, visit_date, visit_type, session) "
      "VALUES (?1, ?2, ?3, ?4, ?5)"),
    getter_AddRefs(mDBInsertVisit));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id, visit_count, typed, hidden "
      "FROM moz_places_temp "
      "WHERE url = ?1 "
      "UNION ALL "
      "SELECT id, visit_count, typed, hidden "
      "FROM moz_places "
      "WHERE url = ?1 "
      "LIMIT 1"),
    getter_AddRefs(mDBGetPageVisitStats));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id "
      "FROM moz_places_temp h "
      "WHERE url = ?1 " 
        "AND ( "
          "EXISTS(SELECT id FROM moz_historyvisits_temp WHERE place_id = h.id LIMIT 1) "
          "OR EXISTS(SELECT id FROM moz_historyvisits WHERE place_id = h.id LIMIT 1) "
        ") "
      "UNION ALL "
      "SELECT h.id "
      "FROM moz_places h "
      "WHERE url = ?1 "
      "AND ( "
        "EXISTS(SELECT id FROM moz_historyvisits_temp WHERE place_id = h.id LIMIT 1) "
        "OR EXISTS(SELECT id FROM moz_historyvisits WHERE place_id = h.id LIMIT 1) "
      ") "
      "LIMIT 1"), 
    getter_AddRefs(mDBIsPageVisited));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places_view "
      "SET hidden = ?2, typed = ?3 "
      "WHERE id = ?1"),
    getter_AddRefs(mDBUpdatePageVisitStats));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_places_view "
        "(url, title, rev_host, hidden, typed, frecency) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6)"),
    getter_AddRefs(mDBAddNewPage));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT GROUP_CONCAT(tag_title, ?1) FROM ("
        "SELECT t.title AS tag_title "
        "FROM moz_bookmarks b "
        "JOIN moz_bookmarks t ON t.id = b.parent "
        "WHERE b.fk = IFNULL((SELECT id FROM moz_places_temp WHERE url = ?3), "
                            "(SELECT id FROM moz_places WHERE url = ?3)) "
          "AND b.type = ") +
            nsPrintfCString("%d", nsINavBookmarksService::TYPE_BOOKMARK) +
        NS_LITERAL_CSTRING(" AND t.parent = ?2 ORDER BY t.title)"),
    getter_AddRefs(mDBGetTags));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT a.item_id, a.content "
    "FROM moz_anno_attributes n "
    "JOIN moz_items_annos a ON n.id = a.anno_attribute_id "
    "WHERE n.name = ?1"), 
    getter_AddRefs(mFoldersWithAnnotationQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places_view "
      "SET title = ?1 "
      "WHERE url = ?2"),
    getter_AddRefs(mDBSetPlaceTitle));
  NS_ENSURE_SUCCESS(rv, rv);


  
  
  
  
  
  
  
  
  

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT v.visit_date, COALESCE( "
        "(SELECT r.visit_type FROM moz_historyvisits_temp r "
          "WHERE v.visit_type IN ") +
            nsPrintfCString("(%d,%d) ", TRANSITION_REDIRECT_PERMANENT,
                                        TRANSITION_REDIRECT_TEMPORARY) +
            NS_LITERAL_CSTRING(" AND r.id = v.from_visit), "
        "(SELECT r.visit_type FROM moz_historyvisits r "
          "WHERE v.visit_type IN ") +
            nsPrintfCString("(%d,%d) ", TRANSITION_REDIRECT_PERMANENT,
                                        TRANSITION_REDIRECT_TEMPORARY) +
            NS_LITERAL_CSTRING(" AND r.id = v.from_visit), "
        "visit_type) "
      "FROM moz_historyvisits_temp v "
      "WHERE v.place_id = ?1 "
      "UNION ALL "
      "SELECT v.visit_date, COALESCE( "
        "(SELECT r.visit_type FROM moz_historyvisits_temp r "
          "WHERE v.visit_type IN ") +
            nsPrintfCString("(%d,%d) ", TRANSITION_REDIRECT_PERMANENT,
                                        TRANSITION_REDIRECT_TEMPORARY) +
            NS_LITERAL_CSTRING(" AND r.id = v.from_visit), "
        "(SELECT r.visit_type FROM moz_historyvisits r "
          "WHERE v.visit_type IN ") +
            nsPrintfCString("(%d,%d) ", TRANSITION_REDIRECT_PERMANENT,
                                        TRANSITION_REDIRECT_TEMPORARY) +
            NS_LITERAL_CSTRING(" AND r.id = v.from_visit), "
        "visit_type) "
      "FROM moz_historyvisits v "
      "WHERE v.place_id = ?1 "
        "AND v.id NOT IN (SELECT id FROM moz_historyvisits_temp) "
      "ORDER BY 1 DESC LIMIT ") +
        nsPrintfCString("%d", mNumVisitsForFrecency),
    getter_AddRefs(mDBVisitsForFrecency));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places_view SET frecency = ?2, hidden = ?3 WHERE id = ?1"),
    getter_AddRefs(mDBUpdateFrecencyAndHidden));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT typed, hidden, frecency "
      "FROM moz_places_temp WHERE id = ?1 "
      "UNION ALL "
      "SELECT typed, hidden, frecency "
      "FROM moz_places WHERE id = ?1 "
      "LIMIT 1"),
    getter_AddRefs(mDBGetPlaceVisitStats));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT "
        "(SELECT COUNT(*) FROM moz_historyvisits WHERE place_id = ?1) + "
        "(SELECT COUNT(*) FROM moz_historyvisits_temp WHERE place_id = ?1 "
            "AND id NOT IN (SELECT id FROM moz_historyvisits))"),
    getter_AddRefs(mDBFullVisitCount));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}









nsresult
nsNavHistory::ForceMigrateBookmarksDB(mozIStorageConnection* aDBConn) 
{
  
  nsresult rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TABLE IF EXISTS moz_bookmarks"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TABLE IF EXISTS moz_bookmarks_folders"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TABLE IF EXISTS moz_bookmarks_roots"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TABLE IF EXISTS moz_keywords"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = nsNavBookmarks::InitTables(aDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  mDatabaseStatus = DATABASE_STATUS_CREATE;

  return NS_OK;
}


nsresult
nsNavHistory::MigrateV3Up(mozIStorageConnection* aDBConn) 
{
  
  
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT type from moz_annos"),
    getter_AddRefs(statement));
  if (NS_SUCCEEDED(rv))
    return NS_OK;

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_annos ADD type INTEGER DEFAULT 0"));
  if (NS_FAILED(rv)) {
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP TABLE IF EXISTS moz_annos"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = nsAnnotationService::InitTables(mDBConn);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}


nsresult
nsNavHistory::MigrateV6Up(mozIStorageConnection* aDBConn) 
{
  mozStorageTransaction transaction(aDBConn, PR_FALSE);

  
  
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT a.dateAdded, a.lastModified FROM moz_annos a"), 
    getter_AddRefs(statement));
  if (NS_FAILED(rv)) {
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_annos ADD dateAdded INTEGER DEFAULT 0"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_annos ADD lastModified INTEGER DEFAULT 0"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.dateAdded, b.lastModified FROM moz_items_annos b"), 
    getter_AddRefs(statement));
  if (NS_FAILED(rv)) {
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_items_annos ADD dateAdded INTEGER DEFAULT 0"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_items_annos ADD lastModified INTEGER DEFAULT 0"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP INDEX IF EXISTS moz_favicons_url"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP INDEX IF EXISTS moz_anno_attributes_nameindex"));
  NS_ENSURE_SUCCESS(rv, rv);


  
  
  nsCOMPtr<mozIStorageStatement> statement2;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT user_title FROM moz_places"),
    getter_AddRefs(statement2));
  if (NS_SUCCEEDED(rv)) {
    
    
    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_places_urlindex"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_places_titleindex"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_places_faviconindex"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_places_hostindex"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_places_visitcount"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_places_frecencyindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = RemoveDuplicateURIs();
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_places RENAME TO moz_places_backup"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE TABLE moz_places ("
          "id INTEGER PRIMARY KEY, "
          "url LONGVARCHAR, "
          "title LONGVARCHAR, "
          "rev_host LONGVARCHAR, "
          "visit_count INTEGER DEFAULT 0, "
          "hidden INTEGER DEFAULT 0 NOT NULL, "
          "typed INTEGER DEFAULT 0 NOT NULL, "
          "favicon_id INTEGER, "
          "frecency INTEGER DEFAULT -1 NOT NULL)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE UNIQUE INDEX moz_places_url_uniqueindex ON moz_places (url)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_places_faviconindex ON moz_places (favicon_id)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_places_hostindex ON moz_places (rev_host)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_places_visitcount ON moz_places (visit_count)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_places_frecencyindex ON moz_places (frecency)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "INSERT INTO moz_places "
        "SELECT id, url, title, rev_host, visit_count, hidden, typed, "
          "favicon_id, frecency "
        "FROM moz_places_backup"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP TABLE moz_places_backup"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return transaction.Commit();
}


nsresult
nsNavHistory::MigrateV7Up(mozIStorageConnection* aDBConn) 
{
  mozStorageTransaction transaction(aDBConn, PR_FALSE);

  
  
  PRBool lastModIndexExists = PR_FALSE;
  nsresult rv = aDBConn->IndexExists(
    NS_LITERAL_CSTRING("moz_bookmarks_itemlastmodifiedindex"),
    &lastModIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!lastModIndexExists) {
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX moz_bookmarks_itemlastmodifiedindex "
        "ON moz_bookmarks (fk, lastModified)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  PRBool pageIndexExists = PR_FALSE;
  rv = aDBConn->IndexExists(
    NS_LITERAL_CSTRING("moz_historyvisits_pageindex"), &pageIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (pageIndexExists) {
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_historyvisits_pageindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX IF NOT EXISTS moz_historyvisits_placedateindex "
        "ON moz_historyvisits (place_id, visit_date)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<mozIStorageStatement> hasFrecencyStatement;
  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT frecency FROM moz_places"),
    getter_AddRefs(hasFrecencyStatement));

  if (NS_FAILED(rv)) {
    
    
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_places ADD frecency INTEGER DEFAULT -1 NOT NULL"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX IF NOT EXISTS moz_places_frecencyindex "
          "ON moz_places (frecency)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    

    
    
    rv = FixInvalidFrecenciesForExcludedPlaces();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<mozIStorageStatement> moveUnfiledBookmarks;
  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks "
      "SET parent = ("
        "SELECT folder_id "
        "FROM moz_bookmarks_roots "
        "WHERE root_name = ?1 "
      ") "
      "WHERE type = ?2 "
      "AND parent = ("
        "SELECT folder_id "
        "FROM moz_bookmarks_roots "
        "WHERE root_name = ?3 "
      ")"),
    getter_AddRefs(moveUnfiledBookmarks));
  rv = moveUnfiledBookmarks->BindUTF8StringParameter(0, NS_LITERAL_CSTRING("unfiled"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = moveUnfiledBookmarks->BindInt32Parameter(1, nsINavBookmarksService::TYPE_BOOKMARK);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = moveUnfiledBookmarks->BindUTF8StringParameter(2, NS_LITERAL_CSTRING("places"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = moveUnfiledBookmarks->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> triggerDetection;
  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT name "
      "FROM sqlite_master "
      "WHERE type = 'trigger' "
      "AND name = ?"),
    getter_AddRefs(triggerDetection));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool triggerExists;
  rv = triggerDetection->BindUTF8StringParameter(
    0, NS_LITERAL_CSTRING("moz_historyvisits_afterinsert_v1_trigger")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = triggerDetection->ExecuteStep(&triggerExists);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = triggerDetection->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  if (!triggerExists) {
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "UPDATE moz_places SET visit_count = "
          "(SELECT count(*) FROM moz_historyvisits "
           "WHERE place_id = moz_places.id "
            "AND visit_type NOT IN ") +
              nsPrintfCString("(0,%d,%d) ",
                              nsINavHistoryService::TRANSITION_EMBED,
                              nsINavHistoryService::TRANSITION_DOWNLOAD) +
          NS_LITERAL_CSTRING(")"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
  }

  
  rv = triggerDetection->BindUTF8StringParameter(
    0, NS_LITERAL_CSTRING("moz_bookmarks_beforedelete_v1_trigger")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = triggerDetection->ExecuteStep(&triggerExists);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = triggerDetection->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (!triggerExists) {
    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DELETE FROM moz_keywords "
        "WHERE id IN ("
          "SELECT k.id "
          "FROM moz_keywords k "
          "LEFT OUTER JOIN moz_bookmarks b "
          "ON b.keyword_id = k.id "
          "WHERE b.id IS NULL"
        ")"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_KEYWORD_VALIDITY_TRIGGER);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return transaction.Commit();
}

nsresult
nsNavHistory::MigrateV8Up(mozIStorageConnection *aDBConn)
{
  mozStorageTransaction transaction(aDBConn, PR_FALSE);

  nsresult rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TRIGGER IF EXISTS moz_historyvisits_afterinsert_v1_trigger"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TRIGGER IF EXISTS moz_historyvisits_afterdelete_v1_trigger"));
  NS_ENSURE_SUCCESS(rv, rv);


  
  rv = mDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_titleindex"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_annos_item_idindex"));
  NS_ENSURE_SUCCESS(rv, rv);


  
  PRBool oldIndexExists = PR_FALSE;
  rv = mDBConn->IndexExists(NS_LITERAL_CSTRING("moz_annos_attributesindex"), &oldIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (oldIndexExists) {
    
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX moz_annos_attributesindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE UNIQUE INDEX moz_annos_placeattributeindex ON moz_annos (place_id, anno_attribute_id)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_items_annos_attributesindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE UNIQUE INDEX moz_items_annos_itemattributeindex ON moz_items_annos (item_id, anno_attribute_id)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return transaction.Commit();
}












nsresult
nsNavHistory::GetUrlIdFor(nsIURI* aURI, PRInt64* aEntryID,
                          PRBool aAutoCreate)
{
  *aEntryID = 0;

  mozStorageStatementScoper statementResetter(mDBGetURLPageInfo);
  nsresult rv = BindStatementURI(mDBGetURLPageInfo, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasEntry = PR_FALSE;
  rv = mDBGetURLPageInfo->ExecuteStep(&hasEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasEntry) {
    return mDBGetURLPageInfo->GetInt64(kGetInfoIndex_PageID, aEntryID);
  } else if (aAutoCreate) {
    
    mDBGetURLPageInfo->Reset();
    statementResetter.Abandon();
    nsString voidString;
    voidString.SetIsVoid(PR_TRUE);
    return InternalAddNewPage(aURI, voidString, PR_TRUE, PR_FALSE, 0, PR_TRUE, aEntryID);
  } else {
    
    return NS_OK;
  }
}










nsresult
nsNavHistory::InternalAddNewPage(nsIURI* aURI,
                                 const nsAString& aTitle,
                                 PRBool aHidden,
                                 PRBool aTyped,
                                 PRInt32 aVisitCount,
                                 PRBool aCalculateFrecency,
                                 PRInt64* aPageID)
{
  mozStorageStatementScoper scoper(mDBAddNewPage);
  nsresult rv = BindStatementURI(mDBAddNewPage, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aTitle.IsVoid()) {
    
    nsAutoString title;
    GenerateTitleFromURI(aURI, title);
    rv = mDBAddNewPage->BindStringParameter(1,
        StringHead(title, HISTORY_TITLE_LENGTH_MAX));
  } else {
    rv = mDBAddNewPage->BindStringParameter(1,
        StringHead(aTitle, HISTORY_TITLE_LENGTH_MAX));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString revHost;
  rv = GetReversedHostname(aURI, revHost);
  
  if (NS_SUCCEEDED(rv)) {
    rv = mDBAddNewPage->BindStringParameter(2, revHost);
  } else {
    rv = mDBAddNewPage->BindNullParameter(2);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBAddNewPage->BindInt32Parameter(3, aHidden);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBAddNewPage->BindInt32Parameter(4, aTyped);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString url;
  rv = aURI->GetSpec(url);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 frecency = -1;
  if (aCalculateFrecency) {
    rv = CalculateFrecency(-1 ,
                           aTyped, aVisitCount, url, &frecency);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mDBAddNewPage->BindInt32Parameter(5, frecency);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAddNewPage->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aPageID) {
    mozStorageStatementScoper scoper(mDBGetURLPageInfo);

    rv = BindStatementURI(mDBGetURLPageInfo, 0, aURI);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult = PR_FALSE;
    rv = mDBGetURLPageInfo->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(hasResult, "hasResult is false but the call succeeded?");

    *aPageID = mDBGetURLPageInfo->AsInt64(0);
  }

  return NS_OK;
}





nsresult
nsNavHistory::InternalAddVisit(PRInt64 aPageID, PRInt64 aReferringVisit,
                               PRInt64 aSessionID, PRTime aTime,
                               PRInt32 aTransitionType, PRInt64* visitID)
{
  nsresult rv;

  {
    mozStorageStatementScoper scoper(mDBInsertVisit);
  
    rv = mDBInsertVisit->BindInt64Parameter(0, aReferringVisit);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBInsertVisit->BindInt64Parameter(1, aPageID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBInsertVisit->BindInt64Parameter(2, aTime);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBInsertVisit->BindInt32Parameter(3, aTransitionType);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBInsertVisit->BindInt64Parameter(4, aSessionID);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = mDBInsertVisit->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  {
    mozStorageStatementScoper scoper(mDBRecentVisitOfPlace);

    rv = mDBRecentVisitOfPlace->BindInt64Parameter(0, aPageID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBRecentVisitOfPlace->BindInt64Parameter(1, aTime);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBRecentVisitOfPlace->BindInt64Parameter(2, aSessionID);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = mDBRecentVisitOfPlace->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(hasResult, "hasResult is false but the call succeeded?");

    *visitID = mDBRecentVisitOfPlace->AsInt64(0);
  }

  return NS_OK;
}










PRBool
nsNavHistory::FindLastVisit(nsIURI* aURI, PRInt64* aVisitID,
                            PRInt64* aSessionID)
{
  mozStorageStatementScoper scoper(mDBRecentVisitOfURL);
  nsresult rv = BindStatementURI(mDBRecentVisitOfURL, 0, aURI);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool hasMore;
  rv = mDBRecentVisitOfURL->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  if (hasMore) {
    *aVisitID = mDBRecentVisitOfURL->AsInt64(0);
    *aSessionID = mDBRecentVisitOfURL->AsInt64(1);
    return PR_TRUE;
  }
  return PR_FALSE;
}








PRBool nsNavHistory::IsURIStringVisited(const nsACString& aURIString)
{
#ifdef LAZY_ADD
  
  for (PRUint32 i = 0; i < mLazyMessages.Length(); i ++) {
    if (mLazyMessages[i].type == LazyMessage::Type_AddURI) {
      if (aURIString.Equals(mLazyMessages[i].uriSpec))
        return PR_TRUE;
    }
  }
#endif

  
  mozStorageStatementScoper scoper(mDBIsPageVisited);
  nsresult rv = mDBIsPageVisited->BindUTF8StringParameter(0, aURIString);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool hasMore = PR_FALSE;
  rv = mDBIsPageVisited->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  return hasMore;
}




nsresult
nsNavHistory::LoadPrefs(PRBool aInitializing)
{
  if (! mPrefBranch)
    return NS_OK;

  mPrefBranch->GetIntPref(PREF_BROWSER_HISTORY_EXPIRE_DAYS_MAX, &mExpireDaysMax);
  mPrefBranch->GetIntPref(PREF_BROWSER_HISTORY_EXPIRE_DAYS_MIN, &mExpireDaysMin);
  
  
  if (mExpireDaysMax && mExpireDaysMax < mExpireDaysMin)
    mExpireDaysMax = mExpireDaysMin;
  if (NS_FAILED(mPrefBranch->GetIntPref(PREF_BROWSER_HISTORY_EXPIRE_SITES,
                                        &mExpireSites)))
    mExpireSites = EXPIRATION_CAP_SITES;
  
#ifdef MOZ_XUL
  mPrefBranch->GetBoolPref(PREF_AUTOCOMPLETE_ENABLED, &mAutoCompleteEnabled);

  PRInt32 matchBehavior = 1;
  mPrefBranch->GetIntPref(PREF_AUTOCOMPLETE_MATCH_BEHAVIOR,
                          &matchBehavior);
  switch (matchBehavior) {
    case 0:
      mAutoCompleteMatchBehavior = MATCH_ANYWHERE;
      break;
    case 2:
      mAutoCompleteMatchBehavior = MATCH_BOUNDARY;
      break;
    case 3:
      mAutoCompleteMatchBehavior = MATCH_BEGINNING;
      break;
    case 1:
    default:
      mAutoCompleteMatchBehavior = MATCH_BOUNDARY_ANYWHERE;
      break;
  }

  mPrefBranch->GetBoolPref(PREF_AUTOCOMPLETE_FILTER_JAVASCRIPT,
                           &mAutoCompleteFilterJavascript);
  mPrefBranch->GetIntPref(PREF_AUTOCOMPLETE_MAX_RICH_RESULTS,
                          &mAutoCompleteMaxResults);
  mPrefBranch->GetIntPref(PREF_AUTOCOMPLETE_SEARCH_CHUNK_SIZE,
                          &mAutoCompleteSearchChunkSize);
  mPrefBranch->GetIntPref(PREF_AUTOCOMPLETE_SEARCH_TIMEOUT,
                          &mAutoCompleteSearchTimeout);
  mPrefBranch->GetIntPref(PREF_AUTOCOMPLETE_DEFAULT_BEHAVIOR,
                          &mAutoCompleteDefaultBehavior);
  nsXPIDLCString prefStr;
  mPrefBranch->GetCharPref(PREF_AUTOCOMPLETE_RESTRICT_HISTORY,
                           getter_Copies(prefStr));
  CopyUTF8toUTF16(prefStr, mAutoCompleteRestrictHistory);
  mPrefBranch->GetCharPref(PREF_AUTOCOMPLETE_RESTRICT_BOOKMARK,
                           getter_Copies(prefStr));
  CopyUTF8toUTF16(prefStr, mAutoCompleteRestrictBookmark);
  mPrefBranch->GetCharPref(PREF_AUTOCOMPLETE_RESTRICT_TAG,
                           getter_Copies(prefStr));
  CopyUTF8toUTF16(prefStr, mAutoCompleteRestrictTag);
  mPrefBranch->GetCharPref(PREF_AUTOCOMPLETE_MATCH_TITLE,
                           getter_Copies(prefStr));
  CopyUTF8toUTF16(prefStr, mAutoCompleteMatchTitle);
  mPrefBranch->GetCharPref(PREF_AUTOCOMPLETE_MATCH_URL,
                           getter_Copies(prefStr));
  CopyUTF8toUTF16(prefStr, mAutoCompleteMatchUrl);
  mPrefBranch->GetCharPref(PREF_AUTOCOMPLETE_RESTRICT_TYPED,
                           getter_Copies(prefStr));
  CopyUTF8toUTF16(prefStr, mAutoCompleteRestrictTyped);

  
  mCurrentSearchString = EmptyString();
#endif

  
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService("@mozilla.org/preferences-service;1"));
  if (prefs) {
    prefs->GetIntPref(PREF_FRECENCY_NUM_VISITS, 
      &mNumVisitsForFrecency);
    prefs->GetIntPref(PREF_FRECENCY_CALC_ON_IDLE, 
      &mNumCalculateFrecencyOnIdle);
    prefs->GetIntPref(PREF_FRECENCY_CALC_ON_MIGRATE, 
      &mNumCalculateFrecencyOnMigrate);
    prefs->GetIntPref(PREF_FRECENCY_UPDATE_IDLE_TIME, 
      &mFrecencyUpdateIdleTime);
    prefs->GetIntPref(PREF_FRECENCY_FIRST_BUCKET_CUTOFF, 
      &mFirstBucketCutoffInDays);
    prefs->GetIntPref(PREF_FRECENCY_SECOND_BUCKET_CUTOFF,
      &mSecondBucketCutoffInDays);
    prefs->GetIntPref(PREF_FRECENCY_THIRD_BUCKET_CUTOFF, 
      &mThirdBucketCutoffInDays);
    prefs->GetIntPref(PREF_FRECENCY_FOURTH_BUCKET_CUTOFF, 
      &mFourthBucketCutoffInDays);
    prefs->GetIntPref(PREF_FRECENCY_EMBED_VISIT_BONUS, 
      &mEmbedVisitBonus);
    prefs->GetIntPref(PREF_FRECENCY_LINK_VISIT_BONUS, 
      &mLinkVisitBonus);
    prefs->GetIntPref(PREF_FRECENCY_TYPED_VISIT_BONUS, 
      &mTypedVisitBonus);
    prefs->GetIntPref(PREF_FRECENCY_BOOKMARK_VISIT_BONUS, 
      &mBookmarkVisitBonus);
    prefs->GetIntPref(PREF_FRECENCY_DOWNLOAD_VISIT_BONUS, 
      &mDownloadVisitBonus);
    prefs->GetIntPref(PREF_FRECENCY_PERM_REDIRECT_VISIT_BONUS, 
      &mPermRedirectVisitBonus);
    prefs->GetIntPref(PREF_FRECENCY_TEMP_REDIRECT_VISIT_BONUS, 
      &mTempRedirectVisitBonus);
    prefs->GetIntPref(PREF_FRECENCY_DEFAULT_VISIT_BONUS, 
      &mDefaultVisitBonus);
    prefs->GetIntPref(PREF_FRECENCY_UNVISITED_BOOKMARK_BONUS, 
      &mUnvisitedBookmarkBonus);
    prefs->GetIntPref(PREF_FRECENCY_UNVISITED_TYPED_BONUS,
      &mUnvisitedTypedBonus);
    prefs->GetIntPref(PREF_FRECENCY_FIRST_BUCKET_WEIGHT, 
      &mFirstBucketWeight);
    prefs->GetIntPref(PREF_FRECENCY_SECOND_BUCKET_WEIGHT, 
      &mSecondBucketWeight);
    prefs->GetIntPref(PREF_FRECENCY_THIRD_BUCKET_WEIGHT, 
      &mThirdBucketWeight);
    prefs->GetIntPref(PREF_FRECENCY_FOURTH_BUCKET_WEIGHT, 
      &mFourthBucketWeight);
    prefs->GetIntPref(PREF_FRECENCY_DEFAULT_BUCKET_WEIGHT, 
      &mDefaultWeight);
  }
  return NS_OK;
}








PRTime
nsNavHistory::GetNow()
{
  if (!mNowValid) {
    mLastNow = PR_Now();
    mNowValid = PR_TRUE;
    if (!mExpireNowTimer)
      mExpireNowTimer = do_CreateInstance("@mozilla.org/timer;1");

    if (mExpireNowTimer)
      mExpireNowTimer->InitWithFuncCallback(expireNowTimerCallback, this,
                                            HISTORY_EXPIRE_NOW_TIMEOUT,
                                            nsITimer::TYPE_ONE_SHOT);
  }

  return mLastNow;
}




void nsNavHistory::expireNowTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistory* history = static_cast<nsNavHistory*>(aClosure);
  history->mNowValid = PR_FALSE;
  history->mExpireNowTimer = nsnull;
}

static PRTime
NormalizeTimeRelativeToday(PRTime aTime)
{
  
  PRExplodedTime explodedTime;
  PR_ExplodeTime(aTime, PR_LocalTimeParameters, &explodedTime);

  
  explodedTime.tm_min =
    explodedTime.tm_hour =
    explodedTime.tm_sec =
    explodedTime.tm_usec = 0;

  return PR_ImplodeTime(&explodedTime);
}











PRTime 
nsNavHistory::NormalizeTime(PRUint32 aRelative, PRTime aOffset)
{
  PRTime ref;
  switch (aRelative)
  {
    case nsINavHistoryQuery::TIME_RELATIVE_EPOCH:
      return aOffset;
    case nsINavHistoryQuery::TIME_RELATIVE_TODAY:
      ref = NormalizeTimeRelativeToday(PR_Now());
      break;
    case nsINavHistoryQuery::TIME_RELATIVE_NOW:
      ref = PR_Now();
      break;
    default:
      NS_NOTREACHED("Invalid relative time");
      return 0;
  }
  return ref + aOffset;
}























PRUint32
nsNavHistory::GetUpdateRequirements(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                    nsNavHistoryQueryOptions* aOptions,
                                    PRBool* aHasSearchTerms)
{
  NS_ASSERTION(aQueries.Count() > 0, "Must have at least one query");

  
  *aHasSearchTerms = PR_FALSE;
  PRInt32 i;
  for (i = 0; i < aQueries.Count(); i ++) {
    aQueries[i]->GetHasSearchTerms(aHasSearchTerms);
    if (*aHasSearchTerms)
      break;
  }

  PRBool nonTimeBasedItems = PR_FALSE;
  PRBool domainBasedItems = PR_FALSE;

  for (i = 0; i < aQueries.Count(); i ++) {
    nsNavHistoryQuery* query = aQueries[i];

    if (query->Folders().Length() > 0 || query->OnlyBookmarked()) {
      return QUERYUPDATE_COMPLEX_WITH_BOOKMARKS;
    }
    
    
    if (! query->SearchTerms().IsEmpty() ||
        ! query->Domain().IsVoid() ||
        query->Uri() != nsnull)
      nonTimeBasedItems = PR_TRUE;

    if (! query->Domain().IsVoid())
      domainBasedItems = PR_TRUE;
  }

  if (aOptions->ResultType() ==
      nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY)
    return QUERYUPDATE_COMPLEX_WITH_BOOKMARKS;

  
  
  
  
  if (aOptions->MaxResults() > 0)
    return QUERYUPDATE_COMPLEX;

  if (aQueries.Count() == 1 && domainBasedItems)
    return QUERYUPDATE_HOST;
  if (aQueries.Count() == 1 && ! nonTimeBasedItems)
    return QUERYUPDATE_TIME;
  return QUERYUPDATE_SIMPLE;
}
















PRBool
nsNavHistory::EvaluateQueryForNode(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                   nsNavHistoryQueryOptions* aOptions,
                                   nsNavHistoryResultNode* aNode)
{
  
  nsCOMPtr<nsIURI> nodeUri;

  for (PRInt32 i = 0; i < aQueries.Count(); i ++) {
    PRBool hasIt;
    nsCOMPtr<nsNavHistoryQuery> query = aQueries[i];

    
    query->GetHasBeginTime(&hasIt);
    if (hasIt) {
      PRTime beginTime = NormalizeTime(query->BeginTimeReference(),
                                       query->BeginTime());
      if (aNode->mTime < beginTime)
        continue; 
    }

    
    query->GetHasEndTime(&hasIt);
    if (hasIt) {
      PRTime endTime = NormalizeTime(query->EndTimeReference(),
                                     query->EndTime());
      if (aNode->mTime > endTime)
        continue; 
    }

    
    if (! query->SearchTerms().IsEmpty()) {
      
      
      nsCOMArray<nsNavHistoryResultNode> inputSet;
      inputSet.AppendObject(aNode);
      nsCOMArray<nsNavHistoryQuery> queries;
      queries.AppendObject(query);
      nsCOMArray<nsNavHistoryResultNode> filteredSet;
      nsresult rv = FilterResultSet(nsnull, inputSet, &filteredSet, queries, aOptions);
      if (NS_FAILED(rv))
        continue;
      if (! filteredSet.Count())
        continue; 
    }

    
    query->GetHasDomain(&hasIt);
    if (hasIt) {
      if (! nodeUri) {
        
        if (NS_FAILED(NS_NewURI(getter_AddRefs(nodeUri), aNode->mURI)))
          continue;
      }
      nsCAutoString asciiRequest;
      if (NS_FAILED(AsciiHostNameFromHostString(query->Domain(), asciiRequest)))
        continue;

      if (query->DomainIsHost()) {
        nsCAutoString host;
        if (NS_FAILED(nodeUri->GetAsciiHost(host)))
          continue;

        if (! asciiRequest.Equals(host))
          continue; 
      }
      
      nsCAutoString domain;
      DomainNameFromURI(nodeUri, domain);
      if (! asciiRequest.Equals(domain))
        continue; 
    }

    
    if (query->Uri()) {
      if (! nodeUri) { 
        if (NS_FAILED(NS_NewURI(getter_AddRefs(nodeUri), aNode->mURI)))
          continue;
      }
      if (! query->UriIsPrefix()) {
        
        PRBool equals;
        nsresult rv = query->Uri()->Equals(nodeUri, &equals);
        NS_ENSURE_SUCCESS(rv, PR_FALSE);
        if (! equals)
          continue;
      } else {
        
        
        
        nsCAutoString nodeUriString;
        nodeUri->GetAsciiSpec(nodeUriString);
        nsCAutoString queryUriString;
        query->Uri()->GetAsciiSpec(queryUriString);
        if (queryUriString.Length() > nodeUriString.Length())
          continue; 
        nodeUriString.SetLength(queryUriString.Length());
        if (! nodeUriString.Equals(queryUriString))
          continue; 
      }
    }

    
    
    
    return PR_TRUE;
  }

  
  return PR_FALSE;
}









nsresult 
nsNavHistory::AsciiHostNameFromHostString(const nsACString& aHostName,
                                          nsACString& aAscii)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aHostName);
  NS_ENSURE_SUCCESS(rv, rv);
  return uri->GetAsciiHost(aAscii);
}







void
nsNavHistory::DomainNameFromURI(nsIURI *aURI,
                                nsACString& aDomainName)
{
  
  if (!mTLDService)
    mTLDService = do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID);

  if (mTLDService) {
    
    
    nsresult rv = mTLDService->GetBaseDomain(aURI, 0, aDomainName);
    if (NS_SUCCEEDED(rv))
      return;
  }

  
  
  aURI->GetAsciiHost(aDomainName);
}







NS_IMETHODIMP
nsNavHistory::GetHasHistoryEntries(PRBool* aHasEntries)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsCOMPtr<mozIStorageStatement> dbSelectStatement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT 1 "
      "WHERE EXISTS (SELECT id FROM moz_historyvisits_temp LIMIT 1) "
        "OR EXISTS (SELECT id FROM moz_historyvisits LIMIT 1)"),
    getter_AddRefs(dbSelectStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  return dbSelectStatement->ExecuteStep(aHasEntries);
}

nsresult
nsNavHistory::FixInvalidFrecenciesForExcludedPlaces()
{
  
  
  
  nsCOMPtr<mozIStorageStatement> dbUpdateStatement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places_view "
      "SET frecency = 0 WHERE id IN ("
        "SELECT h.id FROM moz_places h "
        "WHERE h.url >= 'place:' AND h.url < 'place;' "
        "UNION "
        "SELECT h.id FROM moz_places_temp h "
        "WHERE  h.url >= 'place:' AND h.url < 'place;' "
        "UNION "
        
        "SELECT b.fk FROM moz_bookmarks b "
        "JOIN moz_bookmarks bp ON bp.id = b.parent "
        "JOIN moz_items_annos a ON a.item_id = bp.id "
        "JOIN moz_anno_attributes n ON n.id = a.anno_attribute_id "
        "WHERE n.name = ?1 "
        "AND b.fk IN( "
          "SELECT id FROM moz_places WHERE visit_count = 0 AND frecency < 0 "
          "UNION ALL "
          "SELECT id FROM moz_places_temp WHERE visit_count = 0 AND frecency < 0 "
        ") "
      ")"),
    getter_AddRefs(dbUpdateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbUpdateStatement->BindUTF8StringParameter(0, NS_LITERAL_CSTRING(LMANNO_FEEDURI));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbUpdateStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::CalculateFullVisitCount(PRInt64 aPlaceId, PRInt32 *aVisitCount)
{
  mozStorageStatementScoper scope(mDBFullVisitCount);

  nsresult rv = mDBFullVisitCount->BindInt64Parameter(0, aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasVisits = PR_TRUE;
  rv = mDBFullVisitCount->ExecuteStep(&hasVisits);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasVisits) {
    rv = mDBFullVisitCount->GetInt32(0, aVisitCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
    *aVisitCount = 0;
  
  return NS_OK;
}











NS_IMETHODIMP
nsNavHistory::MarkPageAsFollowedBookmark(nsIURI* aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  if (IsHistoryDisabled())
    return NS_OK;

  nsCAutoString uriString;
  nsresult rv = aURI->GetSpec(uriString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 unusedEventTime;
  if (mRecentBookmark.Get(uriString, &unusedEventTime))
    mRecentBookmark.Remove(uriString);

  if (mRecentBookmark.Count() > RECENT_EVENT_QUEUE_MAX_LENGTH)
    ExpireNonrecentEvents(&mRecentBookmark);

  mRecentBookmark.Put(uriString, GetNow());
  return NS_OK;
}










NS_IMETHODIMP
nsNavHistory::CanAddURI(nsIURI* aURI, PRBool* canAdd)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  NS_ENSURE_ARG_POINTER(aURI);

  
  if (InPrivateBrowsingMode()) {
    *canAdd = PR_FALSE;
    return NS_OK;
  }

  nsCAutoString scheme;
  nsresult rv = aURI->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (scheme.EqualsLiteral("http")) {
    *canAdd = PR_TRUE;
    return NS_OK;
  }
  if (scheme.EqualsLiteral("https")) {
    *canAdd = PR_TRUE;
    return NS_OK;
  }

  
  if (scheme.EqualsLiteral("about") ||
      scheme.EqualsLiteral("imap") ||
      scheme.EqualsLiteral("news") ||
      scheme.EqualsLiteral("mailbox") ||
      scheme.EqualsLiteral("moz-anno") ||
      scheme.EqualsLiteral("view-source") ||
      scheme.EqualsLiteral("chrome") ||
      scheme.EqualsLiteral("data") ||
      scheme.EqualsLiteral("wyciwyg")) {
    *canAdd = PR_FALSE;
    return NS_OK;
  }
  *canAdd = PR_TRUE;
  return NS_OK;
}









NS_IMETHODIMP
nsNavHistory::AddVisit(nsIURI* aURI, PRTime aTime, nsIURI* aReferringURI,
                       PRInt32 aTransitionType, PRBool aIsRedirect,
                       PRInt64 aSessionID, PRInt64* aVisitID)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  PRBool canAdd = PR_FALSE;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    *aVisitID = 0;
    return NS_OK;
  }

  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  mozStorageStatementScoper scoper(mDBGetPageVisitStats);
  rv = BindStatementURI(mDBGetPageVisitStats, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool alreadyVisited = PR_FALSE;
  rv = mDBGetPageVisitStats->ExecuteStep(&alreadyVisited);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 pageID = 0;
  PRInt32 hidden;
  PRInt32 typed;
  PRBool newItem = PR_FALSE; 
  if (alreadyVisited) {
    
    rv = mDBGetPageVisitStats->GetInt64(0, &pageID);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 oldVisitCount = 0;
    rv = mDBGetPageVisitStats->GetInt32(1, &oldVisitCount);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 oldTypedState = 0;
    rv = mDBGetPageVisitStats->GetInt32(2, &oldTypedState);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool oldHiddenState = 0;
    rv = mDBGetPageVisitStats->GetInt32(3, &oldHiddenState);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mDBGetPageVisitStats->Reset();
    scoper.Abandon();

    
    
    
    
    
    
    
    
    
    
    
    
    hidden = oldHiddenState;
    if (hidden == 1 && (!aIsRedirect || aTransitionType == TRANSITION_TYPED) &&
        aTransitionType != TRANSITION_EMBED)
      hidden = 0; 

    typed = (PRInt32)(oldTypedState == 1 || (aTransitionType == TRANSITION_TYPED));

    
    
    if (oldVisitCount == 0)
      newItem = PR_TRUE;

    
    mozStorageStatementScoper updateScoper(mDBUpdatePageVisitStats);
    rv = mDBUpdatePageVisitStats->BindInt64Parameter(0, pageID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBUpdatePageVisitStats->BindInt32Parameter(1, hidden);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBUpdatePageVisitStats->BindInt32Parameter(2, typed);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBUpdatePageVisitStats->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    newItem = PR_TRUE;

    
    mDBGetPageVisitStats->Reset();
    scoper.Abandon();

    
    
    hidden = (PRInt32)(aTransitionType == TRANSITION_EMBED || aIsRedirect);

    typed = (PRInt32)(aTransitionType == TRANSITION_TYPED);

    
    nsString voidString;
    voidString.SetIsVoid(PR_TRUE);
    rv = InternalAddNewPage(aURI, voidString, hidden == 1, typed == 1, 1,
                            PR_TRUE, &pageID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRInt64 referringVisitID = 0;
  PRInt64 referringSessionID;
  if (aReferringURI &&
      !FindLastVisit(aReferringURI, &referringVisitID, &referringSessionID)) {
    
    rv = AddVisit(aReferringURI, aTime - 1, nsnull, TRANSITION_LINK, PR_FALSE,
                  aSessionID, &referringVisitID);
    if (NS_FAILED(rv))
      referringVisitID = 0;
  }

  rv = InternalAddVisit(pageID, referringVisitID, aSessionID, aTime,
                        aTransitionType, aVisitID);
  transaction.Commit();

  
  
  
  nsNavBookmarks *bs = nsNavBookmarks::GetBookmarksService();
  (void)UpdateFrecency(pageID, bs->IsRealBookmark(pageID));

  
  
  
  PRUint32 added = 0;
  if (!hidden && aTransitionType != TRANSITION_EMBED &&
                 aTransitionType != TRANSITION_DOWNLOAD) {
    ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                        OnVisit(aURI, *aVisitID, aTime, aSessionID,
                                referringVisitID, aTransitionType, &added));
  }

  
  
  
  
  
  if (newItem && (aIsRedirect || aTransitionType == TRANSITION_DOWNLOAD)) {
    nsCOMPtr<nsIObserverService> obsService =
      do_GetService("@mozilla.org/observer-service;1");
    if (obsService)
      obsService->NotifyObservers(aURI, NS_LINK_VISITED_EVENT_TOPIC, nsnull);
  }

  return NS_OK;
}




NS_IMETHODIMP nsNavHistory::GetNewQuery(nsINavHistoryQuery **_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  *_retval = new nsNavHistoryQuery();
  if (! *_retval)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval);
  return NS_OK;
}



NS_IMETHODIMP nsNavHistory::GetNewQueryOptions(nsINavHistoryQueryOptions **_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  *_retval = new nsNavHistoryQueryOptions();
  NS_ENSURE_TRUE(*_retval, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::ExecuteQuery(nsINavHistoryQuery *aQuery, nsINavHistoryQueryOptions *aOptions,
                           nsINavHistoryResult** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  return ExecuteQueries(&aQuery, 1, aOptions, _retval);
}














NS_IMETHODIMP
nsNavHistory::ExecuteQueries(nsINavHistoryQuery** aQueries, PRUint32 aQueryCount,
                             nsINavHistoryQueryOptions *aOptions,
                             nsINavHistoryResult** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv;
  NS_ENSURE_ARG_POINTER(aQueries);
  NS_ENSURE_ARG_POINTER(aOptions);
  if (! aQueryCount)
    return NS_ERROR_INVALID_ARG;

  
  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions);
  NS_ENSURE_TRUE(options, NS_ERROR_INVALID_ARG);

  
  nsCOMArray<nsNavHistoryQuery> queries;
  for (PRUint32 i = 0; i < aQueryCount; i ++) {
    nsCOMPtr<nsNavHistoryQuery> query = do_QueryInterface(aQueries[i], &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    queries.AppendObject(query);
  }

  
  nsRefPtr<nsNavHistoryContainerResultNode> rootNode;
  PRInt64 folderId = GetSimpleBookmarksQueryFolder(queries, options);
  if (folderId) {
    
    
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
    nsRefPtr<nsNavHistoryResultNode> tempRootNode;
    rv = bookmarks->ResultNodeForContainer(folderId, options,
                                           getter_AddRefs(tempRootNode));
    NS_ENSURE_SUCCESS(rv, rv);
    rootNode = tempRootNode->GetAsContainer();
  } else {
    
    rootNode = new nsNavHistoryQueryResultNode(EmptyCString(), EmptyCString(),
                                               queries, options);
    NS_ENSURE_TRUE(rootNode, NS_ERROR_OUT_OF_MEMORY);
  }

  
  nsRefPtr<nsNavHistoryResult> result;
  rv = nsNavHistoryResult::NewHistoryResult(aQueries, aQueryCount, options, rootNode,
                                            getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = result);
  return NS_OK;
}









static
PRBool IsHistoryMenuQuery(const nsCOMArray<nsNavHistoryQuery>& aQueries, nsNavHistoryQueryOptions *aOptions, PRUint16 aSortMode)
{
  if (aQueries.Count() != 1)
    return PR_FALSE;

  nsNavHistoryQuery *aQuery = aQueries[0];
 
  if (aOptions->QueryType() != nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY)
    return PR_FALSE;

  if (aOptions->ResultType() != nsINavHistoryQueryOptions::RESULTS_AS_URI)
    return PR_FALSE;

  if (aOptions->SortingMode() != aSortMode)
    return PR_FALSE;

  if (aOptions->MaxResults() <= 0)
    return PR_FALSE;

  if (aOptions->ExcludeItems())
    return PR_FALSE;

  if (aOptions->IncludeHidden())
    return PR_FALSE;

  if (aQuery->MinVisits() != -1 || aQuery->MaxVisits() != -1)
    return PR_FALSE;

  if (aQuery->BeginTime() || aQuery->BeginTimeReference()) 
    return PR_FALSE;

  if (aQuery->EndTime() || aQuery->EndTimeReference()) 
    return PR_FALSE;

  if (!aQuery->SearchTerms().IsEmpty()) 
    return PR_FALSE;

  if (aQuery->OnlyBookmarked()) 
    return PR_FALSE;

  if (aQuery->DomainIsHost() || !aQuery->Domain().IsEmpty())
    return PR_FALSE;

  if (aQuery->AnnotationIsNot() || !aQuery->Annotation().IsEmpty()) 
    return PR_FALSE;

  if (aQuery->UriIsPrefix() || aQuery->Uri()) 
    return PR_FALSE;

  if (aQuery->Folders().Length() > 0)
    return PR_FALSE;

  return PR_TRUE;
}

static
PRBool NeedToFilterResultSet(const nsCOMArray<nsNavHistoryQuery>& aQueries, 
                             nsNavHistoryQueryOptions *aOptions)
{
  
  
  if (aOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS &&
      aOptions->ResultType() != nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY)
    return PR_TRUE;

  nsCString parentAnnotationToExclude;
  nsresult rv = aOptions->GetExcludeItemIfParentHasAnnotation(parentAnnotationToExclude);
  NS_ENSURE_SUCCESS(rv, PR_TRUE);
  if (!parentAnnotationToExclude.IsEmpty())
    return PR_TRUE;

  PRInt32 i;
  for (i = 0; i < aQueries.Count(); i ++) {
    if (aQueries[i]->Folders().Length() != 0) {
      return PR_TRUE;
    } else {
      PRBool hasSearchTerms;
      nsresult rv = aQueries[i]->GetHasSearchTerms(&hasSearchTerms);
      if (NS_FAILED(rv) || hasSearchTerms)
        return PR_TRUE;
    }
  }
  return PR_FALSE;
}



class PlacesSQLQueryBuilder
{
public:
  PlacesSQLQueryBuilder(const nsCString& aConditions,
                        nsNavHistoryQueryOptions* aOptions,
                        PRBool aUseLimit,
                        nsNavHistory::StringHash& aAddParams);

  nsresult GetQueryString(nsCString& aQueryString);

private:
  nsresult Select();

  nsresult SelectAsURI();
  nsresult SelectAsVisit();
  nsresult SelectAsDay();
  nsresult SelectAsSite();
  nsresult SelectAsTag();

  nsresult Where();
  nsresult GroupBy();
  nsresult OrderBy();
  nsresult Limit();

  void OrderByColumnIndexAsc(PRInt32 aIndex);
  void OrderByColumnIndexDesc(PRInt32 aIndex);

  const nsCString& mConditions;
  PRBool mUseLimit;

  PRUint16 mResultType;
  PRUint16 mQueryType;
  PRBool mIncludeHidden;
  PRUint16 mRedirectsMode;
  PRUint16 mSortingMode;
  PRUint32 mMaxResults;

  nsCString mQueryString;
  nsCString mGroupBy;
  PRBool mHasDateColumns;
  PRBool mSkipOrderBy;
  nsNavHistory::StringHash& mAddParams;
};

PlacesSQLQueryBuilder::PlacesSQLQueryBuilder(
    const nsCString& aConditions, 
    nsNavHistoryQueryOptions* aOptions, 
    PRBool aUseLimit,
    nsNavHistory::StringHash& aAddParams) :
  mConditions(aConditions),
  mUseLimit(aUseLimit),
  mResultType(aOptions->ResultType()),
  mQueryType(aOptions->QueryType()),
  mIncludeHidden(aOptions->IncludeHidden()),
  mRedirectsMode(aOptions->RedirectsMode()),
  mSortingMode(aOptions->SortingMode()),
  mMaxResults(aOptions->MaxResults()),
  mSkipOrderBy(PR_FALSE),
  mAddParams(aAddParams)
{
  mHasDateColumns = (mQueryType == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS);
}

nsresult
PlacesSQLQueryBuilder::GetQueryString(nsCString& aQueryString)
{
  nsresult rv = Select();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = Where();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = GroupBy();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = OrderBy();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = Limit();
  NS_ENSURE_SUCCESS(rv, rv);

  aQueryString = mQueryString;
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::Select()
{
  nsresult rv;

  switch (mResultType)
  {
    case nsINavHistoryQueryOptions::RESULTS_AS_URI:
    case nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS:
      rv = SelectAsURI();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsINavHistoryQueryOptions::RESULTS_AS_VISIT:
    case nsINavHistoryQueryOptions::RESULTS_AS_FULL_VISIT:
      rv = SelectAsVisit();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY:
    case nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY:
      rv = SelectAsDay();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY:
      rv = SelectAsSite();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY:
      rv = SelectAsTag();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    default:
      NS_NOTREACHED("Invalid result type");
  }
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsURI()
{
  switch (mQueryType)
  {
    case nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY:
      if (!mIncludeHidden) {
        mQueryString = NS_LITERAL_CSTRING(
          "SELECT id, url, title, rev_host, visit_count, MAX(visit_date), "
            "favicon_url, session, empty "
          "FROM ( "
            "SELECT id, url, title, rev_host, visit_count, visit_date, "
              "favicon_url, session, empty "
            "FROM ("
              "SELECT h.id AS id, h.url AS url, h.title AS title, "
                "h.rev_host AS rev_host, h.visit_count AS visit_count, "
                "MAX(v.visit_date) AS visit_date, f.url AS favicon_url, "
                "v.session AS session, null AS empty "
              "FROM moz_places h "
              "JOIN moz_historyvisits v ON h.id = v.place_id "
              "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
              "WHERE h.hidden <> 1 AND v.visit_type NOT IN ") +
                nsPrintfCString("(0,%d) ",
                                nsINavHistoryService::TRANSITION_EMBED) +
                NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
                "{ADDITIONAL_CONDITIONS} "
              "GROUP BY h.id "
            ") "
            "UNION ALL "
            "SELECT id, url, title, rev_host, visit_count, visit_date, "
              "favicon_url, session, empty "
            "FROM ( "
              "SELECT h.id AS id, h.url AS url, h.title AS title, "
                "h.rev_host AS rev_host, h.visit_count AS visit_count, "
                "MAX(v.visit_date) AS visit_date, f.url AS favicon_url, "
                "v.session AS session, null AS empty "
              "FROM moz_places_temp h "
              "JOIN moz_historyvisits v ON h.id = v.place_id "
              "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
              "WHERE h.hidden <> 1 AND v.visit_type NOT IN ") +
                nsPrintfCString("(0,%d) ",
                                nsINavHistoryService::TRANSITION_EMBED) +
                NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
                "{ADDITIONAL_CONDITIONS} "
                "AND h.id NOT IN (SELECT id FROM moz_places_temp) "
              "GROUP BY h.id "
            ") "
            "UNION ALL "
            "SELECT id, url, title, rev_host, visit_count, visit_date, "
              "favicon_url, session, empty "
            "FROM ( "
              "SELECT h.id AS id, h.url AS url, h.title AS title, "
                "h.rev_host AS rev_host, h.visit_count AS visit_count, "
                "MAX(v.visit_date) AS visit_date, f.url AS favicon_url, "
                "v.session AS session, null AS empty "
              "FROM moz_places h "
              "JOIN moz_historyvisits_temp v ON h.id = v.place_id "
              "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
              "WHERE h.hidden <> 1 AND v.visit_type NOT IN ") +
                nsPrintfCString("(0,%d) ",
                                nsINavHistoryService::TRANSITION_EMBED) +
                NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
                "{ADDITIONAL_CONDITIONS} "
                "AND h.id NOT IN (SELECT id FROM moz_places_temp) "
              "GROUP BY h.id "
            ") "
            "UNION ALL "
            "SELECT id, url, title, rev_host, visit_count, visit_date, "
              "favicon_url, session, empty "
            "FROM ( "
              "SELECT h.id AS id, h.url AS url, h.title AS title, "
                "h.rev_host AS rev_host, h.visit_count AS visit_count, "
                "MAX(v.visit_date) AS visit_date, f.url AS favicon_url, "
                "v.session AS session, null AS empty "
              "FROM moz_places_temp h "
              "JOIN moz_historyvisits_temp v ON h.id = v.place_id "
              "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
              "WHERE h.hidden <> 1 AND v.visit_type NOT IN ") +
                nsPrintfCString("(0,%d) ",
                                nsINavHistoryService::TRANSITION_EMBED) +
                NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
                "{ADDITIONAL_CONDITIONS} "
              "GROUP BY h.id "
            ") "
          ") "
          "GROUP BY id ");
      }
      else {
        mQueryString = NS_LITERAL_CSTRING(
          "SELECT id, url, title, rev_host, visit_count, MAX(visit_date), "
            "favicon_url, session, empty "
          "FROM ( "
            "SELECT id, url, title, rev_host, visit_count, visit_date, "
              "favicon_url, session, empty "
            "FROM ("
              "SELECT h.id AS id, h.url AS url, h.title AS title, "
                "h.rev_host AS rev_host, h.visit_count AS visit_count, "
                "MAX(v.visit_date) AS visit_date, f.url AS favicon_url, "
                "v.session AS session, null AS empty "
              "FROM moz_places h "
              "JOIN moz_historyvisits v ON h.id = v.place_id "
              "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
              
              "WHERE 1=1 "
                "{QUERY_OPTIONS} "
                "{ADDITIONAL_CONDITIONS} "
              "GROUP BY h.id "
            ") "
            "UNION ALL "
            "SELECT id, url, title, rev_host, visit_count, visit_date, "
              "favicon_url, session, empty "
            "FROM ( "
              "SELECT h.id AS id, h.url AS url, h.title AS title, "
                "h.rev_host AS rev_host, h.visit_count AS visit_count, "
                "MAX(v.visit_date) AS visit_date, f.url AS favicon_url, "
                "v.session AS session, null AS empty "
              "FROM moz_places_temp h "
              "JOIN moz_historyvisits v ON h.id = v.place_id "
              "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
              "WHERE h.id NOT IN (SELECT id FROM moz_places_temp) "
                "{QUERY_OPTIONS} "
                "{ADDITIONAL_CONDITIONS} "
              "GROUP BY h.id "
            ") "
            "UNION ALL "
            "SELECT id, url, title, rev_host, visit_count, visit_date, "
              "favicon_url, session, empty "
            "FROM ( "
              "SELECT h.id AS id, h.url AS url, h.title AS title, "
                "h.rev_host AS rev_host, h.visit_count AS visit_count, "
                "MAX(v.visit_date) AS visit_date, f.url AS favicon_url, "
                "v.session AS session, null AS empty "
              "FROM moz_places h "
              "JOIN moz_historyvisits_temp v ON h.id = v.place_id "
              "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
              "WHERE h.id NOT IN (SELECT id FROM moz_places_temp) "
                "{QUERY_OPTIONS} "
                "{ADDITIONAL_CONDITIONS} "
              "GROUP BY h.id "
            ") "
            "UNION ALL "
            "SELECT id, url, title, rev_host, visit_count, visit_date, "
              "favicon_url, session, empty "
            "FROM ( "
              "SELECT h.id AS id, h.url AS url, h.title AS title, "
                "h.rev_host AS rev_host, h.visit_count AS visit_count, "
                "MAX(v.visit_date) AS visit_date, f.url AS favicon_url, "
                "v.session AS session, null AS empty "
              "FROM moz_places_temp h "
              "JOIN moz_historyvisits_temp v ON h.id = v.place_id "
              "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
              
              "WHERE 1=1 "
                "{QUERY_OPTIONS} "
                "{ADDITIONAL_CONDITIONS} "
              "GROUP BY h.id "
            ") "
          ") "
          "GROUP BY id ");    
      }
      break;

    case nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS:
      
      
      
      nsNavHistory* history;
      history = nsNavHistory::GetHistoryService();
      NS_ENSURE_STATE(history);

      if (mResultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS) {
        
        
        
        
        mSkipOrderBy = PR_TRUE;

        mQueryString = NS_LITERAL_CSTRING(
          "SELECT b2.fk, h.url, COALESCE(b2.title, h.title), h.rev_host, "
            "h.visit_count, "
            SQL_STR_FRAGMENT_MAX_VISIT_DATE( "b2.fk" )
            ", f.url, null, b2.id, b2.dateAdded, b2.lastModified "
          "FROM moz_bookmarks b2 "
          "JOIN moz_places_temp h ON b2.fk = h.id AND b2.type = 1 "
          "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
          "WHERE b2.id IN ( "
            "SELECT b1.id FROM moz_bookmarks b1 "
            "WHERE b1.fk IN "
              "(SELECT b.fk FROM moz_bookmarks b WHERE b.type = 1 {ADDITIONAL_CONDITIONS}) "
            "AND NOT EXISTS ( "
              "SELECT id FROM moz_bookmarks WHERE id = b1.parent AND parent = ") +
                nsPrintfCString("%lld", history->GetTagsFolder()) +
              NS_LITERAL_CSTRING(") "
          ") "
          "UNION ALL "
          "SELECT b2.fk, h.url, COALESCE(b2.title, h.title), h.rev_host, "
            "h.visit_count, "
            SQL_STR_FRAGMENT_MAX_VISIT_DATE( "b2.fk" )
            ", f.url, null, b2.id, b2.dateAdded, b2.lastModified "
          "FROM moz_bookmarks b2 "
          "JOIN moz_places h ON b2.fk = h.id AND b2.type = 1 "
          "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
          "WHERE b2.id IN ( "
            "SELECT b1.id FROM moz_bookmarks b1 "
            "WHERE h.id NOT IN (SELECT id FROM moz_places_temp) "
            "AND b1.fk IN "
              "(SELECT b.fk FROM moz_bookmarks b WHERE b.type = 1 {ADDITIONAL_CONDITIONS}) "
            "AND NOT EXISTS ( "
              "SELECT id FROM moz_bookmarks WHERE id = b1.parent AND parent = ") +
                nsPrintfCString("%lld", history->GetTagsFolder()) +
              NS_LITERAL_CSTRING(") "
          ") "          
          "ORDER BY b2.fk DESC, b2.lastModified DESC");
      }
      else {
        mQueryString = NS_LITERAL_CSTRING(
          "SELECT b.fk, h.url, COALESCE(b.title, h.title), h.rev_host, "
            "h.visit_count,"
            SQL_STR_FRAGMENT_MAX_VISIT_DATE( "b.fk" )
            ", f.url, null, b.id, b.dateAdded, b.lastModified "
          "FROM moz_bookmarks b "
          "JOIN moz_places_temp h ON b.fk = h.id AND b.type = 1 "
          "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
          "WHERE NOT EXISTS "
            "(SELECT id FROM moz_bookmarks "
              "WHERE id = b.parent AND parent = ") +
                nsPrintfCString("%lld", history->GetTagsFolder()) +
            NS_LITERAL_CSTRING(") "
            "{ADDITIONAL_CONDITIONS}"
          "UNION ALL "
          "SELECT b.fk, h.url, COALESCE(b.title, h.title), h.rev_host, "
            "h.visit_count,"
            SQL_STR_FRAGMENT_MAX_VISIT_DATE( "b.fk" )
            ", f.url, null, b.id, b.dateAdded, b.lastModified "
          "FROM moz_bookmarks b "
          "JOIN moz_places h ON b.fk = h.id AND b.type = 1 "
          "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
          "WHERE h.id NOT IN (SELECT id FROM moz_places_temp) "
            "AND NOT EXISTS "
              "(SELECT id FROM moz_bookmarks "
                "WHERE id = b.parent AND parent = ") +
                  nsPrintfCString("%lld", history->GetTagsFolder()) +
              NS_LITERAL_CSTRING(") "
            "{ADDITIONAL_CONDITIONS}");
      }
      break;

    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsVisit()
{
  if (!mIncludeHidden) {
    mQueryString = NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "v.visit_date, f.url, v.session, null "
      "FROM moz_places h "
      "JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.hidden <> 1 AND v.visit_type NOT IN ") +
          nsPrintfCString("(0,%d) ",
                          nsINavHistoryService::TRANSITION_EMBED) +
        NS_LITERAL_CSTRING("AND h.id NOT IN (SELECT id FROM moz_places_temp) "
        "{QUERY_OPTIONS} "
        "{ADDITIONAL_CONDITIONS} "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "v.visit_date, f.url, v.session, null "
      "FROM moz_places_temp h "
      "JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.hidden <> 1 AND v.visit_type NOT IN ") +
          nsPrintfCString("(0,%d) ",
                          nsINavHistoryService::TRANSITION_EMBED) +
        NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
        "{ADDITIONAL_CONDITIONS} "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "v.visit_date, f.url, v.session, null "
      "FROM moz_places h "
      "JOIN moz_historyvisits_temp v ON h.id = v.place_id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.id NOT IN (SELECT id FROM moz_places_temp) "
        "AND h.hidden <> 1 AND v.visit_type NOT IN ") +
          nsPrintfCString("(0,%d) ", 
                          nsINavHistoryService::TRANSITION_EMBED) +
        NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
        "{ADDITIONAL_CONDITIONS} "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "v.visit_date, f.url, v.session, null "
      "FROM moz_places_temp h "
      "JOIN moz_historyvisits_temp v ON h.id = v.place_id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.hidden <> 1 AND v.visit_type NOT IN ") +
          nsPrintfCString("(0,%d) ",
                          nsINavHistoryService::TRANSITION_EMBED) +
        NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
        "{ADDITIONAL_CONDITIONS} ");
  }
  else {
    mQueryString = NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "v.visit_date, f.url, v.session, null "
      "FROM moz_places h "
      "JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.id NOT IN (SELECT id FROM moz_places_temp) "
        "{QUERY_OPTIONS} "
        "{ADDITIONAL_CONDITIONS} "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "v.visit_date, f.url, v.session, null "
      "FROM moz_places_temp h "
      "JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      
      "WHERE 1=1 "
        "{QUERY_OPTIONS} "
        "{ADDITIONAL_CONDITIONS} "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "v.visit_date, f.url, v.session, null "
      "FROM moz_places h "
      "JOIN moz_historyvisits_temp v ON h.id = v.place_id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.id NOT IN (SELECT id FROM moz_places_temp) "
        "{QUERY_OPTIONS} "
        "{ADDITIONAL_CONDITIONS} "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "v.visit_date, f.url, v.session, null "
      "FROM moz_places_temp h "
      "JOIN moz_historyvisits_temp v ON h.id = v.place_id "
      "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
      
      "WHERE 1=1 "
        "{QUERY_OPTIONS} "
        "{ADDITIONAL_CONDITIONS} ");
  }

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsDay()
{
  mSkipOrderBy = PR_TRUE;

  PRUint16 resultType =
    mResultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ?
    nsINavHistoryQueryOptions::RESULTS_AS_URI :
    nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY;

  
  
  
  mQueryString = nsPrintfCString(1024,
     "SELECT null, "
       "'place:type=%ld&sort=%ld&beginTime='||beginTime||'&endTime='||endTime, "
      "dayTitle, null, null, beginTime, null, null, null, null "
     "FROM (", 
     resultType,
      nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING);
 
   nsNavHistory* history = nsNavHistory::GetHistoryService();
   NS_ENSURE_STATE(history);

  
  PRInt32 additionalContainers = 3;
  
  
  PRInt32 monthContainers = PR_MIN(6, (history->mExpireDaysMax/30));
  PRInt32 numContainers = monthContainers + additionalContainers;
  for (PRInt32 i = 0; i <= numContainers; i++) {
    nsCAutoString dateName;
    
    
    
    
    
    nsCAutoString sqlFragmentBeginTime;
    nsCAutoString sqlFragmentEndTime;
    switch(i) {
       case 0:
        
         history->GetStringFromName(
          NS_LITERAL_STRING("finduri-AgeInDays-is-0").get(), dateName);
        
        sqlFragmentBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of day','utc')*1000000)");
        
        sqlFragmentEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of day','+1 day','utc')*1000000)");
         break;
       case 1:
        
         history->GetStringFromName(
          NS_LITERAL_STRING("finduri-AgeInDays-is-1").get(), dateName);
        
        sqlFragmentBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of day','-1 day','utc')*1000000)");
        
        sqlFragmentEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of day','utc')*1000000)");
        break;
      case 2:
        
        history->GetAgeInDaysString(7,
          NS_LITERAL_STRING("finduri-AgeInDays-last-is").get(), dateName);
        
        sqlFragmentBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of day','-7 days','utc')*1000000)");
        
        sqlFragmentEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of day','+1 day','utc')*1000000)");
        break;
      case 3:
        
        history->GetStringFromName(
          NS_LITERAL_STRING("finduri-AgeInMonths-is-0").get(), dateName);
        
        sqlFragmentBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of month','utc')*1000000)");
        
        sqlFragmentEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of day','+1 day','utc')*1000000)");
         break;
       default:
        if (i == additionalContainers + 6) {
          
          history->GetAgeInDaysString(6,
            NS_LITERAL_STRING("finduri-AgeInMonths-isgreater").get(), dateName);
          
          sqlFragmentBeginTime = NS_LITERAL_CSTRING(
            "(datetime(0, 'unixepoch')*1000000)");
          
          sqlFragmentEndTime = NS_LITERAL_CSTRING(
            "(strftime('%s','now','start of day','-6 months','utc')*1000000)");
          break;
        }
        PRInt32 MonthIndex = i - additionalContainers;
        
        
        PRExplodedTime tm;
        PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &tm);
        PRUint16 currentYear = tm.tm_year;
        tm.tm_month -= MonthIndex;
        PR_NormalizeTime(&tm, PR_LocalTimeParameters);
        
        history->GetMonthName(tm.tm_month+1, dateName);

        
        if (tm.tm_year < currentYear)
          dateName.Append(nsPrintfCString(" %d", tm.tm_year));

        
        sqlFragmentBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of month','-");
        sqlFragmentBeginTime.AppendInt(MonthIndex);
        sqlFragmentBeginTime.Append(NS_LITERAL_CSTRING(
            " months','utc')*1000000)"));
        
        sqlFragmentEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','start of month','-");
        sqlFragmentEndTime.AppendInt(MonthIndex - 1);
        sqlFragmentEndTime.Append(NS_LITERAL_CSTRING(
            " months','utc')*1000000)"));
        break;
    }
 
     nsPrintfCString dayRange(1024,
        "SELECT '%s' AS dayTitle, "
               "%s AS beginTime, "
               "%s AS endTime "
         "WHERE EXISTS ( "
           "SELECT id FROM moz_historyvisits_temp "
          "WHERE visit_date >= %s "
            "AND visit_date < %s "
            "AND visit_type NOT IN (0,%d) "
            "{QUERY_OPTIONS} "
          "UNION ALL "
          "SELECT id FROM moz_historyvisits "
          "WHERE visit_date >= %s "
            "AND visit_date < %s "
             "AND visit_type NOT IN (0,%d) "
             "{QUERY_OPTIONS} "
           "LIMIT 1 "
        ") ",
      dateName.get(),
      sqlFragmentBeginTime.get(),
      sqlFragmentEndTime.get(),
      sqlFragmentBeginTime.get(),
      sqlFragmentEndTime.get(),
       nsINavHistoryService::TRANSITION_EMBED,
      sqlFragmentBeginTime.get(),
      sqlFragmentEndTime.get(),
      nsINavHistoryService::TRANSITION_EMBED);

    mQueryString.Append(dayRange);

    if (i < numContainers)
        mQueryString.Append(NS_LITERAL_CSTRING(" UNION ALL "));
  }

  mQueryString.Append(NS_LITERAL_CSTRING(") ")); 

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsSite()
{
  nsCAutoString localFiles;

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  history->GetStringFromName(NS_LITERAL_STRING("localhost").get(), localFiles);
  mAddParams.Put(NS_LITERAL_CSTRING(":localhost"), localFiles);

  
  if (mConditions.IsEmpty()) {
    mQueryString = nsPrintfCString(2048,
      "SELECT DISTINCT null, "
             "'place:type=%ld&sort=%ld&domain=&domainIsHost=true', "
             ":localhost, :localhost, null, null, null, null, null "
      "WHERE EXISTS ( "
        "SELECT id FROM moz_places_temp "
        "WHERE hidden <> 1 "
          "AND rev_host = '.' "
          "AND visit_count > 0 "
          "AND url BETWEEN 'file://' AND 'file:/~' "
        "UNION ALL "
        "SELECT id FROM moz_places "
        "WHERE id NOT IN (SELECT id FROM moz_places_temp) "
          "AND hidden <> 1 "
          "AND rev_host = '.' "
          "AND visit_count > 0 "
          "AND url BETWEEN 'file://' AND 'file:/~' "
      ") "
      "UNION ALL "
      "SELECT DISTINCT null, "
             "'place:type=%ld&sort=%ld&domain='||host||'&domainIsHost=true', "
             "host, host, null, null, null, null, null "
      "FROM ( "
        "SELECT get_unreversed_host(rev_host) host "
        "FROM ( "
          "SELECT DISTINCT rev_host FROM moz_places_temp "
          "WHERE hidden <> 1 "
            "AND rev_host <> '.' "
            "AND visit_count > 0 "
          "UNION ALL "
          "SELECT DISTINCT rev_host FROM moz_places "
          "WHERE id NOT IN (SELECT id FROM moz_places_temp) "
            "AND hidden <> 1 "
            "AND rev_host <> '.' "
            "AND visit_count > 0 "
        ") "
      "ORDER BY 1 ASC)",
      nsINavHistoryQueryOptions::RESULTS_AS_URI,
      nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING,
      nsINavHistoryQueryOptions::RESULTS_AS_URI,
      nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING);
  
  } else {

    mQueryString = nsPrintfCString(4096,
      "SELECT DISTINCT null, "
             "'place:type=%ld&sort=%ld&domain=&domainIsHost=true"
               "&beginTime='||:begin_time||'&endTime='||:end_time, "
             ":localhost, :localhost, null, null, null, null, null "
      "WHERE EXISTS( "
        "SELECT h.id "
        "FROM moz_places h "
        "JOIN moz_historyvisits v ON v.place_id = h.id "
        "WHERE h.hidden <> 1 AND h.rev_host = '.' "
          "AND h.visit_count > 0 "
          "AND h.url BETWEEN 'file://' AND 'file:/~' "
          "{QUERY_OPTIONS} "
          "{ADDITIONAL_CONDITIONS} "
        "UNION "
        "SELECT h.id "
        "FROM moz_places_temp h "
        "JOIN moz_historyvisits v ON v.place_id = h.id "
        "WHERE h.hidden <> 1 AND h.rev_host = '.' "
          "AND h.visit_count > 0 "
          "AND h.url BETWEEN 'file://' AND 'file:/~' "
          "{QUERY_OPTIONS} "
          "{ADDITIONAL_CONDITIONS} "
        "UNION "
        "SELECT h.id "
        "FROM moz_places h "
        "JOIN moz_historyvisits_temp v ON v.place_id = h.id "
        "WHERE h.hidden <> 1 AND h.rev_host = '.' "
          "AND h.visit_count > 0 "
          "AND h.url BETWEEN 'file://' AND 'file:/~' "
          "{QUERY_OPTIONS} "
          "{ADDITIONAL_CONDITIONS} "
        "UNION "
        "SELECT h.id "
        "FROM moz_places_temp h "
        "JOIN moz_historyvisits_temp v ON v.place_id = h.id "
        "WHERE h.hidden <> 1 AND h.rev_host = '.' "
          "AND h.visit_count > 0 "
          "AND h.url BETWEEN 'file://' AND 'file:/~' "
          "{QUERY_OPTIONS} "
          "{ADDITIONAL_CONDITIONS} "        
      ") "
      "UNION ALL "
      "SELECT DISTINCT null, "
             "'place:type=%ld&sort=%ld&domain='||host||'&domainIsHost=true"
               "&beginTime='||:begin_time||'&endTime='||:end_time, "
             "host, host, null, null, null, null, null "
      "FROM ( "
        "SELECT DISTINCT get_unreversed_host(rev_host) AS host "
        "FROM moz_places h "
        "JOIN moz_historyvisits v ON v.place_id = h.id "
        "WHERE h.hidden <> 1 AND h.rev_host <> '.' "
          "AND h.visit_count > 0 "
          "{QUERY_OPTIONS} "
          "{ADDITIONAL_CONDITIONS} "
        "UNION "
        "SELECT DISTINCT get_unreversed_host(rev_host) AS host "
        "FROM moz_places_temp h "
        "JOIN moz_historyvisits v ON v.place_id = h.id "
        "WHERE h.hidden <> 1 AND h.rev_host <> '.' "
          "AND h.visit_count > 0 "
          "{QUERY_OPTIONS} "
          "{ADDITIONAL_CONDITIONS} "
        "UNION "
        "SELECT DISTINCT get_unreversed_host(rev_host) AS host "
        "FROM moz_places h "
        "JOIN moz_historyvisits_temp v ON v.place_id = h.id "
        "WHERE h.hidden <> 1 AND h.rev_host <> '.' "
          "AND h.visit_count > 0 "
          "{QUERY_OPTIONS} "
          "{ADDITIONAL_CONDITIONS} "
        "UNION "
        "SELECT DISTINCT get_unreversed_host(rev_host) AS host "
        "FROM moz_places_temp h "
        "JOIN moz_historyvisits_temp v ON v.place_id = h.id "        
        "WHERE h.hidden <> 1 AND h.rev_host <> '.' "
          "AND h.visit_count > 0 "
          "{QUERY_OPTIONS} "
          "{ADDITIONAL_CONDITIONS} "        
        "ORDER BY 1 ASC "
      ")",
      nsINavHistoryQueryOptions::RESULTS_AS_URI,
      nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING,
      nsINavHistoryQueryOptions::RESULTS_AS_URI,
      nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING);
  }

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsTag()
{
  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  
  
  mHasDateColumns = PR_TRUE; 

  mQueryString = nsPrintfCString(2048,
    "SELECT null, 'place:folder=' || id || '&queryType=%d&type=%ld', "
      "title, null, null, null, null, null, null, dateAdded, lastModified "
    "FROM   moz_bookmarks "
    "WHERE  parent = %lld",
    nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS,
    nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS,
    history->GetTagsFolder());

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::Where()
{

  
  nsCAutoString additionalQueryOptions;
  if (mRedirectsMode == nsINavHistoryQueryOptions::REDIRECTS_MODE_SOURCE) {
    additionalQueryOptions += NS_LITERAL_CSTRING(
      "AND v.visit_type NOT IN (5, 6) ");
  }
  else if (mRedirectsMode == nsINavHistoryQueryOptions::REDIRECTS_MODE_TARGET) {
    additionalQueryOptions += NS_LITERAL_CSTRING(
      "AND NOT EXISTS ( "
        "SELECT id FROM moz_historyvisits_temp WHERE from_visit = v.id "
        "AND visit_type IN (5, 6) "
      ") AND NOT EXISTS ( "
        "SELECT id FROM moz_historyvisits WHERE from_visit = v.id "
        "AND visit_type IN (5, 6) "
      ") ");
  }

  mQueryString.ReplaceSubstring("{QUERY_OPTIONS}",
                                additionalQueryOptions.get());

  
  
  PRInt32 useInnerCondition;
  useInnerCondition = mQueryString.Find("{ADDITIONAL_CONDITIONS}", 0);
  if (useInnerCondition != kNotFound) {

    nsCAutoString innerCondition;
    
    if (!mConditions.IsEmpty()) {
      innerCondition = " AND (";
      innerCondition += mConditions;
      innerCondition += ")";
    }
    mQueryString.ReplaceSubstring("{ADDITIONAL_CONDITIONS}",
                                  innerCondition.get());

  } else if (!mConditions.IsEmpty()) {

    mQueryString += "WHERE ";
    mQueryString += mConditions;

  }
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::GroupBy()
{
  mQueryString += mGroupBy;
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::OrderBy()
{
  if (mSkipOrderBy)
    return NS_OK;

  
  
  
  switch(mSortingMode)
  {
    case nsINavHistoryQueryOptions::SORT_BY_NONE:
      break;
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING:
      
      
      
      
      
      
      if (mMaxResults > 0)
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_VisitDate);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING:
      OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_VisitDate);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING:
      OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_VisitDate);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_URI_ASCENDING:
      OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_URL);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_URI_DESCENDING:
      OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_URL);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING:
      OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_VisitCount);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING:
      OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_VisitCount);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATEADDED_ASCENDING:
      if (mHasDateColumns)
        OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_ItemDateAdded);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATEADDED_DESCENDING:
      if (mHasDateColumns)
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_ItemDateAdded);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_LASTMODIFIED_ASCENDING:
      if (mHasDateColumns)
        OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_ItemLastModified);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_LASTMODIFIED_DESCENDING:
      if (mHasDateColumns)
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_ItemLastModified);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_TAGS_ASCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_TAGS_DESCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_ASCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_DESCENDING:
      break; 
    default:
      NS_NOTREACHED("Invalid sorting mode");
  }
  return NS_OK;
}

void PlacesSQLQueryBuilder::OrderByColumnIndexAsc(PRInt32 aIndex)
{
  mQueryString += nsPrintfCString(128, " ORDER BY %d ASC", aIndex+1);
}

void PlacesSQLQueryBuilder::OrderByColumnIndexDesc(PRInt32 aIndex)
{
  mQueryString += nsPrintfCString(128, " ORDER BY %d DESC", aIndex+1);
}

nsresult
PlacesSQLQueryBuilder::Limit()
{
  if (mUseLimit && mMaxResults > 0) {
    mQueryString += NS_LITERAL_CSTRING(" LIMIT ");
    mQueryString.AppendInt(mMaxResults);
    mQueryString.AppendLiteral(" ");
  }
  return NS_OK;
}

nsresult
nsNavHistory::ConstructQueryString(
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions, 
    nsCString& queryString, 
    PRBool& aParamsPresent,
    nsNavHistory::StringHash& aAddParams)
{
  nsresult rv;

  
  
  
  
  
  

  aParamsPresent = PR_FALSE;

  PRInt32 sortingMode = aOptions->SortingMode();
  if (sortingMode < 0 ||
      sortingMode > nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_DESCENDING) {
    return NS_ERROR_INVALID_ARG;
  }

  
  
  if (IsHistoryMenuQuery(aQueries, aOptions, 
        nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING)) {

    nsCString sqlFragment = NS_LITERAL_CSTRING(
      "SELECT * FROM ( "
        "SELECT DISTINCT place_id "
        "FROM moz_historyvisits v "
        "WHERE visit_type NOT IN ") +
          nsPrintfCString("(0,%d) ", nsINavHistoryService::TRANSITION_EMBED) +
          NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
          "AND NOT EXISTS "
            "(SELECT id FROM moz_places h WHERE h.id = place_id AND hidden = 1) "
          "AND NOT EXISTS (SELECT id FROM moz_places_temp h WHERE h.id = place_id AND hidden = 1) "
        "ORDER by visit_date DESC LIMIT ") +
        nsPrintfCString("%d ", aOptions->MaxResults()) +
      NS_LITERAL_CSTRING(") "
      "UNION ALL "
      "SELECT * FROM ( "
        "SELECT DISTINCT place_id "
        "FROM moz_historyvisits_temp v "
        "WHERE visit_type NOT IN ") +
          nsPrintfCString("(0,%d) ", nsINavHistoryService::TRANSITION_EMBED) +
          NS_LITERAL_CSTRING("{QUERY_OPTIONS} "
          "AND NOT EXISTS "
            "(SELECT id FROM moz_places h WHERE h.id = place_id AND hidden = 1) "
          "AND NOT EXISTS (SELECT id FROM moz_places_temp h WHERE h.id = place_id AND hidden = 1) "
        "ORDER by visit_date DESC LIMIT ") +
        nsPrintfCString("%d ", aOptions->MaxResults()) +
      NS_LITERAL_CSTRING(")");

    queryString = NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
          ", f.url, null, null "
        "FROM moz_places_temp h "
        "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE h.id IN ( ") + sqlFragment + NS_LITERAL_CSTRING(") "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
          ", f.url, null, null "
        "FROM moz_places h "
        "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE h.id IN ( ") + sqlFragment + NS_LITERAL_CSTRING(") "
        "AND h.id NOT IN (SELECT id FROM moz_places_temp) "
        "ORDER BY 6 DESC " 
        "LIMIT ");
    
    queryString.AppendInt(aOptions->MaxResults());

    nsCAutoString additionalQueryOptions;
    if (aOptions->RedirectsMode() ==
          nsINavHistoryQueryOptions::REDIRECTS_MODE_SOURCE) {
      additionalQueryOptions += NS_LITERAL_CSTRING(
        "AND v.visit_type NOT IN (5,6) ");
    }
    else if (aOptions->RedirectsMode() ==
              nsINavHistoryQueryOptions::REDIRECTS_MODE_TARGET) {
      additionalQueryOptions += NS_LITERAL_CSTRING(
        "AND NOT EXISTS ( "
          "SELECT id FROM moz_historyvisits_temp WHERE from_visit = v.id "
          "AND visit_type IN (5, 6) "
        ") AND NOT EXISTS ( "
          "SELECT id FROM moz_historyvisits WHERE from_visit = v.id "
          "AND visit_type IN (5, 6) "
        ") ");
    }
    queryString.ReplaceSubstring("{QUERY_OPTIONS}",
                                  additionalQueryOptions.get());

    return NS_OK;
  }

  
  
  if (IsHistoryMenuQuery(aQueries, aOptions, 
        nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING)) {
    queryString = NS_LITERAL_CSTRING(
      "SELECT * FROM ( "
        "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
          ", f.url, null, null "
        "FROM moz_places_temp h "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE h.hidden <> 1 AND h.visit_count > 0 "
          "{QUERY_OPTIONS} "
        "ORDER BY h.visit_count DESC LIMIT ") +
        nsPrintfCString("%d ", aOptions->MaxResults()) +
      NS_LITERAL_CSTRING(") "
      "UNION ALL "
      "SELECT * FROM ( "
        "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
          ", f.url, null, null "
        "FROM moz_places h "
        "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE h.hidden <> 1 AND h.visit_count > 0 "
          "{QUERY_OPTIONS} "
          "AND h.id NOT IN (SELECT id FROM moz_places_temp) "
        "ORDER BY h.visit_count DESC LIMIT ") +
        nsPrintfCString("%d ", aOptions->MaxResults()) +
      NS_LITERAL_CSTRING(") "
      "ORDER BY 5 DESC LIMIT "); 
    queryString.AppendInt(aOptions->MaxResults());

    nsCAutoString additionalQueryOptions;

    if (aOptions->RedirectsMode() ==
          nsINavHistoryQueryOptions::REDIRECTS_MODE_SOURCE) {
      additionalQueryOptions += NS_LITERAL_CSTRING(
        "AND ( "
          "EXISTS (SELECT id FROM moz_historyvisits WHERE place_id = h.id AND visit_type NOT IN (5,6) LIMIT 1) "
          "OR EXISTS (SELECT id FROM moz_historyvisits_temp WHERE place_id = h.id AND visit_type NOT IN (5,6) LIMIT 1) "
        ") ");
    }
    else if (aOptions->RedirectsMode() ==
              nsINavHistoryQueryOptions::REDIRECTS_MODE_TARGET) {
      additionalQueryOptions += NS_LITERAL_CSTRING(
        "AND NOT EXISTS ( "
          "SELECT id FROM moz_historyvisits_temp WHERE from_visit IN ( "
            "SELECT id FROM moz_historyvisits_temp WHERE place_id = h.id) "
        ") "
        "AND NOT EXISTS ( "
          "SELECT id FROM moz_historyvisits_temp WHERE from_visit IN ( "
            "SELECT id FROM moz_historyvisits WHERE place_id = h.id) "
        ") "
        "AND NOT EXISTS ( "
          "SELECT id FROM moz_historyvisits WHERE from_visit IN ( "
            "SELECT id FROM moz_historyvisits_temp WHERE place_id = h.id) "
        ") "
        "AND NOT EXISTS ( "
          "SELECT id FROM moz_historyvisits WHERE from_visit IN ( "
            "SELECT id FROM moz_historyvisits WHERE place_id = h.id) "
        ") ");
    }

    queryString.ReplaceSubstring("{QUERY_OPTIONS}",
                                  additionalQueryOptions.get());

    return NS_OK;
  }  

  nsCAutoString conditions;

  PRInt32 i;
  for (i = 0; i < aQueries.Count(); i ++) {
    nsCString queryClause;
    rv = QueryToSelectClause(aQueries[i], aOptions, i, &queryClause);
    NS_ENSURE_SUCCESS(rv, rv);
    if (! queryClause.IsEmpty()) {
      aParamsPresent = PR_TRUE;
      if (! conditions.IsEmpty()) 
        conditions += NS_LITERAL_CSTRING(" OR ");
      conditions += NS_LITERAL_CSTRING("(") + queryClause +
        NS_LITERAL_CSTRING(")");
    }
  }

  
  
  
  PRBool useLimitClause = !NeedToFilterResultSet(aQueries, aOptions);

  PlacesSQLQueryBuilder queryStringBuilder(conditions, aOptions, 
                                           useLimitClause, aAddParams);
  rv = queryStringBuilder.GetQueryString(queryString);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PLDHashOperator BindAdditionalParameter(nsNavHistory::StringHash::KeyType aParamName, 
                                        nsCString aParamValue,
                                        void* aStatement)
{
  mozIStorageStatement* stmt = static_cast<mozIStorageStatement*>(aStatement);

  PRUint32 index;
  nsresult rv = stmt->GetParameterIndex(aParamName, &index);

  if (NS_FAILED(rv))
    return PL_DHASH_STOP;

  rv = stmt->BindUTF8StringParameter(index, aParamValue);
  if (NS_FAILED(rv))
    return PL_DHASH_STOP;

  return PL_DHASH_NEXT;
}















nsresult
nsNavHistory::GetQueryResults(nsNavHistoryQueryResultNode *aResultNode,
                              const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions *aOptions,
                              nsCOMArray<nsNavHistoryResultNode>* aResults)
{
  NS_ENSURE_ARG_POINTER(aOptions);
  NS_ASSERTION(aResults->Count() == 0, "Initial result array must be empty");
  if (! aQueries.Count())
    return NS_ERROR_INVALID_ARG;

  nsCString queryString;
  PRBool paramsPresent = PR_FALSE;
  nsNavHistory::StringHash addParams;
  addParams.Init(1);
  nsresult rv = ConstructQueryString(aQueries, aOptions, queryString, 
                                     paramsPresent, addParams);
  NS_ENSURE_SUCCESS(rv,rv);

#ifdef DEBUG_FRECENCY
  printf("Constructed the query: %s\n", PromiseFlatCString(queryString).get());
#endif

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(queryString, getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  if (paramsPresent) {
    
    PRInt32 i;
    for (i = 0; i < aQueries.Count(); i++) {
      rv = BindQueryClauseParameters(statement, i, aQueries[i], aOptions);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  addParams.EnumerateRead(BindAdditionalParameter, statement.get());

  
  
  if (NeedToFilterResultSet(aQueries, aOptions)) {
    
    nsCOMArray<nsNavHistoryResultNode> toplevel;
    rv = ResultsAsList(statement, aOptions, &toplevel);
    NS_ENSURE_SUCCESS(rv, rv);

    FilterResultSet(aResultNode, toplevel, aResults, aQueries, aOptions);
  } else {
    rv = ResultsAsList(statement, aOptions, aResults);
    NS_ENSURE_SUCCESS(rv, rv);
  } 

  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::AddObserver(nsINavHistoryObserver* aObserver, PRBool aOwnsWeak)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  return mObservers.AppendWeakElement(aObserver, aOwnsWeak);
}




NS_IMETHODIMP
nsNavHistory::RemoveObserver(nsINavHistoryObserver* aObserver)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  return mObservers.RemoveWeakElement(aObserver);
}



nsresult
nsNavHistory::BeginUpdateBatch()
{
  if (mBatchLevel++ == 0) {
    PRBool transactionInProgress = PR_TRUE; 
    mDBConn->GetTransactionInProgress(&transactionInProgress);
    mBatchHasTransaction = ! transactionInProgress;
    if (mBatchHasTransaction)
      mDBConn->BeginTransaction();

    ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                        OnBeginUpdateBatch())
  }
  return NS_OK;
}


nsresult
nsNavHistory::EndUpdateBatch()
{
  if (--mBatchLevel == 0) {
    if (mBatchHasTransaction)
      mDBConn->CommitTransaction();
    mBatchHasTransaction = PR_FALSE;
    ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver, OnEndUpdateBatch())
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::RunInBatchMode(nsINavHistoryBatchCallback* aCallback,
                             nsISupports* aUserData)
{
  NS_ENSURE_ARG_POINTER(aCallback);
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  UpdateBatchScoper batch(*this);
  return aCallback->RunBatched(aUserData);
}

NS_IMETHODIMP
nsNavHistory::GetHistoryDisabled(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  *_retval = IsHistoryDisabled();
  return NS_OK;
}











NS_IMETHODIMP
nsNavHistory::AddPageWithDetails(nsIURI *aURI, const PRUnichar *aTitle,
                                 PRInt64 aLastVisited)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  PRInt64 visitID;
  nsresult rv = AddVisit(aURI, aLastVisited, 0, TRANSITION_LINK, PR_FALSE,
                         0, &visitID);
  NS_ENSURE_SUCCESS(rv, rv);

  return SetPageTitleInternal(aURI, nsString(aTitle));
}








NS_IMETHODIMP
nsNavHistory::GetLastPageVisited(nsACString & aLastPageVisited)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsCOMPtr<mozIStorageStatement> statement;
  
  
  
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT * FROM ( "
        "SELECT url, visit_date FROM moz_historyvisits_temp v "
        "JOIN moz_places_temp h ON v.place_id = h.id "
        "WHERE h.hidden <> 1 "
        "ORDER BY visit_date DESC LIMIT 1 "
      ") "
      "UNION ALL "
      "SELECT * FROM ( "
        "SELECT url, visit_date FROM moz_historyvisits_temp v "
        "JOIN moz_places h ON v.place_id = h.id "
        "WHERE h.hidden <> 1 "
        "ORDER BY visit_date DESC LIMIT 1 "
      ") "
      "UNION ALL "
      "SELECT * FROM ( "
        "SELECT url, visit_date FROM moz_historyvisits v "
        "JOIN moz_places h ON v.place_id = h.id "
        "WHERE h.hidden <> 1 "
        "ORDER BY visit_date DESC LIMIT 1 "
      ") "
      "UNION ALL "
      "SELECT * FROM ( "
        "SELECT url, visit_date FROM moz_historyvisits v "
        "JOIN moz_places_temp h ON v.place_id = h.id "
        "WHERE h.hidden <> 1 "
        "ORDER BY visit_date DESC LIMIT 1 "
      ") "
      "ORDER BY 2 DESC LIMIT 1"), 
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMatch = PR_FALSE;
  if (NS_SUCCEEDED(statement->ExecuteStep(&hasMatch)) && hasMatch) {
    return statement->GetUTF8String(0, aLastPageVisited);
  }
  aLastPageVisited.Truncate(0);
  return NS_OK;
}









NS_IMETHODIMP
nsNavHistory::GetCount(PRUint32 *aCount)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  PRBool hasEntries = PR_FALSE;
  nsresult rv = GetHasHistoryEntries(&hasEntries);
  if (hasEntries)
    *aCount = 1;
  else
    *aCount = 0;
  return rv;
}









nsresult
nsNavHistory::RemovePagesInternal(const nsCString& aPlaceIdsQueryString)
{
  
  if (aPlaceIdsQueryString.IsEmpty())
    return NS_OK;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  
  
  
  
  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "UPDATE moz_places_view "
      "SET frecency = -MAX(visit_count, 1) "
      "WHERE id IN ( "
        "SELECT h.id " 
        "FROM moz_places_temp h "
        "WHERE h.id IN ( ") + aPlaceIdsQueryString + NS_LITERAL_CSTRING(") "
          "AND ( "
            "EXISTS (SELECT b.id FROM moz_bookmarks b WHERE b.fk =h.id) "
            "OR EXISTS (SELECT a.id FROM moz_annos a WHERE a.place_id = h.id) "
          ") "
        "UNION ALL "
        "SELECT h.id " 
        "FROM moz_places h "
        "WHERE h.id IN ( ") + aPlaceIdsQueryString + NS_LITERAL_CSTRING(") "
          "AND h.id NOT IN (SELECT id FROM moz_places_temp) "
          "AND ( "
            "EXISTS (SELECT b.id FROM moz_bookmarks b WHERE b.fk =h.id) "
            "OR EXISTS (SELECT a.id FROM moz_annos a WHERE a.place_id = h.id) "
          ") "        
      ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits_view WHERE place_id IN (") +
        aPlaceIdsQueryString +
        NS_LITERAL_CSTRING(")"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  (void)mExpire.OnDeleteURI();

  
  
  
  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_places_view WHERE id IN ("
        "SELECT h.id FROM moz_places_temp h "
        "WHERE h.id IN ( ") + aPlaceIdsQueryString + NS_LITERAL_CSTRING(") "
          "AND SUBSTR(h.url, 0, 6) <> 'place:' "
          "AND NOT EXISTS "
            "(SELECT b.id FROM moz_bookmarks b WHERE b.fk = h.id LIMIT 1) "
        "UNION ALL "
        "SELECT h.id FROM moz_places h "
        "WHERE h.id NOT IN (SELECT id FROM moz_places_temp) "
          "AND h.id IN ( ") + aPlaceIdsQueryString + NS_LITERAL_CSTRING(") "
          "AND SUBSTR(h.url, 0, 6) <> 'place:' "
          "AND NOT EXISTS "
            "(SELECT b.id FROM moz_bookmarks b WHERE b.fk = h.id LIMIT 1) "
    ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = FixInvalidFrecenciesForExcludedPlaces();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  

  return transaction.Commit();
}










NS_IMETHODIMP
nsNavHistory::RemovePages(nsIURI **aURIs, PRUint32 aLength, PRBool aDoBatchNotify)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv;
  
  nsCString deletePlaceIdsQueryString;
  for (PRUint32 i = 0; i < aLength; i++) {
    PRInt64 placeId;
    rv = GetUrlIdFor(aURIs[i], &placeId, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    if (placeId != 0) {
      if (!deletePlaceIdsQueryString.IsEmpty())
        deletePlaceIdsQueryString.AppendLiteral(",");
      deletePlaceIdsQueryString.AppendInt(placeId);
    }
  }

  rv = RemovePagesInternal(deletePlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aDoBatchNotify)
    UpdateBatchScoper batch(*this); 

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::RemovePage(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsIURI** URIs = &aURI;
  nsresult rv = RemovePages(URIs, 1, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver, OnDeleteURI(aURI))
  return NS_OK;
}















NS_IMETHODIMP
nsNavHistory::RemovePagesFromHost(const nsACString& aHost, PRBool aEntireDomain)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv;
  
  
  if (aHost.IsEmpty())
    aEntireDomain = PR_FALSE;

  
  
  nsCString localFiles;
  TitleForDomain(EmptyCString(), localFiles);
  nsAutoString host16;
  if (!aHost.Equals(localFiles))
    CopyUTF8toUTF16(aHost, host16);

  
  nsCOMPtr<nsISupportsString> hostSupports(do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = hostSupports->SetData(host16);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString revHostDot;
  GetReversedHostname(host16, revHostDot);
  NS_ASSERTION(revHostDot[revHostDot.Length() - 1] == '.', "Invalid rev. host");
  nsAutoString revHostSlash(revHostDot);
  revHostSlash.Truncate(revHostSlash.Length() - 1);
  revHostSlash.Append(NS_LITERAL_STRING("/"));

  
  nsCAutoString conditionString;
  if (aEntireDomain)
    conditionString.AssignLiteral("rev_host >= ?1 AND rev_host < ?2 ");
  else
    conditionString.AssignLiteral("rev_host = ?1 ");

  nsCOMPtr<mozIStorageStatement> statement;

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id FROM moz_places_temp "
      "WHERE ") + conditionString + NS_LITERAL_CSTRING(
      "UNION ALL "
      "SELECT id FROM moz_places "
      "WHERE id NOT IN (SELECT id FROM moz_places_temp) "
        "AND ") + conditionString,
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(0, revHostDot);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aEntireDomain) {
    rv = statement->BindStringParameter(1, revHostSlash);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCString hostPlaceIds;
  PRBool hasMore = PR_FALSE;
  while ((statement->ExecuteStep(&hasMore) == NS_OK) && hasMore) {
    if (!hostPlaceIds.IsEmpty())
      hostPlaceIds.AppendLiteral(",");
    PRInt64 placeId;
    rv = statement->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    hostPlaceIds.AppendInt(placeId);
  }

  
  UpdateBatchScoper batch(*this); 

  rv = RemovePagesInternal(hostPlaceIds);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}










NS_IMETHODIMP
nsNavHistory::RemovePagesByTimeframe(PRTime aBeginTime, PRTime aEndTime)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv;
  
  nsCString deletePlaceIdsQueryString;

  
  
  nsCOMPtr<mozIStorageStatement> selectByTime;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id FROM moz_places_temp h WHERE "
        "EXISTS "
          "(SELECT id FROM moz_historyvisits v WHERE v.place_id = h.id "
            "AND v.visit_date >= ?1 AND v.visit_date <= ?2 LIMIT 1)"
        "OR EXISTS "
          "(SELECT id FROM moz_historyvisits_temp v WHERE v.place_id = h.id "
            "AND v.visit_date >= ?1 AND v.visit_date <= ?2 LIMIT 1) "
      "UNION "
      "SELECT h.id FROM moz_places h WHERE "
        "EXISTS "
          "(SELECT id FROM moz_historyvisits v WHERE v.place_id = h.id "
            "AND v.visit_date >= ?1 AND v.visit_date <= ?2 LIMIT 1)"
        "OR EXISTS "
          "(SELECT id FROM moz_historyvisits_temp v WHERE v.place_id = h.id "
            "AND v.visit_date >= ?1 AND v.visit_date <= ?2 LIMIT 1)"),
    getter_AddRefs(selectByTime));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = selectByTime->BindInt64Parameter(0, aBeginTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = selectByTime->BindInt64Parameter(1, aEndTime);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(selectByTime->ExecuteStep(&hasMore)) && hasMore) {
    PRInt64 placeId;
    rv = selectByTime->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    if (placeId != 0) {
      if (!deletePlaceIdsQueryString.IsEmpty())
        deletePlaceIdsQueryString.AppendLiteral(",");
      deletePlaceIdsQueryString.AppendInt(placeId);
    }
  }

  rv = RemovePagesInternal(deletePlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  UpdateBatchScoper batch(*this); 

  return NS_OK;
}






NS_IMETHODIMP
nsNavHistory::RemoveAllPages()
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  mExpire.ClearHistory();

  
  
#if 0
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("VACUUM"));
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  
  nsCOMPtr<nsIFile> oldHistoryFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_HISTORY_50_FILE,
                                       getter_AddRefs(oldHistoryFile));
  if (NS_FAILED(rv)) return rv;

  PRBool fileExists;
  if (NS_SUCCEEDED(oldHistoryFile->Exists(&fileExists)) && fileExists) {
    rv = oldHistoryFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::HidePage(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  return NS_ERROR_NOT_IMPLEMENTED;
  



























































}












NS_IMETHODIMP
nsNavHistory::MarkPageAsTyped(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  if (IsHistoryDisabled())
    return NS_OK;

  nsCAutoString uriString;
  nsresult rv = aURI->GetSpec(uriString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 unusedEventTime;
  if (mRecentTyped.Get(uriString, &unusedEventTime))
    mRecentTyped.Remove(uriString);

  if (mRecentTyped.Count() > RECENT_EVENT_QUEUE_MAX_LENGTH)
    ExpireNonrecentEvents(&mRecentTyped);

  mRecentTyped.Put(uriString, GetNow());
  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::SetCharsetForURI(nsIURI* aURI,
                               const nsAString& aCharset)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);

  if (aCharset.IsEmpty()) {
    
    nsresult rv = annosvc->RemovePageAnnotation(aURI, CHARSET_ANNO);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    nsresult rv = annosvc->SetPageAnnotationString(aURI, CHARSET_ANNO,
                                                   aCharset, 0,
                                                   nsAnnotationService::EXPIRE_NEVER);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}






NS_IMETHODIMP
nsNavHistory::GetCharsetForURI(nsIURI* aURI, 
                               nsAString& aCharset)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString charset;
  nsresult rv = annosvc->GetPageAnnotationString(aURI, CHARSET_ANNO, aCharset);
  if (NS_FAILED(rv)) {
    
    aCharset.Truncate();
  }
  return NS_OK;
}









NS_IMETHODIMP
nsNavHistory::AddURI(nsIURI *aURI, PRBool aRedirect,
                     PRBool aToplevel, nsIURI *aReferrer)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  if (IsHistoryDisabled())
    return NS_OK;

  
  PRBool canAdd = PR_FALSE;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd)
    return NS_OK;

  PRTime now = PR_Now();

#ifdef LAZY_ADD
  LazyMessage message;
  rv = message.Init(LazyMessage::Type_AddURI, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  message.isRedirect = aRedirect;
  message.isToplevel = aToplevel;
  if (aReferrer) {
    rv = aReferrer->Clone(getter_AddRefs(message.referrer));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  message.time = now;
  rv = AddLazyMessage(message);
  NS_ENSURE_SUCCESS(rv, rv);
#else
  rv = AddURIInternal(aURI, now, aRedirect, aToplevel, aReferrer);
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  return NS_OK;
}






nsresult
nsNavHistory::AddURIInternal(nsIURI* aURI, PRTime aTime, PRBool aRedirect,
                             PRBool aToplevel, nsIURI* aReferrer)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  PRInt64 redirectBookmark = 0;
  PRInt64 visitID, sessionID;
  nsresult rv = AddVisitChain(aURI, aTime, aToplevel, aRedirect, aReferrer,
                              &visitID, &sessionID, &redirectBookmark);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  if (redirectBookmark) {
    nsNavBookmarks* bookmarkService = nsNavBookmarks::GetBookmarksService();
    if (bookmarkService) {
      PRTime now = GetNow();
      bookmarkService->AddBookmarkToHash(redirectBookmark,
                                         now - BOOKMARK_REDIRECT_TIME_THRESHOLD);
    }
  }

  return transaction.Commit();
}






















nsresult
nsNavHistory::AddVisitChain(nsIURI* aURI, PRTime aTime,
                            PRBool aToplevel, PRBool aIsRedirect,
                            nsIURI* aReferrerURI, PRInt64* aVisitID,
                            PRInt64* aSessionID, PRInt64* aRedirectBookmark)
{
  PRUint32 transitionType = 0;
  PRInt64 referringVisit = 0;
  PRTime visitTime = 0;
  nsCOMPtr<nsIURI> fromVisitURI = aReferrerURI;

  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString redirectSource;
  if (GetRedirectFor(spec, redirectSource, &visitTime, &transitionType)) {
    
    nsCOMPtr<nsIURI> redirectURI;
    rv = NS_NewURI(getter_AddRefs(redirectURI), redirectSource);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsNavBookmarks* bookmarkService = nsNavBookmarks::GetBookmarksService();
    PRBool isBookmarked;
    if (bookmarkService &&
        NS_SUCCEEDED(bookmarkService->IsBookmarked(redirectURI, &isBookmarked))
        && isBookmarked) {
      GetUrlIdFor(redirectURI, aRedirectBookmark, PR_FALSE);
    }

    
    
    
    
    
    rv = AddVisitChain(redirectURI, aTime - 1, aToplevel, PR_TRUE, aReferrerURI,
                       &referringVisit, aSessionID, aRedirectBookmark);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!aToplevel) {
      transitionType = nsINavHistoryService::TRANSITION_EMBED;
    }

    
    
    
    fromVisitURI = redirectURI;
  } else if (aReferrerURI) {
    
    
    
    PRBool referrerIsSame;
    if (NS_SUCCEEDED(aURI->Equals(aReferrerURI, &referrerIsSame)) && referrerIsSame)
      return NS_OK;

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (aToplevel)
      transitionType = nsINavHistoryService::TRANSITION_LINK;
    else
      transitionType = nsINavHistoryService::TRANSITION_EMBED;

    
    
    
    
    
    visitTime = PR_Now();

    
    
    if (!FindLastVisit(aReferrerURI, &referringVisit, aSessionID)) {
      
      *aSessionID = GetNewSessionID();
    }
  } else {
    
    
    
    
    
    
    
    
    if (CheckIsRecentEvent(&mRecentTyped, spec))
      transitionType = nsINavHistoryService::TRANSITION_TYPED;
    else if (CheckIsRecentEvent(&mRecentBookmark, spec))
      transitionType = nsINavHistoryService::TRANSITION_BOOKMARK;
    else if (aToplevel)
      transitionType = nsINavHistoryService::TRANSITION_LINK;
    else
      transitionType = nsINavHistoryService::TRANSITION_EMBED;

    visitTime = PR_Now();
    *aSessionID = GetNewSessionID();
  }

  
  return AddVisit(aURI, visitTime, fromVisitURI, transitionType,
                  aIsRedirect, *aSessionID, aVisitID);
}








NS_IMETHODIMP
nsNavHistory::IsVisited(nsIURI *aURI, PRBool *_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  if (IsHistoryDisabled()) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  nsCAutoString utf8URISpec;
  nsresult rv = aURI->GetSpec(utf8URISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = IsURIStringVisited(utf8URISpec);
  return NS_OK;
}













NS_IMETHODIMP
nsNavHistory::SetPageTitle(nsIURI* aURI,
                           const nsAString& aTitle)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  
  

#ifdef LAZY_ADD
  LazyMessage message;
  nsresult rv = message.Init(LazyMessage::Type_Title, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  message.title = aTitle;
  if (aTitle.IsEmpty())
    message.title.SetIsVoid(PR_TRUE);
  return AddLazyMessage(message);
#else
  if (aTitle.IsEmpty()) {
    nsString voidString;
    voidString.SetIsVoid(PR_TRUE);
    return SetPageTitleInternal(aURI, voidString);
  }
  return SetPageTitleInternal(aURI, aTitle);
#endif
}

NS_IMETHODIMP
nsNavHistory::GetPageTitle(nsIURI* aURI, nsAString& aTitle)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  aTitle.Truncate(0);

  mozIStorageStatement *statement = DBGetURLPageInfo();
  mozStorageStatementScoper scope(statement);
  nsresult rv = BindStatementURI(statement, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);


  PRBool results;
  rv = statement->ExecuteStep(&results);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!results) {
    aTitle.SetIsVoid(PR_TRUE);
    return NS_OK; 
  }

  return statement->GetString(nsNavHistory::kGetInfoIndex_Title, aTitle);
}






NS_IMETHODIMP
nsNavHistory::GetURIGeckoFlags(nsIURI* aURI, PRUint32* aResult)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP
nsNavHistory::SetURIGeckoFlags(nsIURI* aURI, PRUint32 aFlags)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  return NS_ERROR_NOT_IMPLEMENTED;
}











PLDHashOperator nsNavHistory::ExpireNonrecentRedirects(
    nsCStringHashKey::KeyType aKey, RedirectInfo& aData, void* aUserArg)
{
  PRInt64* threshold = reinterpret_cast<PRInt64*>(aUserArg);
  if (aData.mTimeCreated < *threshold)
    return PL_DHASH_REMOVE;
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsNavHistory::AddDocumentRedirect(nsIChannel *aOldChannel,
                                  nsIChannel *aNewChannel,
                                  PRInt32 aFlags,
                                  PRBool aTopLevel)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  if (aFlags & nsIChannelEventSink::REDIRECT_INTERNAL)
    return NS_OK;

  nsresult rv;
  nsCOMPtr<nsIURI> oldURI, newURI;
  rv = aOldChannel->GetURI(getter_AddRefs(oldURI));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aNewChannel->GetURI(getter_AddRefs(newURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString oldSpec, newSpec;
  rv = oldURI->GetSpec(oldSpec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = newURI->GetSpec(newSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mRecentRedirects.Count() > RECENT_EVENT_QUEUE_MAX_LENGTH) {
    
    PRInt64 threshold = PR_Now() - RECENT_EVENT_THRESHOLD;
    mRecentRedirects.Enumerate(ExpireNonrecentRedirects,
                               reinterpret_cast<void*>(&threshold));
  }

  RedirectInfo info;

  
  if (mRecentRedirects.Get(newSpec, &info))
    mRecentRedirects.Remove(newSpec);

  
  info.mSourceURI = oldSpec;
  info.mTimeCreated = PR_Now();
  if (aFlags & nsIChannelEventSink::REDIRECT_TEMPORARY)
    info.mType = TRANSITION_REDIRECT_TEMPORARY;
  else
    info.mType = TRANSITION_REDIRECT_PERMANENT;
  mRecentRedirects.Put(newSpec, info);

  return NS_OK;
}

nsresult
nsNavHistory::OnIdle()
{
  nsresult rv;
  nsCOMPtr<nsIIdleService> idleService =
    do_GetService("@mozilla.org/widget/idleservice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 idleTime;
  rv = idleService->GetIdleTime(&idleTime);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (idleTime > EXPIRE_IDLE_TIME_IN_MSECS) {
    mozStorageTransaction transaction(mDBConn, PR_TRUE);

    PRBool keepGoing; 
    (void)mExpire.ExpireItems(MAX_EXPIRE_RECORDS_ON_IDLE / 2, &keepGoing);

    (void)mExpire.ExpireOrphans(MAX_EXPIRE_RECORDS_ON_IDLE / 2);
  }

  return NS_OK;
}

void 
nsNavHistory::IdleTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistory* history = static_cast<nsNavHistory*>(aClosure);
  (void)history->OnIdle();
}



NS_IMETHODIMP
nsNavHistory::AddDownload(nsIURI* aSource, nsIURI* aReferrer,
                          PRTime aStartTime)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  if (IsHistoryDisabled())
    return NS_OK;

  PRInt64 visitID;
  return AddVisit(aSource, aStartTime, aReferrer, TRANSITION_DOWNLOAD, PR_FALSE,
                  0, &visitID);
}



NS_IMETHODIMP
nsNavHistory::GetDBConnection(mozIStorageConnection **_DBConnection)
{
  NS_ADDREF(*_DBConnection = mDBConn);
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::FinalizeInternalStatements()
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

#ifdef LAZY_ADD
  
  
  
  
  if (mLazyTimer)
    mLazyTimer->Cancel();
  NS_ABORT_IF_FALSE(mLazyMessages.Length() == 0,
    "There are pending lazy messages, did you call CommitPendingChanges()?");
#endif

  
  nsresult rv = FinalizeStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
  rv = bookmarks->FinalizeStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAnnotationService* annosvc = nsAnnotationService::GetAnnotationService();
  NS_ENSURE_TRUE(annosvc, NS_ERROR_OUT_OF_MEMORY);
  rv = annosvc->FinalizeStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsFaviconService* iconsvc = nsFaviconService::GetFaviconService();
  NS_ENSURE_TRUE(iconsvc, NS_ERROR_OUT_OF_MEMORY);
  rv = iconsvc->FinalizeStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::CommitPendingChanges()
{
  #ifdef LAZY_ADD
    CommitLazyMessages();
  #endif

  
  
  nsCOMPtr<nsIObserverService> os =
    do_GetService("@mozilla.org/observer-service;1");
  NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);
  nsCOMPtr<nsISimpleEnumerator> e;
  nsresult rv = os->EnumerateObservers(PLACES_INIT_COMPLETE_EVENT_TOPIC,
                                       getter_AddRefs(e));
  if (NS_SUCCEEDED(rv) && e) {
    nsCOMPtr<nsIObserver> observer;
    PRBool loop = PR_TRUE;
    while(NS_SUCCEEDED(e->HasMoreElements(&loop)) && loop)
    {
      e->GetNext(getter_AddRefs(observer));
      rv = observer->Observe(observer,
                             PLACES_INIT_COMPLETE_EVENT_TOPIC,
                             nsnull);
    }
  }

  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::NotifyOnPageExpired(nsIURI *aURI, PRTime aVisitTime,
                                  PRBool aWholeEntry)
{
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                      OnPageExpired(aURI, aVisitTime, aWholeEntry));
  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::Observe(nsISupports *aSubject, const char *aTopic,
                    const PRUnichar *aData)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  if (strcmp(aTopic, gQuitApplicationGrantedMessage) == 0) {
    if (mIdleTimer) {
      mIdleTimer->Cancel();
      mIdleTimer = nsnull;
    }
    if (mAutoCompleteTimer) {
      mAutoCompleteTimer->Cancel();
      mAutoCompleteTimer = nsnull;
    }
    nsresult rv;
    nsCOMPtr<nsIPrefService> prefService =
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv))
      prefService->SavePrefFile(nsnull);

    
    mExpire.OnQuit();

    
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
    (void)bookmarks->OnQuit();
  }
  else if (strcmp(aTopic, gXpcomShutdown) == 0) {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    observerService->RemoveObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC);
    observerService->RemoveObserver(this, gIdleDaily);
    observerService->RemoveObserver(this, gAutoCompleteFeedback);
    observerService->RemoveObserver(this, gXpcomShutdown);
    observerService->RemoveObserver(this, gQuitApplicationGrantedMessage);
  }
#ifdef MOZ_XUL
  else if (strcmp(aTopic, gAutoCompleteFeedback) == 0) {
    nsCOMPtr<nsIAutoCompleteInput> input = do_QueryInterface(aSubject);
    if (!input)
      return NS_OK;

    nsCOMPtr<nsIAutoCompletePopup> popup;
    input->GetPopup(getter_AddRefs(popup));
    if (!popup)
      return NS_OK;

    nsCOMPtr<nsIAutoCompleteController> controller;
    input->GetController(getter_AddRefs(controller));
    if (!controller)
      return NS_OK;

    
    PRBool open;
    nsresult rv = popup->GetPopupOpen(&open);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!open)
      return NS_OK;

    
    PRInt32 selectedIndex;
    rv = popup->GetSelectedIndex(&selectedIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    if (selectedIndex == -1)
      return NS_OK;

    rv = AutoCompleteFeedback(selectedIndex, controller);
    NS_ENSURE_SUCCESS(rv, rv);
  }
#endif
  else if (strcmp(aTopic, "nsPref:changed") == 0) {
    PRInt32 oldDaysMin = mExpireDaysMin;
    PRInt32 oldDaysMax = mExpireDaysMax;
    PRInt32 oldVisits = mExpireSites;
    LoadPrefs(PR_FALSE);
    if (oldDaysMin != mExpireDaysMin || oldDaysMax != mExpireDaysMax ||
        oldVisits != mExpireSites)
      mExpire.OnExpirationChanged();
  }
  else if (strcmp(aTopic, gIdleDaily) == 0) {
    
    if (mFrecencyUpdateIdleTime)
      (void)RecalculateFrecencies(mNumCalculateFrecencyOnIdle, PR_TRUE);

    if (mDBConn) {
      
      
      
      
      
      nsCOMPtr<mozIStorageStatement> decayFrecency;
      nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "UPDATE moz_places SET frecency = ROUND(frecency * .975) "
        "WHERE frecency > 0"),
        getter_AddRefs(decayFrecency));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to create decayFrecency");

      
      
      nsCOMPtr<mozIStorageStatement> decayAdaptive;
      rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "UPDATE moz_inputhistory SET use_count = use_count * .975"),
        getter_AddRefs(decayAdaptive));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to create decayAdaptive");

      
      nsCOMPtr<mozIStorageStatement> deleteAdaptive;
      rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "DELETE FROM moz_inputhistory WHERE use_count < .01"),
        getter_AddRefs(deleteAdaptive));
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to create deleteAdaptive");

      
      if (decayFrecency && decayAdaptive && deleteAdaptive) {
        nsCOMPtr<mozIStoragePendingStatement> ps;
        mozIStorageStatement *stmts[] = {
          decayFrecency,
          decayAdaptive,
          deleteAdaptive
        };

        rv = mDBConn->ExecuteAsync(stmts, NS_ARRAY_LENGTH(stmts), nsnull,
                                    getter_AddRefs(ps));
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to exec async idle stmts");
      }
    }
  }
  else if (strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC) == 0) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(aData)) {
      mInPrivateBrowsing = PR_TRUE;
    } else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData)) {
      mInPrivateBrowsing = PR_FALSE;
    }
  }
  else if (strcmp(aTopic, PLACES_INIT_COMPLETE_EVENT_TOPIC) == 0) {
    nsCOMPtr<nsIObserverService> os =
      do_GetService("@mozilla.org/observer-service;1");
    NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);
    (void)os->RemoveObserver(this, PLACES_INIT_COMPLETE_EVENT_TOPIC);

    
    
    
    (void)RecalculateFrecencies(mNumCalculateFrecencyOnMigrate,
                                PR_FALSE );
  }

  return NS_OK;
}




#ifdef LAZY_ADD



nsresult
nsNavHistory::AddLazyLoadFaviconMessage(nsIURI* aPage, nsIURI* aFavicon,
                                        PRBool aForceReload)
{
  LazyMessage message;
  nsresult rv = message.Init(LazyMessage::Type_Favicon, aPage);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aFavicon->Clone(getter_AddRefs(message.favicon));
  NS_ENSURE_SUCCESS(rv, rv);
  message.alwaysLoadFavicon = aForceReload;
  return AddLazyMessage(message);
}












nsresult
nsNavHistory::StartLazyTimer()
{
  if (! mLazyTimer) {
    mLazyTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (! mLazyTimer)
      return NS_ERROR_OUT_OF_MEMORY;
  } else {
    if (mLazyTimerSet) {
      if (mLazyTimerDeferments >= MAX_LAZY_TIMER_DEFERMENTS) {
        
        return NS_OK;
      } else {
        
        mLazyTimer->Cancel();
        mLazyTimerDeferments ++;
      }
    }
  }
  nsresult rv = mLazyTimer->InitWithFuncCallback(LazyTimerCallback, this,
                                                 LAZY_MESSAGE_TIMEOUT,
                                                 nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);
  mLazyTimerSet = PR_TRUE;
  return NS_OK;
}




nsresult
nsNavHistory::AddLazyMessage(const LazyMessage& aMessage)
{
  if (! mLazyMessages.AppendElement(aMessage))
    return NS_ERROR_OUT_OF_MEMORY;
  return StartLazyTimer();
}




void 
nsNavHistory::LazyTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistory* that = static_cast<nsNavHistory*>(aClosure);
  that->mLazyTimerSet = PR_FALSE;
  that->mLazyTimerDeferments = 0;
  that->CommitLazyMessages();
}



void
nsNavHistory::CommitLazyMessages()
{
  mozStorageTransaction transaction(mDBConn, PR_TRUE);
  for (PRUint32 i = 0; i < mLazyMessages.Length(); i ++) {
    LazyMessage& message = mLazyMessages[i];
    switch (message.type) {
      case LazyMessage::Type_AddURI:
        AddURIInternal(message.uri, message.time, message.isRedirect,
                       message.isToplevel, message.referrer);
        break;
      case LazyMessage::Type_Title:
        SetPageTitleInternal(message.uri, message.title);
        break;
      case LazyMessage::Type_Favicon: {
        nsFaviconService* faviconService = nsFaviconService::GetFaviconService();
        if (faviconService) {
          faviconService->DoSetAndLoadFaviconForPage(message.uri,
                                                     message.favicon,
                                                     message.alwaysLoadFavicon);
        }
        break;
      }
      default:
        NS_NOTREACHED("Invalid lazy message type");
    }
  }
  mLazyMessages.Clear();
}
#endif 











class ConditionBuilder
{
public:

  ConditionBuilder(PRInt32 aQueryIndex): mQueryIndex(aQueryIndex)
  { }

  ConditionBuilder& Condition(const char* aStr)
  {
    if (!mClause.IsEmpty())
      mClause.AppendLiteral(" AND ");
    Str(aStr);
    return *this;
  }

  ConditionBuilder& Str(const char* aStr)
  {
    mClause.Append(' ');
    mClause.Append(aStr);
    mClause.Append(' ');
    return *this;
  }

  ConditionBuilder& Param(const char* aParam)
  {
    mClause.Append(' ');
    if (!mQueryIndex)
      mClause.Append(aParam);
    else
      mClause += nsPrintfCString("%s%d", aParam, mQueryIndex);

    mClause.Append(' ');
    return *this;
  }

  void GetClauseString(nsCString& aResult) 
  {
    aResult = mClause;
  }

private:

  PRInt32 mQueryIndex;
  nsCString mClause;
};









nsresult
nsNavHistory::QueryToSelectClause(nsNavHistoryQuery* aQuery, 
                                  nsNavHistoryQueryOptions* aOptions,
                                  PRInt32 aQueryIndex,
                                  nsCString* aClause)
{
  PRBool hasIt;

  ConditionBuilder clause(aQueryIndex);

  
  if (NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) 
    clause.Condition("v.visit_date >=").Param(":begin_time");

  
  if (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt)
    clause.Condition("v.visit_date <=").Param(":end_time");

  

  
  if (aQuery->MinVisits() >= 0)
    clause.Condition("h.visit_count >=").Param(":min_visits");

  if (aQuery->MaxVisits() >= 0)
    clause.Condition("h.visit_count <=").Param(":max_visits");
  
  
  if (aOptions->QueryType() != nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS &&
      aQuery->OnlyBookmarked())
    clause.Condition("EXISTS (SELECT b.fk FROM moz_bookmarks b WHERE b.type = ")
          .Str(nsPrintfCString("%d", nsNavBookmarks::TYPE_BOOKMARK).get())
          .Str("AND b.fk = h.id)");

  
  if (NS_SUCCEEDED(aQuery->GetHasDomain(&hasIt)) && hasIt) {
    PRBool domainIsHost = PR_FALSE;
    aQuery->GetDomainIsHost(&domainIsHost);
    if (domainIsHost)
      clause.Condition("h.rev_host =").Param(":domain_lower");
    else
      
      clause.Condition("h.rev_host >=").Param(":domain_lower")
            .Condition("h.rev_host <").Param(":domain_upper");
  }

  
  
  
  
  
  
  
  if (NS_SUCCEEDED(aQuery->GetHasUri(&hasIt)) && hasIt) {
    if (aQuery->UriIsPrefix())
      clause.Condition("SUBSTR(h.url, 0, LENGTH(").Param(":uri").Str(")) =")
            .Param(":uri");
    else
      clause.Condition("h.url =").Param(":uri");
  }

  
  aQuery->GetHasAnnotation(&hasIt);
  if (hasIt) {
    clause.Condition("");
    if (aQuery->AnnotationIsNot())
      clause.Str("NOT");
    clause.Str(
      "EXISTS "
        "(SELECT h.id "
         "FROM moz_annos anno "
         "JOIN moz_anno_attributes annoname "
           "ON anno.anno_attribute_id = annoname.id "
         "WHERE anno.place_id = h.id "
           "AND annoname.name = ").Param(":anno").Str(")");
    
    
  }

  
  
  if (aOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS &&
      aQuery->Folders().Length() == 1) {
    clause.Condition("b.parent =").Param(":parent");
  }

  clause.GetClauseString(*aClause);
  return NS_OK;
}








class IndexGetter
{
public:
  IndexGetter(PRInt32 aQueryIndex, mozIStorageStatement* aStatement) : 
    mQueryIndex(aQueryIndex), mStatement(aStatement)
  {
    mResult = NS_OK;
  }

  PRUint32 For(const char* aName) 
  {
    PRUint32 index;

    
    if (NS_SUCCEEDED(mResult)) {
      if (!mQueryIndex)
        mResult = mStatement->GetParameterIndex(nsCAutoString(aName), &index);
      else
        mResult = mStatement->GetParameterIndex(
                      nsPrintfCString("%s%d", aName, mQueryIndex), &index);
    }

    if (NS_SUCCEEDED(mResult))
      return index;

    return -1; 
  }

  nsresult Result() 
  {
    return mResult;
  }

private:
  PRInt32 mQueryIndex;
  mozIStorageStatement* mStatement;
  nsresult mResult;
};





nsresult
nsNavHistory::BindQueryClauseParameters(mozIStorageStatement* statement,
                                        PRInt32 aQueryIndex,
                                        nsNavHistoryQuery* aQuery, 
                                        nsNavHistoryQueryOptions* aOptions)
{
  nsresult rv;

  PRBool hasIt;
  IndexGetter index(aQueryIndex, statement);

  
  if (NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) {
    PRTime time = NormalizeTime(aQuery->BeginTimeReference(),
                                aQuery->BeginTime());
    rv = statement->BindInt64Parameter(index.For(":begin_time"), time);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt) {
    PRTime time = NormalizeTime(aQuery->EndTimeReference(),
                                aQuery->EndTime());
    rv = statement->BindInt64Parameter(index.For(":end_time"), time);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  

  
  PRInt32 visits = aQuery->MinVisits();
  if (visits >= 0) {
    rv = statement->BindInt32Parameter(index.For(":min_visits"), visits);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  visits = aQuery->MaxVisits();
  if (visits >= 0) {
    rv = statement->BindInt32Parameter(index.For(":max_visits"), visits);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasDomain(&hasIt)) && hasIt) {
    nsString revDomain;
    GetReversedHostname(NS_ConvertUTF8toUTF16(aQuery->Domain()), revDomain);

    if (aQuery->DomainIsHost()) {
      rv = statement->BindStringParameter(index.For(":domain_lower"), revDomain);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      
      
      
      NS_ASSERTION(revDomain[revDomain.Length() - 1] == '.', "Invalid rev. host");
      rv = statement->BindStringParameter(index.For(":domain_lower"), revDomain);
      NS_ENSURE_SUCCESS(rv, rv);
      revDomain.Truncate(revDomain.Length() - 1);
      revDomain.Append(PRUnichar('/'));
      rv = statement->BindStringParameter(index.For(":domain_upper"), revDomain);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasUri(&hasIt)) && hasIt)
    BindStatementURI(statement, index.For(":uri"), aQuery->Uri());

  
  if (NS_SUCCEEDED(aQuery->GetHasAnnotation(&hasIt)) && hasIt) {
    rv = statement->BindUTF8StringParameter(index.For(":anno"), 
                                            aQuery->Annotation());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (aOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS &&
      aQuery->Folders().Length() == 1) {
    rv = statement->BindInt64Parameter(index.For(":parent"),
                                       aQuery->Folders()[0]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ENSURE_SUCCESS(index.Result(), index.Result());

  return NS_OK;
}





nsresult
nsNavHistory::ResultsAsList(mozIStorageStatement* statement,
                            nsNavHistoryQueryOptions* aOptions,
                            nsCOMArray<nsNavHistoryResultNode>* aResults)
{
  nsresult rv;
  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(statement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
    nsRefPtr<nsNavHistoryResultNode> result;
    rv = RowToResult(row, aOptions, getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);
    aResults->AppendObject(result);
  }
  return NS_OK;
}

static PRInt64
GetAgeInDays(PRTime aNormalizedNow, PRTime aDate)
{
  PRTime dateMidnight = NormalizeTimeRelativeToday(aDate);
  
  
  if (dateMidnight > aNormalizedNow)
    return 0;
  else
    return ((aNormalizedNow - dateMidnight) / USECS_PER_DAY);
}

const PRInt64 UNDEFINED_URN_VALUE = -1;




nsresult
CreatePlacesPersistURN(nsNavHistoryQueryResultNode *aResultNode, 
                      PRInt64 aValue, const nsCString& aTitle, nsCString& aURN)
{
  nsCAutoString uri;
  nsresult rv = aResultNode->GetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);

  aURN.Assign(NS_LITERAL_CSTRING("urn:places-persist:"));
  aURN.Append(uri);

  aURN.Append(NS_LITERAL_CSTRING(","));
  if (aValue != UNDEFINED_URN_VALUE)
    aURN.AppendInt(aValue);

  aURN.Append(NS_LITERAL_CSTRING(","));
  if (!aTitle.IsEmpty()) {
    nsCAutoString escapedTitle;
    PRBool success = NS_Escape(aTitle, escapedTitle, url_XAlphas);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    aURN.Append(escapedTitle);
  }

  return NS_OK;
}

PRInt64
nsNavHistory::GetTagsFolder()
{
  
  
  
  if (mTagsFolder == -1) {
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, -1);
    
    nsresult rv = bookmarks->GetTagsFolder(&mTagsFolder);
    NS_ENSURE_SUCCESS(rv, -1);
  }
  return mTagsFolder;
}














nsresult
nsNavHistory::FilterResultSet(nsNavHistoryQueryResultNode* aQueryNode,
                              const nsCOMArray<nsNavHistoryResultNode>& aSet,
                              nsCOMArray<nsNavHistoryResultNode>* aFiltered,
                              const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions *aOptions)
{
  nsresult rv;

  
  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  
  nsTArray<nsTArray<nsString>*> terms;
  ParseSearchTermsFromQueries(aQueries, &terms);

  PRInt32 queryIndex;
  PRUint16 resultType = aOptions->ResultType();

  
  
  nsTArray< nsTArray<PRInt64>* > includeFolders;
  nsTArray< nsTArray<PRInt64>* > excludeFolders;
  for (queryIndex = 0;
       queryIndex < aQueries.Count(); queryIndex++) {
    includeFolders.AppendElement(new nsTArray<PRInt64>(aQueries[queryIndex]->Folders()));
    excludeFolders.AppendElement(new nsTArray<PRInt64>());
  }

  
  
  PRBool excludeQueries = PR_FALSE;
  if (aQueryNode) {
    rv = aQueryNode->mOptions->GetExcludeQueries(&excludeQueries);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCString parentAnnotationToExclude;
  nsTArray<PRInt64> parentFoldersToExclude;
  if (aQueryNode) {
    rv = aQueryNode->mOptions->GetExcludeItemIfParentHasAnnotation(parentAnnotationToExclude);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!parentAnnotationToExclude.IsEmpty()) {
    
    
    
    
    mozStorageStatementScoper scope(mFoldersWithAnnotationQuery);

    rv = mFoldersWithAnnotationQuery->BindUTF8StringParameter(0, parentAnnotationToExclude);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore = PR_FALSE;
    while (NS_SUCCEEDED(mFoldersWithAnnotationQuery->ExecuteStep(&hasMore)) && hasMore) {
      PRInt64 folderId = 0;
      rv = mFoldersWithAnnotationQuery->GetInt64(0, &folderId);
      NS_ENSURE_SUCCESS(rv, rv);
      parentFoldersToExclude.AppendElement(folderId);
    }
  }

  for (PRInt32 nodeIndex = 0; nodeIndex < aSet.Count(); nodeIndex ++) {
    
    
    if (!aSet[nodeIndex]->IsURI())
      continue;

    PRInt64 parentId = -1;
    if (aSet[nodeIndex]->mItemId != -1) {
      if (aQueryNode->mItemId == aSet[nodeIndex]->mItemId)
        continue;
      rv = bookmarks->GetFolderIdForItem(aSet[nodeIndex]->mItemId, &parentId);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    if (!parentAnnotationToExclude.IsEmpty() && parentFoldersToExclude.Contains(parentId))
      continue;

    
    PRBool appendNode = PR_FALSE;
    for (queryIndex = 0;
         queryIndex < aQueries.Count() && !appendNode; queryIndex++) {
      
      
      
      if (includeFolders[queryIndex]->Length() != 0 &&
          resultType != nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS) {
        
        if (aSet[nodeIndex]->mItemId == -1)
          continue;

        
        
        if (excludeFolders[queryIndex]->Contains(parentId))
          continue;

        if (!includeFolders[queryIndex]->Contains(parentId)) {
          
          PRInt64 ancestor = parentId, lastAncestor;
          PRBool belongs = PR_FALSE;
          nsTArray<PRInt64> ancestorFolders;

          while (!belongs) {
            
            lastAncestor = ancestor;
            ancestorFolders.AppendElement(ancestor);

            
            if (NS_FAILED(bookmarks->GetFolderIdForItem(ancestor,&ancestor))) {
              break;
            } else if (excludeFolders[queryIndex]->Contains(ancestor)) {
              break;
            } else if (includeFolders[queryIndex]->Contains(ancestor)) {
              belongs = PR_TRUE;
            }
          }
          
          
          if (belongs) {
            includeFolders[queryIndex]->AppendElements(ancestorFolders);
          } else {
            excludeFolders[queryIndex]->AppendElements(ancestorFolders);
            continue;
          }
        }
      }

      
      NS_ConvertUTF8toUTF16 nodeTitle(aSet[nodeIndex]->mTitle);
      
      nsCAutoString cNodeURL(aSet[nodeIndex]->mURI);
      NS_ConvertUTF8toUTF16 nodeURL(NS_UnescapeURL(cNodeURL));

      
      mozStorageStatementScoper scoper(mDBGetTags);
      rv = mDBGetTags->BindStringParameter(0, NS_LITERAL_STRING(" "));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mDBGetTags->BindInt64Parameter(1, GetTagsFolder());
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mDBGetTags->BindUTF8StringParameter(2, aSet[nodeIndex]->mURI);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString nodeTags;
      PRBool hasTag = PR_FALSE;
      if (NS_SUCCEEDED(mDBGetTags->ExecuteStep(&hasTag)) && hasTag) {
        rv = mDBGetTags->GetString(0, nodeTags);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      PRBool matchAll = PR_TRUE;
      for (PRInt32 termIndex = terms[queryIndex]->Length(); --termIndex >= 0 &&
           matchAll; ) {
        const nsString& term = terms[queryIndex]->ElementAt(termIndex);

        
        matchAll = CaseInsensitiveFindInReadable(term, nodeTitle) ||
                   CaseInsensitiveFindInReadable(term, nodeURL) ||
                   CaseInsensitiveFindInReadable(term, nodeTags);
      }

      
      if (!matchAll)
        continue;

      appendNode = PR_TRUE;
    }

    
    
    
    if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS &&
        nodeIndex > 0 && aSet[nodeIndex]->mURI == aSet[nodeIndex-1]->mURI)
      continue;

    if (appendNode)
      aFiltered->AppendObject(aSet[nodeIndex]);
      
    
    if (aOptions->MaxResults() > 0 && 
        (PRUint32)aFiltered->Count() >= aOptions->MaxResults())
      break;
  }

  
  for (PRInt32 i = 0; i < aQueries.Count(); i++) {
    delete terms[i];
    delete includeFolders[i];
    delete excludeFolders[i];
  }

  return NS_OK;
}










PRBool
nsNavHistory::CheckIsRecentEvent(RecentEventHash* hashTable,
                                 const nsACString& url)
{
  PRTime eventTime;
  if (hashTable->Get(url, &eventTime)) {
    hashTable->Remove(url);
    if (eventTime > GetNow() - RECENT_EVENT_THRESHOLD)
      return PR_TRUE;
    return PR_FALSE;
  }
  return PR_FALSE;
}






static PLDHashOperator
ExpireNonrecentEventsCallback(nsCStringHashKey::KeyType aKey,
                              PRInt64& aData,
                              void* userArg)
{
  PRInt64* threshold = reinterpret_cast<PRInt64*>(userArg);
  if (aData < *threshold)
    return PL_DHASH_REMOVE;
  return PL_DHASH_NEXT;
}
void
nsNavHistory::ExpireNonrecentEvents(RecentEventHash* hashTable)
{
  PRInt64 threshold = GetNow() - RECENT_EVENT_THRESHOLD;
  hashTable->Enumerate(ExpireNonrecentEventsCallback,
                       reinterpret_cast<void*>(&threshold));
}




















































PRBool
nsNavHistory::GetRedirectFor(const nsACString& aDestination,
                             nsACString& aSource, PRTime* aTime,
                             PRUint32* aRedirectType)
{
  RedirectInfo info;
  if (mRecentRedirects.Get(aDestination, &info)) {
    mRecentRedirects.Remove(aDestination);
    if (info.mTimeCreated < GetNow() - RECENT_EVENT_THRESHOLD)
      return PR_FALSE; 
    aSource = info.mSourceURI;
    *aTime = info.mTimeCreated;
    *aRedirectType = info.mType;
    return PR_TRUE;
  }
  return PR_FALSE;
}







nsresult
nsNavHistory::RowToResult(mozIStorageValueArray* aRow,
                          nsNavHistoryQueryOptions* aOptions,
                          nsNavHistoryResultNode** aResult)
{
  *aResult = nsnull;
  NS_ASSERTION(aRow && aOptions && aResult, "Null pointer in RowToResult");

  
  nsCAutoString url;
  nsresult rv = aRow->GetUTF8String(kGetInfoIndex_URL, url);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCAutoString title;
  rv = aRow->GetUTF8String(kGetInfoIndex_Title, title);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 accessCount = aRow->AsInt32(kGetInfoIndex_VisitCount);
  PRTime time = aRow->AsInt64(kGetInfoIndex_VisitDate);

  
  nsCAutoString favicon;
  rv = aRow->GetUTF8String(kGetInfoIndex_FaviconURL, favicon);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 itemId = -1;
  PRBool isNull;
  rv = aRow->GetIsNull(kGetInfoIndex_ItemId, &isNull);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isNull) {
    itemId = aRow->AsInt64(kGetInfoIndex_ItemId);
  }

  if (IsQueryURI(url)) {
    
      
    
    
    
    
    
    if (itemId != -1) {
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

      rv = bookmarks->GetItemTitle(itemId, title);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = QueryRowToResult(itemId, url, title, accessCount, time, favicon, aResult);

    
    
    
    if (*aResult && (*aResult)->IsFolder() &&
         aOptions->ResultType() != 
           nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY)
      (*aResult)->GetAsContainer()->mOptions = aOptions;

    
    if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_TAG_QUERY) {
      (*aResult)->mDateAdded = aRow->AsInt64(kGetInfoIndex_ItemDateAdded);
      (*aResult)->mLastModified = aRow->AsInt64(kGetInfoIndex_ItemLastModified);
    }

    return rv;
  } else if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_URI ||
             aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS) {
    *aResult = new nsNavHistoryResultNode(url, title, accessCount, time,
                                          favicon);
    if (!*aResult)
      return NS_ERROR_OUT_OF_MEMORY;

    if (itemId != -1) {
      (*aResult)->mItemId = itemId;
      (*aResult)->mDateAdded = aRow->AsInt64(kGetInfoIndex_ItemDateAdded);
      (*aResult)->mLastModified = aRow->AsInt64(kGetInfoIndex_ItemLastModified);
    }
    NS_ADDREF(*aResult);
    return NS_OK;
  }
  

  
  PRInt64 session = aRow->AsInt64(kGetInfoIndex_SessionId);

  if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_VISIT) {
    *aResult = new nsNavHistoryVisitResultNode(url, title, accessCount, time,
                                               favicon, session);
    if (! *aResult)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aResult);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}







nsresult
nsNavHistory::QueryRowToResult(PRInt64 itemId, const nsACString& aURI,
                               const nsACString& aTitle,
                               PRUint32 aAccessCount, PRTime aTime,
                               const nsACString& aFavicon,
                               nsNavHistoryResultNode** aNode)
{
  nsCOMArray<nsNavHistoryQuery> queries;
  nsCOMPtr<nsNavHistoryQueryOptions> options;
  nsresult rv = QueryStringToQueryArray(aURI, &queries,
                                        getter_AddRefs(options));
  if (NS_FAILED(rv)) {
    
    
    
    
    
    *aNode = new nsNavHistoryQueryResultNode(aURI, aTitle, aFavicon);
    if (! *aNode)
      return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(*aNode);
  } else {
    PRInt64 folderId = GetSimpleBookmarksQueryFolder(queries, options);
    if (folderId) {
      
      nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

      
      rv = bookmarks->ResultNodeForContainer(folderId, options, aNode);
      NS_ENSURE_SUCCESS(rv, rv);

      
      (*aNode)->GetAsFolder()->mQueryItemId = itemId;

      
      
      if (!aTitle.IsVoid())
        (*aNode)->mTitle = aTitle;
    } else {
      
      *aNode = new nsNavHistoryQueryResultNode(aTitle, EmptyCString(), aTime,
                                               queries, options);
      if (! *aNode)
        return NS_ERROR_OUT_OF_MEMORY;
      (*aNode)->mItemId = itemId;
      NS_ADDREF(*aNode);
    }
  }
  return NS_OK;
}







nsresult
nsNavHistory::VisitIdToResultNode(PRInt64 visitId,
                                  nsNavHistoryQueryOptions* aOptions,
                                  nsNavHistoryResultNode** aResult)
{
  mozIStorageStatement* statement; 

  switch (aOptions->ResultType())
  {
    case nsNavHistoryQueryOptions::RESULTS_AS_VISIT:
    case nsNavHistoryQueryOptions::RESULTS_AS_FULL_VISIT:
      
      statement = GetDBVisitToVisitResult();
      break;

    case nsNavHistoryQueryOptions::RESULTS_AS_URI:
      
      statement = GetDBVisitToURLResult();
      break;

    default:
      
      
      return NS_OK;
  }
  NS_ENSURE_TRUE(statement, NS_ERROR_UNEXPECTED);

  mozStorageStatementScoper scoper(statement);
  nsresult rv = statement->BindInt64Parameter(0, visitId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = statement->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid visit");
    return NS_ERROR_INVALID_ARG;
  }

  return RowToResult(statement, aOptions, aResult);
}

nsresult
nsNavHistory::BookmarkIdToResultNode(PRInt64 aBookmarkId, nsNavHistoryQueryOptions* aOptions,
                                     nsNavHistoryResultNode** aResult)
{
  mozIStorageStatement *stmt = GetDBBookmarkToUrlResult();
  NS_ENSURE_TRUE(stmt, NS_ERROR_UNEXPECTED);
  mozStorageStatementScoper scoper(stmt);
  nsresult rv = stmt->BindInt64Parameter(0, aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = stmt->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid bookmark identifier");
    return NS_ERROR_INVALID_ARG;
  }

  return RowToResult(stmt, aOptions, aResult);
}







void
nsNavHistory::TitleForDomain(const nsCString& domain, nsACString& aTitle)
{
  if (! domain.IsEmpty()) {
    aTitle = domain;
    return;
  }

  
  GetStringFromName(NS_LITERAL_STRING("localhost").get(), aTitle);
}

void
nsNavHistory::GetAgeInDaysString(PRInt32 aInt, const PRUnichar *aName,
                                 nsACString& aResult)
{
  nsIStringBundle *bundle = GetBundle();
  if (!bundle)
    aResult.Truncate(0);
  else {
    nsAutoString intString;
    intString.AppendInt(aInt);
    const PRUnichar* strings[1] = { intString.get() };
    nsXPIDLString value;
    nsresult rv = bundle->FormatStringFromName(aName, strings,
                                               1, getter_Copies(value));
    if (NS_SUCCEEDED(rv))
      CopyUTF16toUTF8(value, aResult);
    else
      aResult.Truncate(0);
  }
}

void
nsNavHistory::GetStringFromName(const PRUnichar *aName, nsACString& aResult)
{
  nsIStringBundle *bundle = GetBundle();
  if (!bundle)
    aResult.Truncate(0);

  nsXPIDLString value;
  nsresult rv = bundle->GetStringFromName(aName, getter_Copies(value));
  if (NS_SUCCEEDED(rv))
    CopyUTF16toUTF8(value, aResult);
  else
    aResult.Truncate(0);
}

void
nsNavHistory::GetMonthName(PRInt32 aIndex, nsACString& aResult)
{
  nsIStringBundle *bundle = GetDateFormatBundle();
  if (!bundle)
    aResult.Truncate(0);
  else {
    nsCString name = nsPrintfCString("month.%d.name", aIndex);
    nsXPIDLString value;
    nsresult rv = bundle->GetStringFromName(NS_ConvertUTF8toUTF16(name).get(),
                                            getter_Copies(value));
    if (NS_SUCCEEDED(rv))
      CopyUTF16toUTF8(value, aResult);
    else
      aResult.Truncate(0);
  }
}










nsresult
nsNavHistory::SetPageTitleInternal(nsIURI* aURI, const nsAString& aTitle)
{
  nsresult rv;

  
  
  nsAutoString title;
  { 
    mozStorageStatementScoper infoScoper(mDBGetURLPageInfo);
    rv = BindStatementURI(mDBGetURLPageInfo, 0, aURI);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hasURL = PR_FALSE;
    rv = mDBGetURLPageInfo->ExecuteStep(&hasURL);
    NS_ENSURE_SUCCESS(rv, rv);
    if (! hasURL) {
      
      return NS_ERROR_NOT_AVAILABLE;
    }

    
    rv = mDBGetURLPageInfo->GetString(kGetInfoIndex_Title, title);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  if ((aTitle.IsVoid() && title.IsVoid()) || aTitle == title)
    return NS_OK;

  mozStorageStatementScoper scoper(mDBSetPlaceTitle);
  
  if (aTitle.IsVoid())
    rv = mDBSetPlaceTitle->BindNullParameter(0);
  else
    rv = mDBSetPlaceTitle->BindStringParameter(0, StringHead(aTitle, HISTORY_TITLE_LENGTH_MAX));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = BindStatementURI(mDBSetPlaceTitle, 1, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBSetPlaceTitle->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                      OnTitleChanged(aURI, aTitle))

  return NS_OK;

}

nsresult
nsNavHistory::AddPageWithVisits(nsIURI *aURI,
                                const nsString &aTitle,
                                PRInt32 aVisitCount,
                                PRInt32 aTransitionType,
                                PRTime aFirstVisitDate,
                                PRTime aLastVisitDate)
{
  PRBool canAdd = PR_FALSE;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  
  mozStorageStatementScoper scoper(mDBGetPageVisitStats);
  rv = BindStatementURI(mDBGetPageVisitStats, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool alreadyVisited = PR_FALSE;
  rv = mDBGetPageVisitStats->ExecuteStep(&alreadyVisited);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 placeId = 0;
  PRInt32 typed = 0;
  PRInt32 hidden = 0;

  if (alreadyVisited) {
    
    rv = mDBGetPageVisitStats->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mDBGetPageVisitStats->GetInt32(2, &typed);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBGetPageVisitStats->GetInt32(3, &hidden);
    NS_ENSURE_SUCCESS(rv, rv);

    if (typed == 0 && aTransitionType == TRANSITION_TYPED) {
      typed = 1;
      
      mozStorageStatementScoper updateScoper(mDBUpdatePageVisitStats);
      rv = mDBUpdatePageVisitStats->BindInt64Parameter(0, placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mDBUpdatePageVisitStats->BindInt32Parameter(1, hidden);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mDBUpdatePageVisitStats->BindInt32Parameter(2, typed);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mDBUpdatePageVisitStats->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    
    rv = InternalAddNewPage(aURI, aTitle, hidden == 1,
                            aTransitionType == TRANSITION_TYPED, 0,
                            PR_FALSE, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(placeId != 0, "Cannot add a visit to a not existant page");

  if (aFirstVisitDate != -1) {
    
    PRInt64 visitId;
    rv = InternalAddVisit(placeId, 0, 0,
                          aFirstVisitDate, aTransitionType, &visitId);
    aVisitCount--;
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aLastVisitDate != -1) {
   
   for (PRInt64 i = 0; i < aVisitCount; i++) {
      PRInt64 visitId;
      rv = InternalAddVisit(placeId, 0, 0,
                            aLastVisitDate - i, aTransitionType, &visitId);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
nsNavHistory::RemoveDuplicateURIs()
{
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  
  
  nsCOMPtr<mozIStorageStatement> selectStatement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT "
        "(SELECT h.id FROM moz_places h WHERE h.url = url "
         "ORDER BY h.visit_count DESC LIMIT 1), "
        "url, SUM(visit_count) "
      "FROM moz_places "
      "GROUP BY url HAVING( COUNT(url) > 1)"),
    getter_AddRefs(selectStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> updateStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_historyvisits "
      "SET place_id = ?1 "
      "WHERE place_id IN "
        "(SELECT id FROM moz_places WHERE id <> ?1 AND url = ?2)"),
    getter_AddRefs(updateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> bookmarkStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks "
      "SET fk = ?1 "
      "WHERE fk IN "
        "(SELECT id FROM moz_places WHERE id <> ?1 AND url = ?2)"),
    getter_AddRefs(bookmarkStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> annoStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_annos "
      "SET place_id = ?1 "
      "WHERE place_id IN "
        "(SELECT id FROM moz_places WHERE id <> ?1 AND url = ?2)"),
    getter_AddRefs(annoStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  nsCOMPtr<mozIStorageStatement> deleteStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_places WHERE url = ?1 AND id <> ?2"),
    getter_AddRefs(deleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> countStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places SET visit_count = ?1 WHERE id = ?2"),
    getter_AddRefs(countStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool hasMore;
  while (NS_SUCCEEDED(selectStatement->ExecuteStep(&hasMore)) && hasMore) {
    PRUint64 id = selectStatement->AsInt64(0);
    nsCAutoString url;
    rv = selectStatement->GetUTF8String(1, url);
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint64 visit_count = selectStatement->AsInt64(2);

    
    rv = updateStatement->BindInt64Parameter(0, id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStatement->BindUTF8StringParameter(1, url);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStatement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = bookmarkStatement->BindInt64Parameter(0, id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = bookmarkStatement->BindUTF8StringParameter(1, url);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = bookmarkStatement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = annoStatement->BindInt64Parameter(0, id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = annoStatement->BindUTF8StringParameter(1, url);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = annoStatement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    rv = deleteStatement->BindUTF8StringParameter(0, url);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = deleteStatement->BindInt64Parameter(1, id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = deleteStatement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = countStatement->BindInt64Parameter(0, visit_count);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = countStatement->BindInt64Parameter(1, id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = countStatement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}






















nsresult
GetReversedHostname(nsIURI* aURI, nsAString& aRevHost)
{
  nsCString forward8;
  nsresult rv = aURI->GetHost(forward8);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  NS_ConvertUTF8toUTF16 forward(forward8);
  GetReversedHostname(forward, aRevHost);
  return NS_OK;
}






void
GetReversedHostname(const nsString& aForward, nsAString& aRevHost)
{
  ReverseString(aForward, aRevHost);
  aRevHost.Append(PRUnichar('.'));
}












static PRInt64
GetSimpleBookmarksQueryFolder(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                              nsNavHistoryQueryOptions* aOptions)
{
  if (aQueries.Count() != 1)
    return 0;

  nsNavHistoryQuery* query = aQueries[0];
  if (query->Folders().Length() != 1)
    return 0;

  PRBool hasIt;
  query->GetHasBeginTime(&hasIt);
  if (hasIt)
    return 0;
  query->GetHasEndTime(&hasIt);
  if (hasIt)
    return 0;
  query->GetHasDomain(&hasIt);
  if (hasIt)
    return 0;
  query->GetHasUri(&hasIt);
  if (hasIt)
    return 0;
  (void)query->GetHasSearchTerms(&hasIt);
  if (hasIt)
    return 0;
  if (aOptions->MaxResults() > 0)
    return 0;

  
  
  if(aOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS)
    return 0;

  
  
  NS_ASSERTION(query->Folders()[0] > 0, "bad folder id");
  return query->Folders()[0];
}













inline PRBool isQueryWhitespace(PRUnichar ch)
{
  return ch == ' ';
}

void ParseSearchTermsFromQueries(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                 nsTArray<nsTArray<nsString>*>* aTerms)
{
  PRInt32 lastBegin = -1;
  for (PRInt32 i = 0; i < aQueries.Count(); i++) {
    nsTArray<nsString> *queryTerms = new nsTArray<nsString>();
    PRBool hasSearchTerms;
    if (NS_SUCCEEDED(aQueries[i]->GetHasSearchTerms(&hasSearchTerms)) &&
        hasSearchTerms) {
      const nsString& searchTerms = aQueries[i]->SearchTerms();
      for (PRUint32 j = 0; j < searchTerms.Length(); j++) {
        if (isQueryWhitespace(searchTerms[j]) ||
            searchTerms[j] == '"') {
          if (lastBegin >= 0) {
            
            queryTerms->AppendElement(Substring(searchTerms, lastBegin,
                                               j - lastBegin));
            lastBegin = -1;
          }
        } else {
          if (lastBegin < 0) {
            
            lastBegin = j;
          }
        }
      }
      
      if (lastBegin >= 0)
        queryTerms->AppendElement(Substring(searchTerms, lastBegin));
    }
    aTerms->AppendElement(queryTerms);
  }
}








nsresult 
GenerateTitleFromURI(nsIURI* aURI, nsAString& aTitle)
{
  nsCAutoString name;
  nsCOMPtr<nsIURL> url(do_QueryInterface(aURI));
  if (url)
    url->GetFileName(name);
  if (name.IsEmpty()) {
    
    nsresult rv = aURI->GetPath(name);
    if (NS_FAILED(rv) || (name.Length() == 1 && name[0] == '/')) {
      
      rv = aURI->GetHost(name);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  CopyUTF8toUTF16(name, aTitle);
  return NS_OK;
}







nsresult BindStatementURI(mozIStorageStatement* statement, PRInt32 index,
                          nsIURI* aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCAutoString utf8URISpec;
  nsresult rv = aURI->GetSpec(utf8URISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->BindUTF8StringParameter(index,
      StringHead(utf8URISpec, HISTORY_URI_LENGTH_MAX));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsNavHistory::UpdateFrecency(PRInt64 aPlaceId, PRBool aIsBookmarked)
{
  mozStorageStatementScoper statsScoper(mDBGetPlaceVisitStats);
  nsresult rv = mDBGetPlaceVisitStats->BindInt64Parameter(0, aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResults = PR_FALSE;
  rv = mDBGetPlaceVisitStats->ExecuteStep(&hasResults);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResults) {
    NS_WARNING("attempting to update frecency for a bogus place");
    
    
    return NS_OK;
  }

  PRInt32 typed = 0;
  rv = mDBGetPlaceVisitStats->GetInt32(0, &typed);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 hidden = 0;
  rv = mDBGetPlaceVisitStats->GetInt32(1, &hidden);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 oldFrecency = 0;
  rv = mDBGetPlaceVisitStats->GetInt32(2, &oldFrecency);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 visitCountForFrecency = 0;

  
  
  
  rv = CalculateFullVisitCount(aPlaceId, &visitCountForFrecency);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 newFrecency = 0;
  rv = CalculateFrecencyInternal(aPlaceId, typed, visitCountForFrecency,
                                 aIsBookmarked, &newFrecency);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  if (newFrecency == oldFrecency || oldFrecency && newFrecency < 0)
    return NS_OK;

  mozStorageStatementScoper updateScoper(mDBUpdateFrecencyAndHidden);
  rv = mDBUpdateFrecencyAndHidden->BindInt64Parameter(0, aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBUpdateFrecencyAndHidden->BindInt32Parameter(1, newFrecency);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = mDBUpdateFrecencyAndHidden->BindInt32Parameter(2, 
         newFrecency ? 0  : hidden);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBUpdateFrecencyAndHidden->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::CalculateFrecencyInternal(PRInt64 aPlaceId,
                                        PRInt32 aTyped,
                                        PRInt32 aVisitCount,
                                        PRBool aIsBookmarked,
                                        PRInt32 *aFrecency)
{
  PRTime normalizedNow = NormalizeTimeRelativeToday(GetNow());

  float pointsForSampledVisits = 0.0;

  if (aPlaceId != -1) {
    PRInt32 numSampledVisits = 0;

    mozStorageStatementScoper scoper(mDBVisitsForFrecency);
    nsresult rv = mDBVisitsForFrecency->BindInt64Parameter(0, aPlaceId);
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRBool hasMore = PR_FALSE;
    while (NS_SUCCEEDED(mDBVisitsForFrecency->ExecuteStep(&hasMore)) 
           && hasMore) {
      numSampledVisits++;

      PRInt32 visitType = mDBVisitsForFrecency->AsInt32(1);

      PRInt32 bonus = 0;

      switch (visitType) {
        case nsINavHistoryService::TRANSITION_EMBED:
          bonus = mEmbedVisitBonus;
          break;
        case nsINavHistoryService::TRANSITION_LINK:
          bonus = mLinkVisitBonus;
          break;
        case nsINavHistoryService::TRANSITION_TYPED:
          bonus = mTypedVisitBonus;
          break;
        case nsINavHistoryService::TRANSITION_BOOKMARK:
          bonus = mBookmarkVisitBonus;
          break;
        case nsINavHistoryService::TRANSITION_DOWNLOAD:
          bonus = mDownloadVisitBonus;
          break;
        case nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT:
          bonus = mPermRedirectVisitBonus;
          break;
        case nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY:
          bonus = mTempRedirectVisitBonus;
          break;
        default:
          
          if (visitType)
            NS_WARNING("new transition but no weight for frecency");
          bonus = mDefaultVisitBonus;
          break;
      }

      
      if (aIsBookmarked)
        bonus += mBookmarkVisitBonus;

#ifdef DEBUG_FRECENCY
      printf("CalculateFrecency() for place %lld has a bonus of %d\n", aPlaceId, bonus);
#endif

      
      if (bonus) {
        PRTime visitDate = mDBVisitsForFrecency->AsInt64(0);
        PRInt64 ageInDays = GetAgeInDays(normalizedNow, visitDate);

        PRInt32 weight = 0;

        if (ageInDays <= mFirstBucketCutoffInDays)
          weight = mFirstBucketWeight;
        else if (ageInDays <= mSecondBucketCutoffInDays)
          weight = mSecondBucketWeight;
        else if (ageInDays <= mThirdBucketCutoffInDays)
          weight = mThirdBucketWeight;
        else if (ageInDays <= mFourthBucketCutoffInDays) 
          weight = mFourthBucketWeight;
        else
          weight = mDefaultWeight;

        pointsForSampledVisits += (float)(weight * (bonus / 100.0));
      }
    }

    if (numSampledVisits) {
      
      if (!pointsForSampledVisits) {
        
        
        
        PRInt32 visitCount = 0;
        mozStorageStatementScoper scoper(mDBGetIdPageInfo);
        rv = mDBGetIdPageInfo->BindInt64Parameter(0, aPlaceId);
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool hasVisits = PR_TRUE;
        if (NS_SUCCEEDED(mDBGetIdPageInfo->ExecuteStep(&hasVisits)) && hasVisits) {
          rv = mDBGetIdPageInfo->GetInt32(nsNavHistory::kGetInfoIndex_VisitCount,
                                          &visitCount);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        
        *aFrecency = -visitCount;
      }
      else {
        
        
        
        *aFrecency = (PRInt32) NS_ceilf(aVisitCount * NS_ceilf(pointsForSampledVisits) / numSampledVisits);
      }

#ifdef DEBUG_FRECENCY
      printf("CalculateFrecency() for place %lld: %d = %d * %f / %d\n", aPlaceId, *aFrecency, aVisitCount, pointsForSampledVisits, numSampledVisits);
#endif

      return NS_OK;
    }
  }
 
  
  
  
  
  PRInt32 bonus = 0;

  
  
  
  
  
  if (aIsBookmarked)
    bonus += mUnvisitedBookmarkBonus;
  if (aTyped)
    bonus += mUnvisitedTypedBonus;

  
  
  
  
  pointsForSampledVisits = mFirstBucketWeight * (bonus / (float)100.0); 
   
  
  
  if (!aVisitCount && aIsBookmarked)
    aVisitCount = 1;

  
  
  *aFrecency = (PRInt32) NS_ceilf(aVisitCount * NS_ceilf(pointsForSampledVisits));
#ifdef DEBUG_FRECENCY
  printf("CalculateFrecency() for unvisited: frecency %d = %f points (b: %d, t: %d) * visit count %d\n", *aFrecency, pointsForSampledVisits, aIsBookmarked, aTyped, aVisitCount);
#endif
  return NS_OK;
}

nsresult
nsNavHistory::CalculateFrecency(PRInt64 aPlaceId,
                                PRInt32 aTyped,
                                PRInt32 aVisitCount,
                                nsCAutoString &aURL,
                                PRInt32 *aFrecency)
{
  *aFrecency = 0;

  PRBool isBookmark = PR_FALSE;

  
  
  if (!IsQueryURI(aURL) && aPlaceId != -1) {
    nsNavBookmarks *bs = nsNavBookmarks::GetBookmarksService();
    isBookmark = bs->IsRealBookmark(aPlaceId);
  }

  nsresult rv = CalculateFrecencyInternal(aPlaceId, aTyped, aVisitCount,
                                          isBookmark, aFrecency);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsNavHistory::RecalculateFrecencies(PRInt32 aCount, PRBool aRecalcOld)
{
  mozStorageTransaction transaction(mDBConn, PR_TRUE);

  nsresult rv = RecalculateFrecenciesInternal(GetDBInvalidFrecencies(), aCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRecalcOld) {
    rv = RecalculateFrecenciesInternal(GetDBOldFrecencies(), aCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult 
nsNavHistory::RecalculateFrecenciesInternal(mozIStorageStatement *aStatement, PRInt32 aCount)
{
  mozStorageStatementScoper scoper(aStatement);

  nsresult rv = aStatement->BindInt32Parameter(0, aCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(aStatement->ExecuteStep(&hasMore)) && hasMore) {
    PRInt64 placeId = aStatement->AsInt64(0);
    
    PRInt32 hidden = aStatement->AsInt32(2);
    PRInt32 typed = aStatement->AsInt32(3);
    PRInt32 oldFrecency = aStatement->AsInt32(4);

    nsCAutoString url;
    aStatement->GetUTF8String(5, url);

    PRInt32 newFrecency = 0;
    PRInt32 visitCountForFrecency = 0;

    
    
    rv = CalculateFullVisitCount(placeId, &visitCountForFrecency);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CalculateFrecency(placeId, typed, visitCountForFrecency, 
                           url, &newFrecency);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (newFrecency == oldFrecency)
      continue;

    mozStorageStatementScoper updateScoper(mDBUpdateFrecencyAndHidden);
    rv = mDBUpdateFrecencyAndHidden->BindInt64Parameter(0, placeId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBUpdateFrecencyAndHidden->BindInt32Parameter(1, newFrecency);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    rv = mDBUpdateFrecencyAndHidden->BindInt32Parameter(2, 
           newFrecency ? 0  : hidden);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBUpdateFrecencyAndHidden->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


nsICollation *
nsNavHistory::GetCollation()
{
  if (mCollation)
    return mCollation;

  
  nsCOMPtr<nsILocale> locale;
  nsCOMPtr<nsILocaleService> ls(do_GetService(NS_LOCALESERVICE_CONTRACTID));
  NS_ENSURE_TRUE(ls, nsnull);
  nsresult rv = ls->GetApplicationLocale(getter_AddRefs(locale));
  NS_ENSURE_SUCCESS(rv, nsnull);

  
  nsCOMPtr<nsICollationFactory> cfact =
    do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID);
  NS_ENSURE_TRUE(cfact, nsnull);
  rv = cfact->CreateCollation(locale, getter_AddRefs(mCollation));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mCollation;
}

nsIStringBundle *
nsNavHistory::GetBundle()
{
  if (!mBundle) {
    nsCOMPtr<nsIStringBundleService> bundleService =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    NS_ENSURE_TRUE(bundleService, nsnull);
    nsresult rv = bundleService->CreateBundle(
        "chrome://places/locale/places.properties",
        getter_AddRefs(mBundle));
    NS_ENSURE_SUCCESS(rv, nsnull);
  }
  return mBundle;
}

nsIStringBundle *
nsNavHistory::GetDateFormatBundle()
{
  if (!mDateFormatBundle) {
    nsCOMPtr<nsIStringBundleService> bundleService =
      do_GetService(NS_STRINGBUNDLE_CONTRACTID);
    NS_ENSURE_TRUE(bundleService, nsnull);
    nsresult rv = bundleService->CreateBundle(
        "chrome://global/locale/dateFormat.properties",
        getter_AddRefs(mDateFormatBundle));
    NS_ENSURE_SUCCESS(rv, nsnull);
  }
  return mDateFormatBundle;
}

mozIStorageStatement *
nsNavHistory::GetDBVisitToVisitResult()
{
  if (mDBVisitToVisitResult)
    return mDBVisitToVisitResult;

  
  
  
  
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          "v.visit_date, f.url, v.session, null "
        "FROM moz_places_temp h "
        "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id "
        "LEFT JOIN moz_historyvisits v ON h.id = v.place_id "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE v.id = ?1 OR v_t.id = ?1 "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          "v.visit_date, f.url, v.session, null "
        "FROM moz_places h "
        "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id "
        "LEFT JOIN moz_historyvisits v ON h.id = v.place_id "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE v.id = ?1 OR v_t.id = ?1 "
      "LIMIT 1"),
    getter_AddRefs(mDBVisitToVisitResult));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBVisitToVisitResult;
}

mozIStorageStatement *
nsNavHistory::GetDBVisitToURLResult()
{
  if (mDBVisitToURLResult)
    return mDBVisitToURLResult;

  
  
  
  
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
          ", f.url, null, null "
        "FROM moz_places_temp h "
        "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id "
        "LEFT JOIN moz_historyvisits v ON h.id = v.place_id "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE v.id = ?1 OR v_t.id = ?1 "
      "UNION ALL "
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
          ", f.url, null, null "
        "FROM moz_places h "
        "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id "
        "LEFT JOIN moz_historyvisits v ON h.id = v.place_id "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE v.id = ?1 OR v_t.id = ?1 "
      "LIMIT 1"),
    getter_AddRefs(mDBVisitToURLResult));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBVisitToURLResult;
}

mozIStorageStatement *
nsNavHistory::GetDBBookmarkToUrlResult()
{
  if (mDBBookmarkToUrlResult)
    return mDBBookmarkToUrlResult;

  
  
  
  
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.fk, h.url, COALESCE(b.title, h.title), "
        "h.rev_host, h.visit_count, "
        SQL_STR_FRAGMENT_MAX_VISIT_DATE( "b.fk" )
        ", f.url, null, b.id, b.dateAdded, b.lastModified "
      "FROM moz_bookmarks b "
      "JOIN moz_places_temp h ON b.fk = h.id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE b.id = ?1 "
      "UNION ALL "
      "SELECT b.fk, h.url, COALESCE(b.title, h.title), "
        "h.rev_host, h.visit_count, "
        SQL_STR_FRAGMENT_MAX_VISIT_DATE( "b.fk" )
        ", f.url, null, b.id, b.dateAdded, b.lastModified "
      "FROM moz_bookmarks b "
      "JOIN moz_places h ON b.fk = h.id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE b.id = ?1 "
      "LIMIT 1"),
    getter_AddRefs(mDBBookmarkToUrlResult));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBBookmarkToUrlResult;
}

mozIStorageStatement *
nsNavHistory::GetDBInvalidFrecencies()
{
  if (mDBInvalidFrecencies)
    return mDBInvalidFrecencies;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT * FROM ( "
        "SELECT id, visit_count, hidden, typed, frecency, url "
        "FROM ( "
          "SELECT * FROM moz_places_temp "
          "WHERE frecency < 0 "
          "UNION ALL "
          "SELECT * FROM ( "
            "SELECT * FROM moz_places "
            "WHERE +id NOT IN (SELECT id FROM moz_places_temp) "
            "AND frecency < 0 "
            "ORDER BY frecency ASC LIMIT ROUND(?1 / 2) "
          ") "
        ") ORDER BY frecency ASC LIMIT ROUND(?1 / 2)) "
      "UNION "
      "SELECT * FROM ( "
        "SELECT id, visit_count, hidden, typed, frecency, url "
        "FROM moz_places "
        "WHERE frecency < 0 "
        "AND ROWID >= ABS(RANDOM() % (SELECT MAX(ROWID) FROM moz_places)) "
        "LIMIT ROUND(?1 / 2))"),
    getter_AddRefs(mDBInvalidFrecencies));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBInvalidFrecencies;
}

mozIStorageStatement *
nsNavHistory::GetDBOldFrecencies()
{
  if (mDBOldFrecencies)
    return mDBOldFrecencies;

  
  
  
  
  
  
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, visit_count, hidden, typed, frecency, url "
     "FROM moz_places "
     "WHERE ROWID >= ABS(RANDOM() % (SELECT MAX(ROWID) FROM moz_places)) "
     "LIMIT ?1"),
    getter_AddRefs(mDBOldFrecencies));
  NS_ENSURE_SUCCESS(rv, nsnull);

  return mDBOldFrecencies;
}

nsresult
nsNavHistory::FinalizeStatements() {
  mozIStorageStatement* stmts[] = {
    mDBGetURLPageInfo,
    mDBGetIdPageInfo,
    mDBRecentVisitOfURL,
    mDBRecentVisitOfPlace,
    mDBInsertVisit,
    mDBGetPageVisitStats,
    mDBIsPageVisited,
    mDBUpdatePageVisitStats,
    mDBAddNewPage,
    mDBGetTags,
    mFoldersWithAnnotationQuery,
    mDBSetPlaceTitle,
    mDBVisitToURLResult,
    mDBVisitToVisitResult,
    mDBBookmarkToUrlResult,
    mDBVisitsForFrecency,
    mDBUpdateFrecencyAndHidden,
    mDBGetPlaceVisitStats,
    mDBFullVisitCount,
    mDBInvalidFrecencies,
    mDBOldFrecencies,
    mDBCurrentQuery,
    mDBAutoCompleteQuery,
    mDBAutoCompleteTypedQuery,
    mDBAutoCompleteHistoryQuery,
    mDBAutoCompleteStarQuery,
    mDBAutoCompleteTagsQuery,
    mDBPreviousQuery,
    mDBAdaptiveQuery,
    mDBKeywordQuery,
    mDBFeedbackIncrease
  };

  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(stmts); i++) {
    nsresult rv = nsNavHistory::FinalizeStatement(stmts[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::RequestCharset(nsIWebNavigation* aWebNavigation,
                             nsIChannel* aChannel,
                             PRBool* aWantCharset,
                             nsISupports** aClosure,
                             nsACString& aResult)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  *aWantCharset = PR_FALSE;
  *aClosure = nsnull;

  nsCOMPtr<nsIURI> uri;
  nsresult rv = aChannel->GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return NS_OK;

  nsAutoString charset;
  rv = GetCharsetForURI(uri, charset);
  NS_ENSURE_SUCCESS(rv, rv);

  CopyUTF16toUTF8(charset, aResult);
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::NotifyResolvedCharset(const nsACString& aCharset,
                                    nsISupports* aClosure)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  NS_ERROR("Unexpected call to NotifyResolvedCharset -- we never set aWantCharset to true!");
  return NS_ERROR_NOT_IMPLEMENTED;
}
