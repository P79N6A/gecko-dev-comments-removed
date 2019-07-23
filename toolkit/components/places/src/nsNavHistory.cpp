









































#include <stdio.h>
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsAnnotationService.h"

#include "nsIArray.h"
#include "nsArrayEnumerator.h"
#include "nsCollationCID.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsDateTimeFormatCID.h"
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

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "mozIStorageFunction.h"
#include "mozStorageCID.h"
#include "mozStorageHelper.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIIdleService.h"
#include "nsILivemarkService.h"

#include "nsMathUtils.h" 




#define RECENT_EVENT_THRESHOLD (15 * 60 * 1000000)



#define BOOKMARK_REDIRECT_TIME_THRESHOLD (2 * 60 * 100000)






#define RECENT_EVENT_QUEUE_MAX_LENGTH 128


#define PREF_BRANCH_BASE                        "browser."
#define PREF_BROWSER_HISTORY_EXPIRE_DAYS_MIN    "history_expire_days_min"
#define PREF_BROWSER_HISTORY_EXPIRE_DAYS_MAX    "history_expire_days"
#define PREF_BROWSER_HISTORY_EXPIRE_SITES       "history_expire_sites"
#define PREF_AUTOCOMPLETE_ONLY_TYPED            "urlbar.matchOnlyTyped"
#define PREF_AUTOCOMPLETE_FILTER_JAVASCRIPT     "urlbar.filter.javascript"
#define PREF_AUTOCOMPLETE_ENABLED               "urlbar.autocomplete.enabled"
#define PREF_AUTOCOMPLETE_MAX_RICH_RESULTS      "urlbar.maxRichResults"
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
#define PREF_BROWSER_IMPORT_BOOKMARKS           "browser.places.importBookmarksHTML"
#define PREF_BROWSER_IMPORT_DEFAULTS            "browser.places.importDefaults"
#define PREF_BROWSER_CREATEDSMARTBOOKMARKS      "browser.places.createdSmartBookmarks"
#define PREF_BROWSER_LEFTPANEFOLDERID           "browser.places.leftPaneFolderId"
      





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



#define LONG_IDLE_TIME_IN_MSECS (900000)



#define EXPIRE_IDLE_TIME_IN_MSECS (300000)


#define MAX_EXPIRE_RECORDS_ON_IDLE 200


#define EXPIRATION_CAP_SITES 40000


#define DB_MIGRATION_NONE    0
#define DB_MIGRATION_CREATED 1
#define DB_MIGRATION_UPDATED 2

NS_IMPL_ADDREF(nsNavHistory)
NS_IMPL_RELEASE(nsNavHistory)

NS_INTERFACE_MAP_BEGIN(nsNavHistory)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryService)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIGlobalHistory2, nsIGlobalHistory3)
  NS_INTERFACE_MAP_ENTRY(nsIGlobalHistory3)
  NS_INTERFACE_MAP_ENTRY(nsIDownloadHistory)
  NS_INTERFACE_MAP_ENTRY(nsIBrowserHistory)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
#ifdef MOZ_XUL
  NS_INTERFACE_MAP_ENTRY(nsIAutoCompleteSearch)
  NS_INTERFACE_MAP_ENTRY(nsIAutoCompleteSimpleResultListener)
#endif
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryService)
NS_INTERFACE_MAP_END

static nsresult GetReversedHostname(nsIURI* aURI, nsAString& host);
static void GetReversedHostname(const nsString& aForward, nsAString& aReversed);
static nsresult GenerateTitleFromURI(nsIURI* aURI, nsAString& aTitle);
static PRInt64 GetSimpleBookmarksQueryFolder(
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions);
static void ParseSearchTermsFromQueries(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                        nsTArray<nsStringArray*>* aTerms);

inline void ReverseString(const nsString& aInput, nsAString& aReversed)
{
  aReversed.Truncate(0);
  for (PRInt32 i = aInput.Length() - 1; i >= 0; i --)
    aReversed.Append(aInput[i]);
}
inline void parameterString(PRInt32 paramIndex, nsACString& aParamString)
{
  aParamString = nsPrintfCString("?%d", paramIndex + 1);
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

static const char* gQuitApplicationMessage = "quit-application";
static const char* gXpcomShutdown = "xpcom-shutdown";
static const char* gAutoCompleteFeedback = "autocomplete-will-enter-text";


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
                               mAutoCompleteOnlyTyped(PR_FALSE),
                               mAutoCompleteMaxResults(25),
                               mAutoCompleteSearchChunkSize(100),
                               mAutoCompleteSearchTimeout(100),
                               mAutoCompleteFinishedSearch(PR_FALSE),
                               mExpireDaysMin(0),
                               mExpireDaysMax(0),
                               mExpireSites(0),
                               mNumVisitsForFrecency(10),
                               mTagsFolder(-1)
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

  
  PRInt16 migrationType;
  rv = InitDB(&migrationType);
  if (NS_FAILED(rv)) {
    
    
    rv = InitDBFile(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = InitDB(&migrationType);
  }
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef IN_MEMORY_LINKS
  rv = InitMemDB();
  NS_ENSURE_SUCCESS(rv, rv);
#endif

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

  
  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = bundleService->CreateBundle(
      "chrome://places/locale/places.properties",
      getter_AddRefs(mBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsILocaleService> ls = do_GetService(NS_LOCALESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ls->GetApplicationLocale(getter_AddRefs(mLocale));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsICollationFactory> cfact = do_CreateInstance(
     NS_COLLATIONFACTORY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cfact->CreateCollation(mLocale, getter_AddRefs(mCollation));
  NS_ENSURE_SUCCESS(rv, rv);

  
  mDateFormatter = do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  InitializeIdleTimer();

  
  NS_ENSURE_TRUE(mRecentTyped.Init(128), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentBookmark.Init(128), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentRedirects.Init(128), NS_ERROR_OUT_OF_MEMORY);

  







  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch2> pbi = do_QueryInterface(mPrefBranch);
  if (pbi) {
    pbi->AddObserver(PREF_AUTOCOMPLETE_ONLY_TYPED, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_FILTER_JAVASCRIPT, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_MAX_RICH_RESULTS, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_SEARCH_CHUNK_SIZE, this, PR_FALSE);
    pbi->AddObserver(PREF_AUTOCOMPLETE_SEARCH_TIMEOUT, this, PR_FALSE);
    pbi->AddObserver(PREF_BROWSER_HISTORY_EXPIRE_DAYS_MAX, this, PR_FALSE);
    pbi->AddObserver(PREF_BROWSER_HISTORY_EXPIRE_DAYS_MIN, this, PR_FALSE);
    pbi->AddObserver(PREF_BROWSER_HISTORY_EXPIRE_SITES, this, PR_FALSE);
  }

  observerService->AddObserver(this, gQuitApplicationMessage, PR_FALSE);
  observerService->AddObserver(this, gXpcomShutdown, PR_FALSE);
  observerService->AddObserver(this, gAutoCompleteFeedback, PR_FALSE);

  







  if (migrationType == DB_MIGRATION_CREATED) {
    nsCOMPtr<nsIFile> historyFile;
    rv = NS_GetSpecialDirectory(NS_APP_HISTORY_50_FILE,
                                getter_AddRefs(historyFile));
    if (NS_SUCCEEDED(rv) && historyFile) {
      ImportHistory(historyFile);
    }
  }

  
  
  
  if (migrationType != DB_MIGRATION_NONE)
    (void)RecalculateFrecencies(mNumCalculateFrecencyOnMigrate,
                                PR_FALSE );

  
  

  return NS_OK;
}


nsresult
nsNavHistory::InitDBFile(PRBool aForceInit)
{
  if (aForceInit) {
    NS_ASSERTION(mDBConn,
                 "When forcing initialization, a database connection must exist!");
  }

  
  nsCOMPtr<nsIFile> profDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(profDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = profDir->Clone(getter_AddRefs(mDBFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBFile->Append(DB_FILENAME);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  PRBool dbExists;
  if (aForceInit) {
    
    nsCOMPtr<nsIFile> backup;
    rv = mDBConn->BackupDB(DB_CORRUPT_FILENAME, profDir, getter_AddRefs(backup));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->Close();
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    dbExists = PR_FALSE;
  }
  else {
    
    rv = mDBFile->Exists(&dbExists);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  
  mDBService = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBService->OpenDatabase(mDBFile, getter_AddRefs(mDBConn));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool ready;
  (void)mDBConn->GetConnectionReady(&ready);
  if (!ready) {
    dbExists = PR_FALSE;
  
    
    nsCOMPtr<nsIFile> backup;
    rv = mDBConn->BackupDB(DB_CORRUPT_FILENAME, profDir, getter_AddRefs(backup));
    NS_ENSURE_SUCCESS(rv, rv);
 
    
    rv = mDBFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = profDir->Clone(getter_AddRefs(mDBFile));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBFile->Append(DB_FILENAME);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBService->OpenDatabase(mDBFile, getter_AddRefs(mDBConn));
    NS_ENSURE_SUCCESS(rv, rv);
    (void)mDBConn->GetConnectionReady(&ready);
    if (!ready) {
      mDBConn = nsnull;
      return NS_ERROR_UNEXPECTED;
    }
  }
  
  
  if (!dbExists) {
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService("@mozilla.org/preferences-service;1"));
    if (prefs) {
      rv = prefs->SetBoolPref(PREF_BROWSER_IMPORT_BOOKMARKS, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = prefs->SetBoolPref(PREF_BROWSER_IMPORT_DEFAULTS, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);  

      
      rv = prefs->SetBoolPref(PREF_BROWSER_CREATEDSMARTBOOKMARKS, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);  
      
      
      rv = prefs->SetIntPref(PREF_BROWSER_LEFTPANEFOLDERID, -1);
      NS_ENSURE_SUCCESS(rv, rv); 
    }
  }

  return NS_OK;
}





#define PLACES_SCHEMA_VERSION 6

nsresult
nsNavHistory::InitDB(PRInt16 *aMadeChanges)
{
  nsresult rv;
  PRBool tableExists;
  *aMadeChanges = DB_MIGRATION_NONE;

  
  
  
  
  
  
  nsCAutoString pageSizePragma("PRAGMA page_size=");
  pageSizePragma.AppendInt(DEFAULT_DB_PAGE_SIZE);
  rv = mDBConn->ExecuteSimpleSQL(pageSizePragma);
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

  
  if (!tableExists) {
    *aMadeChanges = DB_MIGRATION_CREATED;
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_places ("
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
  }

  
  rv = mDBConn->TableExists(NS_LITERAL_CSTRING("moz_historyvisits"), &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! tableExists) {
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_historyvisits ("
        "id INTEGER PRIMARY KEY, "
        "from_visit INTEGER, "
        "place_id INTEGER, "
        "visit_date INTEGER, "
        "visit_type INTEGER, "
        "session INTEGER)"));
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
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_inputhistory ("
        "place_id INTEGER NOT NULL, "
        "input LONGVARCHAR NOT NULL, "
        "use_count INTEGER, "
        "PRIMARY KEY (place_id, input))"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool migrated = PR_FALSE;
  rv = EnsureCurrentSchema(mDBConn, &migrated);
  NS_ENSURE_SUCCESS(rv, rv);
  if (migrated && *aMadeChanges != DB_MIGRATION_CREATED)
    *aMadeChanges = DB_MIGRATION_UPDATED;

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  

  

  rv = InitFunctions();
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

  PRInt32 idleTimerTimeout = PR_MIN(LONG_IDLE_TIME_IN_MSECS,
                                    EXPIRE_IDLE_TIME_IN_MSECS);
  if (mFrecencyUpdateIdleTime)
    idleTimerTimeout = PR_MIN(idleTimerTimeout, mFrecencyUpdateIdleTime);

  rv = mIdleTimer->InitWithFuncCallback(IdleTimerCallback, this,
                                        idleTimerTimeout,
                                        nsITimer::TYPE_REPEATING_SLACK);
  NS_ENSURE_SUCCESS(rv, rv);
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
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count "
      "FROM moz_places h "
      "WHERE h.url = ?1"),
    getter_AddRefs(mDBGetURLPageInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count "
      "FROM moz_places h WHERE h.id = ?1"),
                                getter_AddRefs(mDBGetIdPageInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT v.id, v.session "
      "FROM moz_places h JOIN moz_historyvisits v ON h.id = v.place_id "
      "WHERE h.url = ?1 "
      "ORDER BY v.visit_date DESC "
      "LIMIT 1"),
    getter_AddRefs(mDBRecentVisitOfURL));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO moz_historyvisits "
      "(from_visit, place_id, visit_date, visit_type, session) "
      "VALUES (?1, ?2, ?3, ?4, ?5)"),
    getter_AddRefs(mDBInsertVisit));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT id, visit_count, typed, hidden "
      "FROM moz_places "
      "WHERE url = ?1"),
    getter_AddRefs(mDBGetPageVisitStats));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET visit_count = ?2, hidden = ?3, typed = ?4 "
      "WHERE id = ?1"),
    getter_AddRefs(mDBUpdatePageVisitStats));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT OR REPLACE INTO moz_places "
      "(url, title, rev_host, hidden, typed, visit_count, frecency) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7)"),
    getter_AddRefs(mDBAddNewPage));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
        ", f.url, null, null "
      "FROM moz_places h "
      "JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE v.id = ?1"),
    getter_AddRefs(mDBVisitToURLResult));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
             "v.visit_date, f.url, v.session, null "
      "FROM moz_places h "
      "JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE v.id = ?1"),
    getter_AddRefs(mDBVisitToVisitResult));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
        ", f.url, null, null "
      "FROM moz_places h "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.url = ?1"),
    getter_AddRefs(mDBUrlToUrlResult));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.fk, h.url, COALESCE(b.title, h.title), "
        "h.rev_host, h.visit_count, "
        SQL_STR_FRAGMENT_MAX_VISIT_DATE( "b.fk" )
        ", f.url, null, null, b.dateAdded, b.lastModified "
      "FROM moz_bookmarks b "
      "JOIN moz_places h ON b.fk = h.id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE b.id = ?1"),
    getter_AddRefs(mDBBookmarkToUrlResult));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT GROUP_CONCAT(t.title, ' ') "
      "FROM moz_places h "
      "JOIN moz_bookmarks b ON b.type = ") +
        nsPrintfCString("%d", nsINavBookmarksService::TYPE_BOOKMARK) +
        NS_LITERAL_CSTRING(" AND b.fk = h.id "
      "JOIN moz_bookmarks t ON t.parent = ?1 AND t.id = b.parent "
      "WHERE h.url = ?2"),
    getter_AddRefs(mDBGetTags));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT annos.item_id, annos.content FROM moz_anno_attributes attrs " 
    "JOIN moz_items_annos annos ON attrs.id = annos.anno_attribute_id "
    "WHERE attrs.name = ?1"), 
    getter_AddRefs(mFoldersWithAnnotationQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT IFNULL(r.visit_date, v.visit_date) date, IFNULL(r.visit_type, v.visit_type) "
    "FROM moz_historyvisits v "
    "LEFT OUTER JOIN moz_historyvisits r "
      "ON r.id = v.from_visit AND v.visit_type IN ") +
      nsPrintfCString("(%d,%d) ", TRANSITION_REDIRECT_PERMANENT,
      TRANSITION_REDIRECT_TEMPORARY) + NS_LITERAL_CSTRING(
    "WHERE v.place_id = ?1 ORDER BY date DESC LIMIT ") +
     nsPrintfCString("%d", mNumVisitsForFrecency),
    getter_AddRefs(mDBVisitsForFrecency));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, visit_count, hidden, typed, frecency, url "
    "FROM ("
      "SELECT * FROM ("
        "SELECT * FROM moz_places WHERE frecency = -1 "
        "ORDER BY visit_count DESC LIMIT ROUND(?1 / 2)) "
      "UNION "
      "SELECT * FROM ("
        "SELECT * FROM moz_places WHERE frecency = -1 "
        "ORDER BY RANDOM() LIMIT ROUND(?1 / 2)))"),
    getter_AddRefs(mDBInvalidFrecencies));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT id, visit_count, hidden, typed, frecency, url "
     "FROM moz_places "
     "ORDER BY RANDOM() LIMIT ?1"),
    getter_AddRefs(mDBOldFrecencies));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places SET frecency = ?2, hidden = ?3 WHERE id = ?1"),
    getter_AddRefs(mDBUpdateFrecencyAndHidden));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT typed, hidden, frecency FROM moz_places WHERE id = ?1"),
    getter_AddRefs(mDBGetPlaceVisitStats));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.parent FROM moz_places h JOIN moz_bookmarks b "
      " on b.fk = h.id WHERE b.type = 1 and h.id = ?1"),
    getter_AddRefs(mDBGetBookmarkParentsForPlace));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(
    NS_LITERAL_CSTRING("SELECT COUNT(*) FROM moz_historyvisits " 
      "WHERE place_id = ?1"),
    getter_AddRefs(mDBVisitCountForFrecency));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = mDBConn->CreateStatement(
    NS_LITERAL_CSTRING("SELECT COUNT(*) FROM moz_historyvisits " 
      "WHERE visit_type NOT IN(0,4) AND place_id = ?1"),
    getter_AddRefs(mDBTrueVisitCount));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}









nsresult
nsNavHistory::ForceMigrateBookmarksDB(mozIStorageConnection* aDBConn) 
{
  
  nsresult rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE IF EXISTS moz_bookmarks"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE IF EXISTS moz_bookmarks_folders"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE IF EXISTS moz_bookmarks_roots"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE IF EXISTS moz_keywords"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = nsNavBookmarks::InitTables(aDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService("@mozilla.org/preferences-service;1"));
  if (prefs) {
    prefs->SetBoolPref(PREF_BROWSER_IMPORT_BOOKMARKS, PR_TRUE);
  }
  return rv;
}


nsresult
nsNavHistory::MigrateV3Up(mozIStorageConnection* aDBConn) 
{
  
  
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("SELECT type from moz_annos"),
                                         getter_AddRefs(statement));
  if (NS_SUCCEEDED(rv))
    return NS_OK;

  
  rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "ALTER TABLE moz_annos ADD type INTEGER DEFAULT 0"));
  if (NS_FAILED(rv)) {
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DROP TABLE IF EXISTS moz_annos"));
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

  
  
  
  
  rv = aDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_favicons_url"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_anno_attributes_nameindex"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::EnsureCurrentSchema(mozIStorageConnection* aDBConn, PRBool* aDidMigrate)
{
  
  
  
  PRBool oldIndexExists = PR_FALSE;
  nsresult rv = aDBConn->IndexExists(
    NS_LITERAL_CSTRING("moz_historyvisits_pageindex"), &oldIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (oldIndexExists) {
    *aDidMigrate = PR_TRUE;
    
    mozStorageTransaction pageindexTransaction(aDBConn, PR_FALSE);

    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_historyvisits_pageindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "CREATE INDEX IF NOT EXISTS moz_historyvisits_placedateindex "
        "ON moz_historyvisits (place_id, visit_date)"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = pageindexTransaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT frecency FROM moz_places"), getter_AddRefs(statement));

  if (NS_FAILED(rv)) {
    *aDidMigrate = PR_TRUE;
    
    mozStorageTransaction frecencyTransaction(aDBConn, PR_FALSE);

    
    
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_places ADD frecency INTEGER DEFAULT -1 NOT NULL"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE INDEX IF NOT EXISTS "
      "moz_places_frecencyindex ON moz_places (frecency)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    

    
    
    rv = FixInvalidFrecenciesForExcludedPlaces();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = frecencyTransaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsNavHistory::CleanUpOnQuit()
{
  
  
  nsCOMPtr<mozIStorageStatement> statement2;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT user_title FROM moz_places"), getter_AddRefs(statement2));
  if (NS_SUCCEEDED(rv)) {
    mozStorageTransaction transaction(mDBConn, PR_FALSE);
    
    
    
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_urlindex"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_titleindex"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_faviconindex"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_hostindex"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_visitcount"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_frecencyindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = RemoveDuplicateURIs();
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("ALTER TABLE moz_places RENAME TO moz_places_backup"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_places ("
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

    
    
    
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE UNIQUE INDEX moz_places_url_uniqueindex ON moz_places (url)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE INDEX moz_places_faviconindex ON moz_places (favicon_id)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE INDEX moz_places_hostindex ON moz_places (rev_host)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE INDEX moz_places_visitcount ON moz_places (visit_count)"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE INDEX moz_places_frecencyindex ON moz_places (frecency)"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "INSERT INTO moz_places "
      "SELECT id, url, title, rev_host, visit_count, hidden, typed, favicon_id, frecency "
      "FROM moz_places_backup"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TABLE moz_places_backup"));
    NS_ENSURE_SUCCESS(rv, rv);
    transaction.Commit();
  }

  
  mozStorageTransaction idxTransaction(mDBConn, PR_FALSE);
  rv = mDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_titleindex"));
  rv = mDBConn->ExecuteSimpleSQL(
    NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_annos_item_idindex"));
  idxTransaction.Commit();

  
  PRBool oldIndexExists = PR_FALSE;
  rv = mDBConn->IndexExists(NS_LITERAL_CSTRING("moz_annos_attributesindex"), &oldIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (oldIndexExists) {
    
    mozStorageTransaction annoIndexTransaction(mDBConn, PR_FALSE);

    
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

    rv = annoIndexTransaction.Commit();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


#ifdef IN_MEMORY_LINKS




nsresult
nsNavHistory::InitMemDB()
{
  nsresult rv = mDBService->OpenSpecialDatabase("memory", getter_AddRefs(mMemDBConn));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMemDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE moz_memhistory (url LONGVARCHAR UNIQUE)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMemDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX moz_memhistory_index ON moz_memhistory (url)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMemDBConn->CreateStatement(
      NS_LITERAL_CSTRING("SELECT url FROM moz_memhistory WHERE url = ?1"),
      getter_AddRefs(mMemDBGetPage));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMemDBConn->CreateStatement(
      NS_LITERAL_CSTRING("INSERT OR IGNORE INTO moz_memhistory VALUES (?1)"),
      getter_AddRefs(mMemDBAddPage));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsCOMPtr<mozIStorageStatement> selectStatement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("SELECT url FROM moz_places WHERE visit_count > 0 ORDER BY url"),
                                getter_AddRefs(selectStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  
  mozStorageTransaction transaction(mMemDBConn, PR_FALSE);
  nsCString url;
  while(NS_SUCCEEDED(rv = selectStatement->ExecuteStep(&hasMore)) && hasMore) {
    rv = selectStatement->GetUTF8String(0, url);
    if (NS_SUCCEEDED(rv) && ! url.IsEmpty()) {
      rv = mMemDBAddPage->BindUTF8StringParameter(0, url);
      if (NS_SUCCEEDED(rv))
        mMemDBAddPage->Execute();
    }
  }
  transaction.Commit();

  return NS_OK;
}
#endif













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
nsNavHistory::InternalAddNewPage(nsIURI* aURI, const nsAString& aTitle,
                                 PRBool aHidden, PRBool aTyped,
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

  
  rv = mDBAddNewPage->BindInt32Parameter(5, aVisitCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString url;
  rv = aURI->GetSpec(url);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 frecency = -1;
  if (aCalculateFrecency) {
    rv = CalculateFrecency(-1 ,
                           aTyped, aVisitCount, url,
                           &frecency);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mDBAddNewPage->BindInt32Parameter(6, frecency);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBAddNewPage->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aPageID) {
    rv = mDBConn->GetLastInsertRowID(aPageID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}





nsresult
nsNavHistory::InternalAddVisit(PRInt64 aPageID, PRInt64 aReferringVisit,
                               PRInt64 aSessionID, PRTime aTime,
                               PRInt32 aTransitionType, PRInt64* visitID)
{
  nsresult rv;
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

  return mDBConn->GetLastInsertRowID(visitID);
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
#ifdef IN_MEMORY_LINKS
  
  nsresult rv = mMemDBGetPage->BindUTF8StringParameter(0, aURIString);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  PRBool hasPage = PR_FALSE;
  mMemDBGetPage->ExecuteStep(&hasPage);
  mMemDBGetPage->Reset();
  return hasPage;
#else

#ifdef LAZY_ADD
  
  for (PRUint32 i = 0; i < mLazyMessages.Length(); i ++) {
    if (mLazyMessages[i].type == LazyMessage::Type_AddURI) {
      if (aURIString.Equals(mLazyMessages[i].uriSpec))
        return PR_TRUE;
    }
  }
#endif

  
  mozStorageStatementScoper scoper(mDBGetPageVisitStats);
  nsresult rv = mDBGetPageVisitStats->BindUTF8StringParameter(0, aURIString);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool hasMore = PR_FALSE;
  rv = mDBGetPageVisitStats->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  if (!hasMore)
    return PR_FALSE;

  
  
  
  PRInt32 visitCount;
  rv = mDBGetPageVisitStats->GetInt32(1, &visitCount);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return visitCount > 0;
#endif
}




nsresult
nsNavHistory::LoadPrefs(PRBool aInitializing)
{
  if (! mPrefBranch)
    return NS_OK;

  mPrefBranch->GetIntPref(PREF_BROWSER_HISTORY_EXPIRE_DAYS_MAX, &mExpireDaysMax);
  mPrefBranch->GetIntPref(PREF_BROWSER_HISTORY_EXPIRE_DAYS_MIN, &mExpireDaysMin);
  if (NS_FAILED(mPrefBranch->GetIntPref(PREF_BROWSER_HISTORY_EXPIRE_SITES,
                                        &mExpireSites)))
    mExpireSites = EXPIRATION_CAP_SITES;
  
#ifdef MOZ_XUL
  PRBool oldCompleteOnlyTyped = mAutoCompleteOnlyTyped;
  mPrefBranch->GetBoolPref(PREF_AUTOCOMPLETE_ONLY_TYPED,
                           &mAutoCompleteOnlyTyped);
  mPrefBranch->GetBoolPref(PREF_AUTOCOMPLETE_FILTER_JAVASCRIPT,
                           &mAutoCompleteFilterJavascript);
  mPrefBranch->GetIntPref(PREF_AUTOCOMPLETE_MAX_RICH_RESULTS,
                          &mAutoCompleteMaxResults);
  mPrefBranch->GetIntPref(PREF_AUTOCOMPLETE_SEARCH_CHUNK_SIZE,
                          &mAutoCompleteSearchChunkSize);
  mPrefBranch->GetIntPref(PREF_AUTOCOMPLETE_SEARCH_TIMEOUT,
                          &mAutoCompleteSearchTimeout);
  if (!aInitializing && oldCompleteOnlyTyped != mAutoCompleteOnlyTyped) {
    
    nsresult rv = CreateAutoCompleteQueries();
    NS_ENSURE_SUCCESS(rv, rv);
  }
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
  nsCOMPtr<mozIStorageStatement> dbSelectStatement;
  nsresult rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("SELECT id FROM moz_historyvisits LIMIT 1"),
      getter_AddRefs(dbSelectStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  return dbSelectStatement->ExecuteStep(aHasEntries);
}

nsresult
nsNavHistory::FixInvalidFrecenciesForExcludedPlaces()
{
  
  
  
  nsCOMPtr<mozIStorageStatement> dbUpdateStatement;
  nsresult rv = mDBConn->CreateStatement(
    NS_LITERAL_CSTRING("UPDATE moz_places SET frecency = 0 WHERE id IN ("
      "SELECT h.id FROM moz_places h JOIN moz_bookmarks b ON h.id = b.fk "
      "WHERE frecency = -1 "
        
        "AND (b.parent IN ("
          "SELECT annos.item_id FROM moz_anno_attributes attrs "
          "JOIN moz_items_annos annos ON attrs.id = annos.anno_attribute_id "
          "WHERE attrs.name = ?1) "
        
        "AND (SELECT visit_date FROM moz_historyvisits "
          "WHERE place_id = h.id AND visit_type NOT IN (0,4) LIMIT 1) is null) "
      "OR SUBSTR(h.url,0,6) = 'place:')"),
    getter_AddRefs(dbUpdateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbUpdateStatement->BindUTF8StringParameter(0, NS_LITERAL_CSTRING(LMANNO_FEEDURI));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbUpdateStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::CalculateVisitCount(PRInt64 aPlaceId, PRBool aForFrecency, PRInt32 *aVisitCount)
{
  nsCOMPtr<mozIStorageStatement> dbSelectStatement = 
    aForFrecency ? mDBVisitCountForFrecency : mDBTrueVisitCount;
   
  mozStorageStatementScoper scope(dbSelectStatement);

  nsresult rv = dbSelectStatement->BindInt64Parameter(0, aPlaceId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasVisits = PR_TRUE;
  rv = dbSelectStatement->ExecuteStep(&hasVisits);
  NS_ENSURE_SUCCESS(rv, rv);

  if (hasVisits) {
    rv = dbSelectStatement->GetInt32(0, aVisitCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
    *aVisitCount = 0;
  
  return NS_OK;
}











NS_IMETHODIMP
nsNavHistory::MarkPageAsFollowedBookmark(nsIURI* aURI)
{
  
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










nsresult
nsNavHistory::CanAddURI(nsIURI* aURI, PRBool* canAdd)
{
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
      scheme.EqualsLiteral("data")) {
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
  
  PRBool canAdd = PR_FALSE;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    *aVisitID = 0;
    return NS_OK;
  }

  
#ifdef IN_MEMORY_LINKS
  rv = BindStatementURI(mMemDBAddPage, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  mMemDBAddPage->Execute();
#endif

  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  mozStorageStatementScoper scoper(mDBGetPageVisitStats);
  rv = BindStatementURI(mDBGetPageVisitStats, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool alreadyVisited = PR_TRUE;
  rv = mDBGetPageVisitStats->ExecuteStep(&alreadyVisited);

  PRInt64 pageID = 0;
  PRBool hidden; 
  PRBool typed;  
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
    if (hidden && (!aIsRedirect || aTransitionType == TRANSITION_TYPED) &&
        aTransitionType != TRANSITION_EMBED)
      hidden = PR_FALSE; 

    typed = oldTypedState || (aTransitionType == TRANSITION_TYPED);

    PRInt32 trueVisitCount = 0;

    
    rv = CalculateVisitCount(pageID, PR_FALSE ,
                             &trueVisitCount);

    
    
    if (trueVisitCount == 0)
      newItem = PR_TRUE;

    
    mozStorageStatementScoper updateScoper(mDBUpdatePageVisitStats);
    rv = mDBUpdatePageVisitStats->BindInt64Parameter(0, pageID);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (aTransitionType != TRANSITION_EMBED)
      trueVisitCount++;

    rv = mDBUpdatePageVisitStats->BindInt32Parameter(1, trueVisitCount);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBUpdatePageVisitStats->BindInt32Parameter(2, hidden);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBUpdatePageVisitStats->BindInt32Parameter(3, typed);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBUpdatePageVisitStats->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    newItem = PR_TRUE;

    
    mDBGetPageVisitStats->Reset();
    scoper.Abandon();

    
    
    hidden = (aTransitionType == TRANSITION_EMBED || aIsRedirect ||
              aTransitionType == TRANSITION_DOWNLOAD);

    typed = (aTransitionType == TRANSITION_TYPED);

    
    nsString voidString;
    voidString.SetIsVoid(PR_TRUE);
    rv = InternalAddNewPage(aURI, voidString, hidden, typed, 1, PR_TRUE, &pageID);
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

  
  
  
  (void)UpdateFrecency(pageID, PR_FALSE);

  
  
  
  PRUint32 added = 0;
  if (! hidden && aTransitionType != TRANSITION_EMBED) {
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
  *_retval = new nsNavHistoryQuery();
  if (! *_retval)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval);
  return NS_OK;
}



NS_IMETHODIMP nsNavHistory::GetNewQueryOptions(nsINavHistoryQueryOptions **_retval)
{
  *_retval = new nsNavHistoryQueryOptions();
  NS_ENSURE_TRUE(*_retval, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*_retval);
  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::ExecuteQuery(nsINavHistoryQuery *aQuery, nsINavHistoryQueryOptions *aOptions,
                           nsINavHistoryResult** _retval)
{
  return ExecuteQueries(&aQuery, 1, aOptions, _retval);
}














NS_IMETHODIMP
nsNavHistory::ExecuteQueries(nsINavHistoryQuery** aQueries, PRUint32 aQueryCount,
                             nsINavHistoryQueryOptions *aOptions,
                             nsINavHistoryResult** _retval)
{
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
  
  if (aOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS)
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
                        PRBool aUseLimit);

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
  PRUint16 mSortingMode;
  PRUint32 mMaxResults;

  nsCString mQueryString;
  nsCString mGroupBy;
  PRBool mHasDateColumns;
  PRBool mSkipOrderBy;
};

PlacesSQLQueryBuilder::PlacesSQLQueryBuilder(
    const nsCString& aConditions, 
    nsNavHistoryQueryOptions* aOptions, 
    PRBool aUseLimit) :
  mConditions(aConditions),
  mResultType(aOptions->ResultType()),
  mQueryType(aOptions->QueryType()),
  mIncludeHidden(aOptions->IncludeHidden()),
  mSortingMode(aOptions->SortingMode()),
  mMaxResults(aOptions->MaxResults()),
  mUseLimit(aUseLimit),
  mSkipOrderBy(PR_FALSE)
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
      mQueryString = NS_LITERAL_CSTRING(
        "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          "MAX(visit_date), f.url, null, null "
        "FROM moz_places h "
             "LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
             "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id ");

      if (!mIncludeHidden)
        mQueryString += NS_LITERAL_CSTRING(
          " WHERE h.hidden <> 1 AND v.visit_type NOT IN (0,4)"
            " {ADDITIONAL_CONDITIONS} ");

      mGroupBy = NS_LITERAL_CSTRING(" GROUP BY h.id");
      break;

    case nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS:
      mQueryString = NS_LITERAL_CSTRING(
        "SELECT b.fk, h.url, COALESCE(b.title, h.title), h.rev_host, "
          "h.visit_count,"
          SQL_STR_FRAGMENT_MAX_VISIT_DATE( "b.fk" ) 
          ", f.url, null, b.id, b.dateAdded, b.lastModified "
        "FROM moz_bookmarks b "
             "JOIN moz_places h ON b.fk = h.id AND b.type = 1 "
             "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id ");
      break;

    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }
  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsVisit()
{
  mQueryString = NS_LITERAL_CSTRING(
    "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
      "v.visit_date, f.url, v.session, null "
    "FROM moz_places h "
         "LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
         "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id ");

  if (!mIncludeHidden)
    mQueryString += NS_LITERAL_CSTRING(
      " WHERE h.hidden <> 1 AND v.visit_type NOT IN (0,4)"
        " {ADDITIONAL_CONDITIONS} ");

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsDay()
{
  mSkipOrderBy = PR_TRUE;
  PRBool asDayQuery = 
    mResultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY;

  mQueryString = nsPrintfCString(255,
    "SELECT null, "
      "'place:type=%ld&sort=%ld&beginTime='||beginTime||'&endTime='||endTime, "
      "dayTitle, null, null, endTime, null, null, null, null "
    "FROM (", 
    (asDayQuery
       ?nsINavHistoryQueryOptions::RESULTS_AS_URI
       :nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY),
     nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING);

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  struct Midnight
  {
    Midnight() {
      mNow = NormalizeTimeRelativeToday(PR_Now());  
    }
    PRTime Get(PRInt32 aDayOffset) {
      PRTime result;
      LL_MUL(result, aDayOffset, USECS_PER_DAY);
      LL_ADD(result, result, mNow);
      return result;
    }
    PRTime mNow;
  } midnight;

  nsCAutoString dateName;

  const PRInt32 MAX_DAYS = 6;

  for (PRInt32 i = 0; i <= MAX_DAYS; i++) {
    switch (i)
    {
      case 0:
        history->GetStringFromName(
            NS_LITERAL_STRING("finduri-AgeInDays-is-0").get(), dateName);
        break;
      case 1:
        history->GetStringFromName(
            NS_LITERAL_STRING("finduri-AgeInDays-is-1").get(), dateName);
        break;
      default:
        history->GetAgeInDaysString(i, 
            NS_LITERAL_STRING("finduri-AgeInDays-is").get(), dateName);
        break;
    }

    PRInt32 fromDayAgo = -i;
    PRInt32 toDayAgo = -i + 1;

    nsPrintfCString dayRange(1024,
      "SELECT * "
      "FROM (SELECT %d dayOrder, "
                  "'%d' dayRange, "
                  "'%s' dayTitle, "
                  "%llu beginTime, "
                  "%llu endTime "
      "FROM  moz_historyvisits "
      "WHERE visit_date >= %llu AND visit_date < %llu "
      "  AND visit_type NOT IN (0,4) "
      "LIMIT 1) TUNION%d UNION ", 
      i, i, dateName.get(), 
      midnight.Get(fromDayAgo),
      midnight.Get(toDayAgo), 
      midnight.Get(fromDayAgo),
      midnight.Get(toDayAgo),
      i);

    mQueryString.Append( dayRange );
  }

  history->GetAgeInDaysString(MAX_DAYS, 
    NS_LITERAL_STRING("finduri-AgeInDays-isgreater").get(), dateName);

  mQueryString.Append(nsPrintfCString(1024,
    "SELECT * "
    "FROM (SELECT %d dayOrder, "
                 "'%d+' dayRange, "
                 "'%s' dayTitle, "
                 "1 beginTime, "
                 "%llu endTime "
          "FROM  moz_historyvisits "
          "WHERE visit_date < %llu "
          "  AND visit_type NOT IN (0,4) "
          "LIMIT 1) TUNIONLAST "
    ") TOUTER " 
    "ORDER BY dayOrder ASC",
    MAX_DAYS+1,
    MAX_DAYS+1,
    dateName.get(),
    midnight.Get(-MAX_DAYS),
    midnight.Get(-MAX_DAYS)
    ));

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsSite()
{
  nsCAutoString localFiles;

  nsNavHistory* history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  history->GetStringFromName(NS_LITERAL_STRING("localhost").get(), localFiles);

  
  
  
  if (mConditions.IsEmpty()) {

    mQueryString = nsPrintfCString(2048,
      "SELECT DISTINCT null, "
             "'place:type=%ld&sort=%ld&domain=&domainIsHost=true', "
             "'%s', '%s', null, null, null, null, null "
      "WHERE EXISTS(SELECT '*' "
                   "FROM moz_places "
                   "WHERE hidden <> 1 AND rev_host = '.' "
                     "AND url BETWEEN 'file://' AND 'file:/~') "
      "UNION ALL "
      "SELECT DISTINCT null, "
             "'place:type=%ld&sort=%ld&domain='||host||'&domainIsHost=true', "
             "host, host, null, null, null, null, null "
      "FROM (SELECT get_unreversed_host(rev_host) host "
            "FROM (SELECT DISTINCT rev_host "
                  "FROM moz_places "
                  "WHERE hidden <> 1 AND rev_host <> '.') inner0 "
            "ORDER BY 1 ASC) inner1",
      nsINavHistoryQueryOptions::RESULTS_AS_URI,
      nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING,
      localFiles.get(),
      localFiles.get(),
      nsINavHistoryQueryOptions::RESULTS_AS_URI,
      nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING);
  
  } else {

    mQueryString = nsPrintfCString(4096,
      "SELECT DISTINCT null, "
             "'place:type=%ld&sort=%ld&domain=&domainIsHost=true"
               "&beginTime='||?1||'&endTime='||?2, "
             "'%s', '%s', null, null, null, null, null "
      "WHERE EXISTS(SELECT '*' "
                   "FROM moz_places h  "
                        "JOIN moz_historyvisits v ON h.id = v.place_id "
                   "WHERE h.hidden <> 1 AND h.rev_host = '.' "
                     "AND h.url BETWEEN 'file://' AND 'file:/~' "
                     "AND v.visit_type NOT IN (0,4) {ADDITIONAL_CONDITIONS} ) "
      "UNION ALL "
      "SELECT DISTINCT null, "
             "'place:type=%ld&sort=%ld&domain='||host||'&domainIsHost=true"
               "&beginTime='||?1||'&endTime='||?2, "
             "host, host, null, null, null, null, null "
      "FROM (SELECT get_unreversed_host(rev_host) host "
            "FROM (SELECT DISTINCT rev_host "
                  "FROM moz_places h "
                       "JOIN moz_historyvisits v ON h.id = v.place_id "
                  "WHERE h.hidden <> 1 AND h.rev_host <> '.' "
                    "AND v.visit_type NOT IN (0,4) "
                    "{ADDITIONAL_CONDITIONS} ) inner0 "
            "ORDER BY 1 ASC) inner1",
      nsINavHistoryQueryOptions::RESULTS_AS_URI,
      nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING,
      localFiles.get(),
      localFiles.get(),
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
    "SELECT null, 'place:type=%ld&queryType=%d&folder=' || id, "
      "title, null, null, null, null, null, null, dateAdded, lastModified "
    "FROM   moz_bookmarks "
    "WHERE  parent = %ld",
    nsINavHistoryQueryOptions::RESULTS_AS_URI,
    nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS,
    history->GetTagsFolder());

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::Where()
{
  
  
  PRUint32 useInnerCondition;
  useInnerCondition = mQueryString.Find("{ADDITIONAL_CONDITIONS}",0);
  if (useInnerCondition != kNotFound) {

    nsCAutoString innerCondition;
    
    if (!mConditions.IsEmpty()) {
      innerCondition = " AND ";
      innerCondition += mConditions;
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
    nsCString& queryString, PRBool& aParamsPresent)
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

    queryString = NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
        ", f.url, null, null "
      "FROM moz_places h "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.id IN ( "
        "SELECT DISTINCT p.id "
        "FROM moz_places p "
        "JOIN moz_historyvisits v ON v.place_id = p.id "
        "WHERE p.hidden <> 1 AND v.visit_type NOT IN (0,4) "
        "ORDER BY v.visit_date DESC "
        "LIMIT ");
    queryString.AppendInt(aOptions->MaxResults());
    queryString += NS_LITERAL_CSTRING(") ORDER BY 6 DESC"); 
    return NS_OK;
  }

  
  
  if (IsHistoryMenuQuery(aQueries, aOptions, 
        nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING)) {
    queryString = NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        SQL_STR_FRAGMENT_MAX_VISIT_DATE( "h.id" )
        ", f.url, null, null "
      "FROM moz_places h "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id WHERE "
      "h.id IN (SELECT p.id FROM moz_places p WHERE p.hidden <> 1 "
      " AND EXISTS (SELECT id FROM moz_historyvisits WHERE "
      " place_id = p.id AND visit_type NOT IN(0,4) LIMIT 1) "
      " ORDER BY p.visit_count DESC LIMIT ");
    queryString.AppendInt(aOptions->MaxResults());
    queryString += NS_LITERAL_CSTRING(") ORDER BY h.visit_count DESC");
    return NS_OK;
  }  

  nsCAutoString conditions;

  
  
  
  
  
  
  PRInt32 numParameters = 0;
  PRInt32 i;
  for (i = 0; i < aQueries.Count(); i ++) {
    nsCString queryClause;
    PRInt32 clauseParameters = 0;
    rv = QueryToSelectClause(aQueries[i], aOptions, numParameters,
                             &queryClause, &clauseParameters);
    NS_ENSURE_SUCCESS(rv, rv);
    if (! queryClause.IsEmpty()) {
      aParamsPresent = PR_TRUE;
      if (! conditions.IsEmpty()) 
        conditions += NS_LITERAL_CSTRING(" OR ");
      conditions += NS_LITERAL_CSTRING("(") + queryClause +
        NS_LITERAL_CSTRING(")");
      numParameters += clauseParameters;
    }
  }

  
  
  
  PRBool useLimitClause = !NeedToFilterResultSet(aQueries, aOptions);

  PlacesSQLQueryBuilder queryStringBuilder(conditions, aOptions, 
                                           useLimitClause);
  rv = queryStringBuilder.GetQueryString(queryString);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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
  nsresult rv = ConstructQueryString(aQueries, aOptions, queryString, 
                                     paramsPresent);
  NS_ENSURE_SUCCESS(rv,rv);

#ifdef DEBUG_thunder
  printf("Constructed the query: %s\n", PromiseFlatCString(queryString).get());
#endif

  
  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(queryString, getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  if (paramsPresent) {
    
    PRInt32 numParameters = 0;
    PRInt32 i;
    for (i = 0; i < aQueries.Count(); i++) {
      PRInt32 clauseParameters = 0;
      rv = BindQueryClauseParameters(statement, numParameters,
                                     aQueries[i], aOptions, &clauseParameters);
      NS_ENSURE_SUCCESS(rv, rv);
      numParameters += clauseParameters;
    }
  }

  
  
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
  return mObservers.AppendWeakElement(aObserver, aOwnsWeak);
}




NS_IMETHODIMP
nsNavHistory::RemoveObserver(nsINavHistoryObserver* aObserver)
{
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

  UpdateBatchScoper batch(*this);
  return aCallback->RunBatched(aUserData);
}

NS_IMETHODIMP
nsNavHistory::GetHistoryDisabled(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = IsHistoryDisabled();
  return NS_OK;
}











NS_IMETHODIMP
nsNavHistory::AddPageWithDetails(nsIURI *aURI, const PRUnichar *aTitle,
                                 PRInt64 aLastVisited)
{
  PRInt64 visitID;
  nsresult rv = AddVisit(aURI, aLastVisited, 0, TRANSITION_LINK, PR_FALSE,
                         0, &visitID);
  NS_ENSURE_SUCCESS(rv, rv);

  return SetPageTitleInternal(aURI, nsString(aTitle));
}








NS_IMETHODIMP
nsNavHistory::GetLastPageVisited(nsACString & aLastPageVisited)
{
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.url "
      "FROM moz_places h LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
      "WHERE v.visit_date IN "
      "(SELECT MAX(visit_date) "
       "FROM moz_historyvisits v2 LEFT JOIN moz_places h2 ON v2.place_id = h2.id "
        "WHERE h2.hidden != 1)"),
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
      "DELETE FROM moz_historyvisits WHERE place_id IN (") +
        aPlaceIdsQueryString +
        NS_LITERAL_CSTRING(")"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  (void)mExpire.OnDeleteURI();

  
  
  
  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_places WHERE id IN ("
        "SELECT h.id FROM moz_places h WHERE h.id IN (") +
        aPlaceIdsQueryString +
        NS_LITERAL_CSTRING(") AND "
        "NOT EXISTS (SELECT b.id FROM moz_bookmarks b WHERE b.fk = h.id) AND "
        "NOT EXISTS (SELECT a.id FROM moz_annos a WHERE a.place_id = h.id))"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "UPDATE moz_places SET frecency = -1 WHERE id IN(") +
        aPlaceIdsQueryString +
        NS_LITERAL_CSTRING(")"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = FixInvalidFrecenciesForExcludedPlaces();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  

  return transaction.Commit();
}










NS_IMETHODIMP
nsNavHistory::RemovePages(nsIURI **aURIs, PRUint32 aLength, PRBool aDoBatchNotify)
{
  nsresult rv;
  
  nsCString deletePlaceIdsQueryString;
  for (PRInt32 i = 0; i < aLength; i++) {
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
  nsIURI** URIs = &aURI;
  nsresult rv = RemovePages(URIs, 1, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver, OnDeleteURI(aURI))
  return NS_OK;
}















NS_IMETHODIMP
nsNavHistory::RemovePagesFromHost(const nsACString& aHost, PRBool aEntireDomain)
{
  nsresult rv;
  
  
  if (aHost.IsEmpty())
    aEntireDomain = PR_FALSE;

  
  
  nsCString localFiles;
  TitleForDomain(EmptyCString(), localFiles);
  nsAutoString host16;
  if (!aHost.Equals(localFiles))
    host16 = NS_ConvertUTF8toUTF16(aHost);

  
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
    conditionString.AssignLiteral("h.rev_host >= ?1 AND h.rev_host < ?2 ");
  else
    conditionString.AssignLiteral("h.rev_host = ?1 ");

  nsCOMPtr<mozIStorageStatement> statement;

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id FROM moz_places h WHERE ") +
      conditionString, getter_AddRefs(statement));
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

  rv = RemovePagesInternal(hostPlaceIds);
  NS_ENSURE_SUCCESS(rv, rv);

  
  UpdateBatchScoper batch(*this); 

  return NS_OK;
}










NS_IMETHODIMP
nsNavHistory::RemovePagesByTimeframe(PRTime aBeginTime, PRTime aEndTime)
{
  nsresult rv;
  
  nsCString deletePlaceIdsQueryString;

  
  
  nsCOMPtr<mozIStorageStatement> selectByTime;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id FROM moz_places h WHERE "
      "EXISTS (SELECT id FROM moz_historyvisits v WHERE v.place_id = h.id "
      " AND v.visit_date >= ?1 AND v.visit_date <= ?2 LIMIT 1)"),
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
  
  mExpire.ClearHistory();

  
  
#if 0
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("VACUUM"));
  NS_ENSURE_SUCCESS(rv, rv);
#endif
  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::HidePage(nsIURI *aURI)
{
  return NS_ERROR_NOT_IMPLEMENTED;
  



























































}












NS_IMETHODIMP
nsNavHistory::MarkPageAsTyped(nsIURI *aURI)
{
  
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
nsNavHistory::AddURI(nsIURI *aURI, PRBool aRedirect,
                     PRBool aToplevel, nsIURI *aReferrer)
{
  
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

  mExpire.OnAddURI(now);

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
                            nsIURI* aReferrer, PRInt64* aVisitID,
                            PRInt64* aSessionID, PRInt64* aRedirectBookmark)
{
  PRUint32 transitionType = 0;
  PRInt64 referringVisit = 0;
  PRTime visitTime = 0;

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

    
    
    
    
    
    rv = AddVisitChain(redirectURI, aTime - 1, aToplevel, PR_TRUE, aReferrer,
                       &referringVisit, aSessionID, aRedirectBookmark);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!aToplevel) {
      transitionType = nsINavHistoryService::TRANSITION_EMBED;
    }
  } else if (aReferrer) {
    
    
    
    PRBool referrerIsSame;
    if (NS_SUCCEEDED(aURI->Equals(aReferrer, &referrerIsSame)) && referrerIsSame)
      return NS_OK;

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (aToplevel)
      transitionType = nsINavHistoryService::TRANSITION_LINK;
    else
      transitionType = nsINavHistoryService::TRANSITION_EMBED;

    
    
    
    
    
    visitTime = PR_Now();

    
    
    if (!FindLastVisit(aReferrer, &referringVisit, aSessionID)) {
      
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

  
  return AddVisit(aURI, visitTime, aReferrer, transitionType,
                  aIsRedirect, *aSessionID, aVisitID);
}








NS_IMETHODIMP
nsNavHistory::IsVisited(nsIURI *aURI, PRBool *_retval)
{
  
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


#ifndef MOZILLA_1_8_BRANCH




NS_IMETHODIMP
nsNavHistory::GetURIGeckoFlags(nsIURI* aURI, PRUint32* aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP
nsNavHistory::SetURIGeckoFlags(nsIURI* aURI, PRUint32 aFlags)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
#endif











PLDHashOperator PR_CALLBACK nsNavHistory::ExpireNonrecentRedirects(
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

  
  
  
  if (mFrecencyUpdateIdleTime && idleTime > mFrecencyUpdateIdleTime)
    (void)RecalculateFrecencies(mNumCalculateFrecencyOnIdle, PR_TRUE);

  
  
  
  
  if (idleTime > EXPIRE_IDLE_TIME_IN_MSECS) {
    PRBool dummy;
    (void)mExpire.ExpireItems(MAX_EXPIRE_RECORDS_ON_IDLE, &dummy);
  }

  
  
  if (idleTime > LONG_IDLE_TIME_IN_MSECS) {
    
    
    PRBool oldIndexExists = PR_FALSE;
    rv = mDBConn->IndexExists(NS_LITERAL_CSTRING("moz_places_urlindex"), &oldIndexExists);
    NS_ENSURE_SUCCESS(rv, rv);
 
    if (oldIndexExists) {
      
      mozStorageTransaction urlindexTransaction(mDBConn, PR_FALSE);
      
      rv = mDBConn->ExecuteSimpleSQL(
          NS_LITERAL_CSTRING("DROP INDEX IF EXISTS moz_places_urlindex"));
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = RemoveDuplicateURIs();
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE UNIQUE INDEX moz_places_url_uniqueindex ON moz_places (url)"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = urlindexTransaction.Commit();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    nsCOMPtr<mozIStorageStatement> detectBogusIndex;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT name FROM sqlite_master WHERE type = 'index' AND "
        "name = 'moz_places_visitcount' AND sql LIKE '%rev_host%'"),
        getter_AddRefs(detectBogusIndex));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = detectBogusIndex->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = detectBogusIndex->Reset();
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasResult) {
      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "DROP INDEX IF EXISTS moz_places_visitcount"));
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
          "CREATE INDEX IF NOT EXISTS moz_places_visitcount "
          "ON moz_places (visit_count)"));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    
    
    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DELETE FROM moz_annos WHERE id IN (SELECT a.id FROM moz_annos a "
        "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
        "WHERE n.name = 'livemark/expiration')"));
    NS_ENSURE_SUCCESS(rv, rv);

#if 0
    
    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("VACUUM;"));
    NS_ENSURE_SUCCESS(rv, rv);
#endif
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
  PRInt64 visitID;
  return AddVisit(aSource, aStartTime, aReferrer, TRANSITION_DOWNLOAD, PR_FALSE,
                  0, &visitID);
}



NS_IMETHODIMP
nsNavHistory::Observe(nsISupports *aSubject, const char *aTopic,
                    const PRUnichar *aData)
{
  if (nsCRT::strcmp(aTopic, gQuitApplicationMessage) == 0) {
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

    
    
    
    (void)CleanUpOnQuit();

    
    nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
    NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
    (void)bookmarks->OnQuit();
  } else if (nsCRT::strcmp(aTopic, gXpcomShutdown) == 0) {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    observerService->RemoveObserver(this, gAutoCompleteFeedback);
    observerService->RemoveObserver(this, gXpcomShutdown);
    observerService->RemoveObserver(this, gQuitApplicationMessage);
  } else if (nsCRT::strcmp(aTopic, gAutoCompleteFeedback) == 0) {
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
  } else if (nsCRT::strcmp(aTopic, "nsPref:changed") == 0) {
    PRInt32 oldDaysMin = mExpireDaysMin;
    PRInt32 oldDaysMax = mExpireDaysMax;
    PRInt32 oldVisits = mExpireSites;
    LoadPrefs(PR_FALSE);
    if (oldDaysMin != mExpireDaysMin || oldDaysMax != mExpireDaysMax ||
        oldVisits != mExpireSites)
      mExpire.OnExpirationChanged();
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
          nsCString spec;
          message.uri->GetSpec(spec);
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












nsresult
nsNavHistory::QueryToSelectClause(nsNavHistoryQuery* aQuery, 
                                  nsNavHistoryQueryOptions* aOptions,
                                  PRInt32 aStartParameter,
                                  nsCString* aClause,
                                  PRInt32* aParamCount)
{
  PRBool hasIt;

  aClause->Truncate();
  *aParamCount = 0;
  nsCAutoString paramString;

  
  if (NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) {
    parameterString(aStartParameter + *aParamCount, paramString);
    *aClause += NS_LITERAL_CSTRING("v.visit_date >= ") + paramString;
    (*aParamCount) ++;
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt) {
    if (! aClause->IsEmpty())
      *aClause += NS_LITERAL_CSTRING(" AND ");
    parameterString(aStartParameter + *aParamCount, paramString);
    *aClause += NS_LITERAL_CSTRING("v.visit_date <= ") + paramString;
    (*aParamCount) ++;
  }

  

  
  if (aQuery->MinVisits() >= 0) {
    if (! aClause->IsEmpty())
      *aClause += NS_LITERAL_CSTRING(" AND ");
    parameterString(aStartParameter + *aParamCount, paramString);
    *aClause += NS_LITERAL_CSTRING("h.visit_count >= ") + paramString;
    (*aParamCount) ++;
  }

  if (aQuery->MaxVisits() >= 0) {
    if (! aClause->IsEmpty())
      *aClause += NS_LITERAL_CSTRING(" AND ");
    parameterString(aStartParameter + *aParamCount, paramString);
    *aClause += NS_LITERAL_CSTRING("h.visit_count <= ") + paramString;
    (*aParamCount) ++;
  }

  
  if (aOptions->QueryType() != nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS &&
      aQuery->OnlyBookmarked()) {
    
    if (!aClause->IsEmpty())
      *aClause += NS_LITERAL_CSTRING(" AND ");

    *aClause += NS_LITERAL_CSTRING("EXISTS (SELECT b.fk FROM moz_bookmarks b WHERE b.type = ") +
                nsPrintfCString("%d", nsNavBookmarks::TYPE_BOOKMARK) +
                NS_LITERAL_CSTRING(" AND b.fk = h.id)");
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasDomain(&hasIt)) && hasIt) {
    if (! aClause->IsEmpty())
      *aClause += NS_LITERAL_CSTRING(" AND ");

    PRBool domainIsHost = PR_FALSE;
    aQuery->GetDomainIsHost(&domainIsHost);
    if (domainIsHost) {
      parameterString(aStartParameter + *aParamCount, paramString);
      *aClause += NS_LITERAL_CSTRING("h.rev_host = ") + paramString;
      aClause->Append(' ');
      (*aParamCount) ++;
    } else {
      
      parameterString(aStartParameter + *aParamCount, paramString);
      *aClause += NS_LITERAL_CSTRING("h.rev_host >= ") + paramString;
      (*aParamCount) ++;

      parameterString(aStartParameter + *aParamCount, paramString);
      *aClause += NS_LITERAL_CSTRING(" AND h.rev_host < ") + paramString;
      aClause->Append(' ');
      (*aParamCount) ++;
    }
  }

  
  
  
  
  
  
  
  if (NS_SUCCEEDED(aQuery->GetHasUri(&hasIt)) && hasIt) {
    if (! aClause->IsEmpty())
      *aClause += NS_LITERAL_CSTRING(" AND ");

    nsCAutoString paramString;
    parameterString(aStartParameter + *aParamCount, paramString);
    (*aParamCount) ++;

    nsCAutoString match;
    if (aQuery->UriIsPrefix()) {
      
      *aClause += NS_LITERAL_CSTRING("SUBSTR(h.url, 0, LENGTH(") +
        paramString + NS_LITERAL_CSTRING(")) = ") + paramString;
    } else {
      *aClause += NS_LITERAL_CSTRING("h.url = ") + paramString;
    }
    aClause->Append(' ');
  }

  
  aQuery->GetHasAnnotation(&hasIt);
  if (hasIt) {
    if (! aClause->IsEmpty())
      *aClause += NS_LITERAL_CSTRING(" AND ");

    nsCAutoString paramString;
    parameterString(aStartParameter + *aParamCount, paramString);
    (*aParamCount) ++;

    if (aQuery->AnnotationIsNot())
      aClause->AppendLiteral("NOT ");
    aClause->AppendLiteral("EXISTS (SELECT h.id FROM moz_annos anno JOIN moz_anno_attributes annoname ON anno.anno_attribute_id = annoname.id WHERE anno.place_id = h.id AND annoname.name = ");
    aClause->Append(paramString);
    aClause->AppendLiteral(") ");
    
    
  }

  return NS_OK;
}






nsresult
nsNavHistory::BindQueryClauseParameters(mozIStorageStatement* statement,
                                        PRInt32 aStartParameter,
                                        nsNavHistoryQuery* aQuery, 
                                        nsNavHistoryQueryOptions* aOptions,
                                        PRInt32* aParamCount)
{
  nsresult rv;
  (*aParamCount) = 0;

  PRBool hasIt;

  
  if (NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) {
    PRTime time = NormalizeTime(aQuery->BeginTimeReference(),
                                aQuery->BeginTime());
    rv = statement->BindInt64Parameter(aStartParameter + *aParamCount, time);
    NS_ENSURE_SUCCESS(rv, rv);
    (*aParamCount) ++;
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt) {
    PRTime time = NormalizeTime(aQuery->EndTimeReference(),
                                aQuery->EndTime());
    rv = statement->BindInt64Parameter(aStartParameter + *aParamCount, time);
    NS_ENSURE_SUCCESS(rv, rv);
    (*aParamCount) ++;
  }

  

  
  PRInt32 visits = aQuery->MinVisits();
  if (visits >= 0) {
    rv = statement->BindInt32Parameter(aStartParameter + *aParamCount, visits);
    NS_ENSURE_SUCCESS(rv, rv);
    (*aParamCount) ++;
  }

  visits = aQuery->MaxVisits();
  if (visits >= 0) {
    rv = statement->BindInt32Parameter(aStartParameter + *aParamCount, visits);
    NS_ENSURE_SUCCESS(rv, rv);
    (*aParamCount) ++;
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasDomain(&hasIt)) && hasIt) {
    nsString revDomain;
    GetReversedHostname(NS_ConvertUTF8toUTF16(aQuery->Domain()), revDomain);

    if (aQuery->DomainIsHost()) {
      rv = statement->BindStringParameter(aStartParameter + *aParamCount, revDomain);
      NS_ENSURE_SUCCESS(rv, rv);
      (*aParamCount) ++;
    } else {
      
      
      
      NS_ASSERTION(revDomain[revDomain.Length() - 1] == '.', "Invalid rev. host");
      rv = statement->BindStringParameter(aStartParameter + *aParamCount, revDomain);
      NS_ENSURE_SUCCESS(rv, rv);
      (*aParamCount) ++;
      revDomain.Truncate(revDomain.Length() - 1);
      revDomain.Append(PRUnichar('/'));
      rv = statement->BindStringParameter(aStartParameter + *aParamCount, revDomain);
      NS_ENSURE_SUCCESS(rv, rv);
      (*aParamCount) ++;
    }
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasUri(&hasIt)) && hasIt) {
    BindStatementURI(statement, aStartParameter + *aParamCount, aQuery->Uri());
    (*aParamCount) ++;
  }

  
  aQuery->GetHasAnnotation(&hasIt);
  if (hasIt) {
    rv = statement->BindUTF8StringParameter(aStartParameter + *aParamCount,
                                            aQuery->Annotation());
    NS_ENSURE_SUCCESS(rv, rv);
  }

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

  
  nsTArray<nsStringArray*> terms;
  ParseSearchTermsFromQueries(aQueries, &terms);

  PRUint32 queryIndex;

  
  
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

    
    
    if (!parentAnnotationToExclude.IsEmpty() && (parentFoldersToExclude.IndexOf(parentId) != -1))
      continue;

    
    PRBool appendNode = PR_FALSE;
    for (queryIndex = 0;
         queryIndex < aQueries.Count() && !appendNode; queryIndex++) {
      
      if (includeFolders[queryIndex]->Length() != 0) {
        
        if (aSet[nodeIndex]->mItemId == -1)
          continue;

        
        
        if (excludeFolders[queryIndex]->IndexOf(parentId) != -1)
          continue;

        if (includeFolders[queryIndex]->IndexOf(parentId) == -1) {
          
          PRInt64 ancestor = parentId, lastAncestor;
          PRBool belongs = PR_FALSE;
          nsTArray<PRInt64> ancestorFolders;

          while (!belongs) {
            
            lastAncestor = ancestor;
            ancestorFolders.AppendElement(ancestor);

            
            if (NS_FAILED(bookmarks->GetFolderIdForItem(ancestor,&ancestor))) {
              break;
            } else if (excludeFolders[queryIndex]->IndexOf(ancestor) != -1) {
              break;
            } else if (includeFolders[queryIndex]->IndexOf(ancestor) != -1) {
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
      rv = mDBGetTags->BindInt32Parameter(0, GetTagsFolder());
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mDBGetTags->BindUTF8StringParameter(1, aSet[nodeIndex]->mURI);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString nodeTags;
      PRBool hasTag = PR_FALSE;
      if (NS_SUCCEEDED(mDBGetTags->ExecuteStep(&hasTag)) && hasTag) {
        rv = mDBGetTags->GetString(0, nodeTags);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      
      PRBool matchAll = PR_TRUE;
      for (PRInt32 termIndex = terms[queryIndex]->Count(); --termIndex >= 0 &&
           matchAll; ) {
        const nsString *term = terms[queryIndex]->StringAt(termIndex);

        
        matchAll = CaseInsensitiveFindInReadable(*term, nodeTitle) ||
                   CaseInsensitiveFindInReadable(*term, nodeURL) ||
                   CaseInsensitiveFindInReadable(*term, nodeTags);
      }

      
      if (!matchAll)
        continue;

      appendNode = PR_TRUE;
    }
    if (appendNode)
      aFiltered->AppendObject(aSet[nodeIndex]);
      
    
    if (aOptions->MaxResults() > 0 && 
        aFiltered->Count() >= aOptions->MaxResults())
      break;
  }

  
  for (PRUint32 i=0; i < aQueries.Count(); i++) {
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






PR_STATIC_CALLBACK(PLDHashOperator)
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
    return rv;
  } else if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_URI) {
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
      
      statement = mDBVisitToVisitResult;
      break;

    case nsNavHistoryQueryOptions::RESULTS_AS_URI:
      
      statement = mDBVisitToURLResult;
      break;

    default:
      
      
      return NS_OK;
  }

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
  mozStorageStatementScoper scoper(mDBBookmarkToUrlResult);
  nsresult rv = mDBBookmarkToUrlResult->BindInt64Parameter(0, aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = mDBBookmarkToUrlResult->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid bookmark identifier");
    return NS_ERROR_INVALID_ARG;
  }

  return RowToResult(mDBBookmarkToUrlResult, aOptions, aResult);
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
nsNavHistory::GetAgeInDaysString(PRInt32 aInt, const PRUnichar *aName, nsACString& aResult)
{
  nsAutoString intString;
  intString.AppendInt(aInt);
  const PRUnichar* strings[1] = { intString.get() };
  nsXPIDLString value;
  nsresult rv = mBundle->FormatStringFromName(aName, strings, 
                                              1, getter_Copies(value));
  if (NS_SUCCEEDED(rv))
    CopyUTF16toUTF8(value, aResult);
  else
    aResult.Truncate(0);
}

void
nsNavHistory::GetStringFromName(const PRUnichar *aName, nsACString& aResult)
{
  nsXPIDLString value;
  nsresult rv = mBundle->GetStringFromName(aName, getter_Copies(value));
  if (NS_SUCCEEDED(rv))
    CopyUTF16toUTF8(value, aResult);
  else
    aResult.Truncate(0);
}










nsresult
nsNavHistory::SetPageTitleInternal(nsIURI* aURI, const nsAString& aTitle)
{
  nsresult rv;

  mozStorageTransaction transaction(mDBConn, PR_TRUE);

  
  
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

  nsCOMPtr<mozIStorageStatement> dbModStatement;
  title = aTitle;
  rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("UPDATE moz_places SET title = ?1 WHERE url = ?2"),
      getter_AddRefs(dbModStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aTitle.IsVoid())
    dbModStatement->BindNullParameter(0);
  else
    dbModStatement->BindStringParameter(0, StringHead(aTitle, HISTORY_TITLE_LENGTH_MAX));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = BindStatementURI(dbModStatement, 1, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbModStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                      OnTitleChanged(aURI, title))

  return NS_OK;

}

nsresult
nsNavHistory::AddPageWithVisit(nsIURI *aURI,
                               const nsString &aTitle,
                               PRBool aHidden, PRBool aTyped,
                               PRInt32 aVisitCount,
                               PRInt32 aLastVisitTransition,
                               PRTime aLastVisitDate)
{
  PRBool canAdd = PR_FALSE;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    return NS_OK;
  }

  PRInt64 pageID;
  
  rv = InternalAddNewPage(aURI, aTitle, aHidden, aTyped, aVisitCount, PR_FALSE, &pageID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aLastVisitDate != -1) {
    PRInt64 visitID;
    rv = InternalAddVisit(pageID, 0, 0,
                          aLastVisitDate, aLastVisitTransition, &visitID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
nsNavHistory::RemoveDuplicateURIs()
{
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  
  
  nsCOMPtr<mozIStorageStatement> selectStatement;
  nsresult rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("SELECT "
        "(SELECT h.id FROM moz_places h WHERE h.url=url ORDER BY h.visit_count DESC LIMIT 1), "
        "url, SUM(visit_count) "
        "FROM moz_places "
        "GROUP BY url HAVING( COUNT(url) > 1)"),
      getter_AddRefs(selectStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> updateStatement;
  rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING(
        "UPDATE moz_historyvisits "
        "SET place_id = ?1 "
        "WHERE place_id IN (SELECT id FROM moz_places WHERE id <> ?1 AND url = ?2)"),
      getter_AddRefs(updateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> bookmarkStatement;
  rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING(
        "UPDATE moz_bookmarks "
        "SET fk = ?1 "
        "WHERE fk IN (SELECT id FROM moz_places WHERE id <> ?1 AND url = ?2)"),
      getter_AddRefs(bookmarkStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> annoStatement;
  rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING(
        "UPDATE moz_annos "
        "SET place_id = ?1 "
        "WHERE place_id IN (SELECT id FROM moz_places WHERE id <> ?1 AND url = ?2)"),
      getter_AddRefs(annoStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  nsCOMPtr<mozIStorageStatement> deleteStatement;
  rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("DELETE FROM moz_places WHERE url = ?1 AND id <> ?2"),
      getter_AddRefs(deleteStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> countStatement;
  rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("UPDATE moz_places SET visit_count = ?1 WHERE id = ?2"),
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

  
  nsAutoString forward = NS_ConvertUTF8toUTF16(forward8);
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

  
  
  NS_ASSERTION(query->Folders()[0] > 0, "bad folder id");
  return query->Folders()[0];
}













inline PRBool isQueryWhitespace(PRUnichar ch)
{
  return ch == ' ';
}

void ParseSearchTermsFromQueries(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                 nsTArray<nsStringArray*>* aTerms)
{
  PRInt32 lastBegin = -1;
  for (PRUint32 i=0; i < aQueries.Count(); i++) {
    nsStringArray *queryTerms = new nsStringArray();
    PRBool hasSearchTerms;
    if (NS_SUCCEEDED(aQueries[i]->GetHasSearchTerms(&hasSearchTerms)) &&
        hasSearchTerms) {
      const nsString& searchTerms = aQueries[i]->SearchTerms();
      for (PRUint32 j = 0; j < searchTerms.Length(); j++) {
        if (isQueryWhitespace(searchTerms[j]) ||
            searchTerms[j] == '"') {
          if (lastBegin >= 0) {
            
            queryTerms->AppendString(Substring(searchTerms, lastBegin,
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
        queryTerms->AppendString(Substring(searchTerms, lastBegin));
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
  aTitle = NS_ConvertUTF8toUTF16(name);
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

  
  
  
  rv = CalculateVisitCount(aPlaceId, PR_TRUE ,
                           &visitCountForFrecency);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 newFrecency = 0;
  rv = CalculateFrecencyInternal(aPlaceId, typed, visitCountForFrecency,
                                 aIsBookmarked, &newFrecency);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  if (newFrecency == oldFrecency || oldFrecency && newFrecency == -1)
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
nsNavHistory::CalculateFrecencyInternal(PRInt64 aPlaceId, PRInt32 aTyped, PRInt32 aVisitCount, PRBool aIsBookmarked, PRInt32 *aFrecency)
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

        pointsForSampledVisits += weight * (bonus / 100.0);
      }
    }

    if (numSampledVisits) {
      
      if (!pointsForSampledVisits) {
        
        
        
        PRInt32 trueVisitCount = 0;
        rv = CalculateVisitCount(aPlaceId, PR_FALSE ,
                                 &trueVisitCount);
        if (NS_SUCCEEDED(rv) && trueVisitCount)
          *aFrecency = -1;
        else
          *aFrecency = 0;
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
nsNavHistory::CalculateFrecency(PRInt64 aPlaceId, PRInt32 aTyped, PRInt32 aVisitCount, nsCAutoString &aURL, PRInt32 *aFrecency)
{
  *aFrecency = 0;

  nsresult rv;

  nsCOMPtr<nsILivemarkService> lms = 
    do_GetService(NS_LIVEMARKSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isBookmark = PR_FALSE;

  
  
  if (!IsQueryURI(aURL) && aPlaceId != -1) {
    mozStorageStatementScoper scope(mDBGetBookmarkParentsForPlace);

    rv = mDBGetBookmarkParentsForPlace->BindInt64Parameter(0, aPlaceId);
    NS_ENSURE_SUCCESS(rv, rv);
   
    
    
    
    PRBool hasMore = PR_FALSE;
    while (NS_SUCCEEDED(mDBGetBookmarkParentsForPlace->ExecuteStep(&hasMore)) 
           && hasMore) {
      PRInt64 folderId;
      rv = mDBGetBookmarkParentsForPlace->GetInt64(0, &folderId);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool parentIsLivemark;
      rv = lms->IsLivemark(folderId, &parentIsLivemark);
      NS_ENSURE_SUCCESS(rv, rv);

      
      if (!parentIsLivemark) {
        isBookmark = PR_TRUE;
        break;
      }
    }
  }

  rv = CalculateFrecencyInternal(aPlaceId, aTyped, aVisitCount,
                                 isBookmark, aFrecency);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
nsNavHistory::RecalculateFrecencies(PRInt32 aCount, PRBool aRecalcOld)
{
  mozStorageTransaction transaction(mDBConn, PR_TRUE);

  nsresult rv = RecalculateFrecenciesInternal(mDBInvalidFrecencies, aCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aRecalcOld) {
    rv = RecalculateFrecenciesInternal(mDBOldFrecencies, aCount);
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

    
    
    rv = CalculateVisitCount(placeId, PR_TRUE ,
                             &visitCountForFrecency);
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
