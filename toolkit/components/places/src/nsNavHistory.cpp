





































#include <stdio.h>
#include "nsNavHistory.h"
#include "nsNavBookmarks.h"
#include "nsMorkHistoryImporter.h"
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

#include "mozIStorageService.h"
#include "mozIStorageConnection.h"
#include "mozIStorageValueArray.h"
#include "mozIStorageStatement.h"
#include "mozIStorageFunction.h"
#include "mozStorageCID.h"
#include "mozStorageHelper.h"
#include "nsAppDirectoryServiceDefs.h"




#define RECENT_EVENT_THRESHOLD (15 * 60 * 1000000)



#define BOOKMARK_REDIRECT_TIME_THRESHOLD (2 * 60 * 100000)






#define RECENT_EVENT_QUEUE_MAX_LENGTH 128


#define PREF_BRANCH_BASE                        "browser."
#define PREF_BROWSER_HISTORY_EXPIRE_DAYS        "history_expire_days"
#define PREF_AUTOCOMPLETE_ONLY_TYPED            "urlbar.matchOnlyTyped"
#define PREF_AUTOCOMPLETE_ENABLED               "urlbar.autocomplete.enabled"
#define PREF_DB_CACHE_PERCENTAGE                "history_cache_percentage"
#define PREF_BROWSER_IMPORT_BOOKMARKS           "browser.places.importBookmarksHTML"






#define DEFAULT_DB_CACHE_PERCENTAGE 6





#define DEFAULT_DB_PAGE_SIZE 1024


#define HISTORY_EXPIRE_NOW_TIMEOUT (3 * PR_MSEC_PER_SEC)



#define HISTORY_URI_LENGTH_MAX 65536
#define HISTORY_TITLE_LENGTH_MAX 4096


#define DB_FILENAME NS_LITERAL_STRING("places.sqlite")


#define DB_CORRUPT_FILENAME NS_LITERAL_STRING("places.sqlite.corrupt")



#ifdef LAZY_ADD


#define LAZY_MESSAGE_TIMEOUT (3 * PR_MSEC_PER_SEC)



#define MAX_LAZY_TIMER_DEFERMENTS 2

#endif 

NS_IMPL_ADDREF(nsNavHistory)
NS_IMPL_RELEASE(nsNavHistory)

NS_INTERFACE_MAP_BEGIN(nsNavHistory)
  NS_INTERFACE_MAP_ENTRY(nsINavHistoryService)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsIGlobalHistory2, nsIGlobalHistory3)
  NS_INTERFACE_MAP_ENTRY(nsIGlobalHistory3)
  NS_INTERFACE_MAP_ENTRY(nsIBrowserHistory)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIAutoCompleteSearch)
  NS_INTERFACE_MAP_ENTRY(nsIAutoCompleteSimpleResultListener)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsINavHistoryService)
NS_INTERFACE_MAP_END

static nsresult GetReversedHostname(nsIURI* aURI, nsAString& host);
static void GetReversedHostname(const nsString& aForward, nsAString& aReversed);
static void GetSubstringFromNthDot(const nsCString& aInput, PRInt32 aStartingSpot,
                                   PRInt32 aN, PRBool aIncludeDot,
                                   nsACString& aSubstr);
static nsresult GenerateTitleFromURI(nsIURI* aURI, nsAString& aTitle);
static PRInt32 GetTLDCharCount(const nsCString& aHost);
static PRInt32 GetTLDType(const nsCString& aHostTail);
static PRBool IsNumericHostName(const nsCString& aHost);
static PRInt64 GetSimpleBookmarksQueryFolder(
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions);
static void ParseSearchQuery(const nsString& aQuery, nsStringArray* aTerms);

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
const PRInt32 nsNavHistory::kAutoCompleteIndex_VisitCount = 2;
const PRInt32 nsNavHistory::kAutoCompleteIndex_Typed = 3;

static nsDataHashtable<nsCStringHashKey, int>* gTldTypes;
static const char* gQuitApplicationMessage = "quit-application";
static const char* gXpcomShutdown = "xpcom-shutdown";


const char nsNavHistory::kAnnotationPreviousEncoding[] = "history/encoding";


nsNavHistory* nsNavHistory::gHistoryService;



nsNavHistory::nsNavHistory() : mNowValid(PR_FALSE),
                               mExpireNowTimer(nsnull),
                               mExpire(this),
                               mBatchesInProgress(0),
                               mAutoCompleteOnlyTyped(PR_FALSE),
                               mExpireDays(0)
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

  gExpandedItems.Init(128);

  
  nsCOMPtr<nsIPrefService> prefService =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prefService->GetBranch(PREF_BRANCH_BASE, getter_AddRefs(mPrefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = InitDBFile(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool doImport;
  rv = InitDB(&doImport);
  if (NS_FAILED(rv)) {
    
    
    rv = InitDBFile(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = InitDB(&doImport);
  }
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef IN_MEMORY_LINKS
  rv = InitMemDB();
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  rv = InitAutoComplete();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  {
    nsCOMPtr<mozIStorageStatement> selectSession;
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT MAX(session) FROM "
        "(SELECT MAX(visit_date) AS visit_date FROM moz_historyvisits) maxvd "
        "JOIN moz_historyvisits v ON maxvd.visit_date = v.visit_date"),
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
      "chrome://browser/locale/places/places.properties",
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

  
  LoadPrefs();

  
  NS_ENSURE_TRUE(mRecentTyped.Init(128), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentBookmark.Init(128), NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentRedirects.Init(128), NS_ERROR_OUT_OF_MEMORY);

  rv = CreateLookupIndexes();
  if (NS_FAILED(rv))
    return rv;

  
  
  
  

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch2> pbi = do_QueryInterface(mPrefBranch);
  if (pbi) {
    pbi->AddObserver(PREF_AUTOCOMPLETE_ONLY_TYPED, this, PR_FALSE);
    pbi->AddObserver(PREF_BROWSER_HISTORY_EXPIRE_DAYS, this, PR_FALSE);
  }

  observerService->AddObserver(this, gQuitApplicationMessage, PR_FALSE);
  observerService->AddObserver(this, gXpcomShutdown, PR_FALSE);

  if (doImport) {
    nsCOMPtr<nsIMorkHistoryImporter> importer = new nsMorkHistoryImporter();
    NS_ENSURE_TRUE(importer, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIFile> historyFile;
    rv = NS_GetSpecialDirectory(NS_APP_HISTORY_50_FILE,
                                getter_AddRefs(historyFile));
    if (NS_SUCCEEDED(rv) && historyFile) {
      importer->ImportHistory(historyFile, this);
    }
  }

  
  

  return NS_OK;
}





nsresult
nsNavHistory::BackupDBFile()
{
  
  nsCOMPtr<nsIFile> profDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(profDir));
  
  nsCOMPtr<nsIFile> corruptBackup;
  rv = profDir->Clone(getter_AddRefs(corruptBackup));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = corruptBackup->Append(DB_CORRUPT_FILENAME);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = corruptBackup->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);
  return mDBFile->MoveTo(profDir, DB_CORRUPT_FILENAME);
}
  

nsresult
nsNavHistory::InitDBFile(PRBool aForceInit)
{
  
  nsCOMPtr<nsIFile> profDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(profDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = profDir->Clone(getter_AddRefs(mDBFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBFile->Append(DB_FILENAME);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  if (aForceInit) {
    rv = BackupDBFile();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRBool dbExists;
  rv = mDBFile->Exists(&dbExists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  mDBService = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBService->OpenDatabase(mDBFile, getter_AddRefs(mDBConn));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    dbExists = PR_FALSE;
  
    
    rv = BackupDBFile();
    NS_ENSURE_SUCCESS(rv, rv);
  
    
    rv = profDir->Clone(getter_AddRefs(mDBFile));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBFile->Append(DB_FILENAME);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBService->OpenDatabase(mDBFile, getter_AddRefs(mDBConn));
  }
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  if (!dbExists) {
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService("@mozilla.org/preferences-service;1"));
    if (prefs) {
      rv = prefs->SetBoolPref(PREF_BROWSER_IMPORT_BOOKMARKS, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  
  return NS_OK;
}





#define PLACES_SCHEMA_VERSION 5

nsresult
nsNavHistory::InitDB(PRBool *aDoImport)
{
  nsresult rv;
  PRBool tableExists;
  *aDoImport = PR_FALSE;

  
  
  
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

      

    } else {
      

      
      

      

      
      
      if (DBSchemaVersion > 2) {
        
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
    *aDoImport = PR_TRUE;
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING("CREATE TABLE moz_places ("
        "id INTEGER PRIMARY KEY, "
        "url LONGVARCHAR, "
        "title LONGVARCHAR, "
        "user_title LONGVARCHAR, "
        "rev_host LONGVARCHAR, "
        "visit_count INTEGER DEFAULT 0, "
        "hidden INTEGER DEFAULT 0 NOT NULL, "
        "typed INTEGER DEFAULT 0 NOT NULL, "
        "favicon_id INTEGER)"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE INDEX moz_places_urlindex ON moz_places (url)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  
  mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE INDEX moz_places_faviconindex ON moz_places (favicon_id)"));

  
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

    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("CREATE INDEX moz_historyvisits_pageindex ON moz_historyvisits (place_id)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  
  mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "CREATE INDEX moz_historyvisits_fromindex ON moz_historyvisits (from_visit)"));

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  

  
  
  
  if (cachePages > 0) {
    rv = mDBConn->Preload();
    if (NS_FAILED(rv))
      NS_WARNING("Preload of database failed");
  }

  

  rv = InitStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




nsresult
nsNavHistory::UpdateSchemaVersion()
{
  return mDBConn->SetSchemaVersion(PLACES_SCHEMA_VERSION);
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
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "(SELECT MAX(visit_date) FROM moz_historyvisits WHERE place_id = h.id), "
        "f.url "
      "FROM moz_places h "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.url = ?1 "),
    getter_AddRefs(mDBGetURLPageInfoFull));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count "
      "FROM moz_places h WHERE h.id = ?1"),
                                getter_AddRefs(mDBGetIdPageInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "(SELECT MAX(visit_date) FROM moz_historyvisits WHERE place_id = h.id), "
        "f.url "
      "FROM moz_places h "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.id = ?1"),
    getter_AddRefs(mDBGetIdPageInfoFull));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.url, h.title, h.visit_count, h.typed "
      "FROM moz_places h "
      "JOIN moz_historyvisits v ON h.id = v.place_id "
      "WHERE h.hidden <> 1 "
      "GROUP BY h.id "
      "ORDER BY h.visit_count "
      "LIMIT ?1"),
    getter_AddRefs(mDBFullAutoComplete));
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
      "INSERT INTO moz_places "
      "(url, title, rev_host, hidden, typed, visit_count) "
      "VALUES (?1, ?2, ?3, ?4, ?5, ?6)"),
    getter_AddRefs(mDBAddNewPage));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
        "(SELECT MAX(visit_date) FROM moz_historyvisits WHERE place_id = h.id), "
        "f.url, null, null "
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
        "(SELECT MAX(visit_date) FROM moz_historyvisits WHERE place_id = h.id), "
        "f.url, null, null "
      "FROM moz_places h "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE h.url = ?1"),
    getter_AddRefs(mDBUrlToUrlResult));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT b.fk, h.url, b.title, h.rev_host, h.visit_count, "
        "(SELECT MAX(visit_date) FROM moz_historyvisits WHERE place_id = b.fk), "
        "f.url, null, null, b.dateAdded, b.lastModified "
      "FROM moz_bookmarks b "
      "JOIN moz_places h ON b.fk = h.id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
      "WHERE b.id = ?1"),
    getter_AddRefs(mDBBookmarkToUrlResult));
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

#ifdef IN_MEMORY_LINKS




nsresult
nsNavHistory::InitMemDB()
{
  nsresult rv = mDBService->OpenSpecialDatabase("memory", getter_AddRefs(mMemDBConn));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMemDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE TABLE moz_memhistory (url LONGVARCHAR UNIQUE)"));
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







void
nsNavHistory::SaveExpandItem(const nsAString& aTitle)
{
  gExpandedItems.Put(aTitle, 1);
}













nsresult
nsNavHistory::GetUrlIdFor(nsIURI* aURI, PRInt64* aEntryID,
                          PRBool aAutoCreate)
{
  *aEntryID = 0;

  mozStorageTransaction transaction(mDBConn, PR_FALSE);
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
    rv = InternalAddNewPage(aURI, voidString, PR_TRUE, PR_FALSE, 0, aEntryID);
    if (NS_SUCCEEDED(rv))
      transaction.Commit();
    return rv;
  } else {
    
    return NS_OK;
  }
}









void
nsNavHistory::SaveCollapseItem(const nsAString& aTitle)
{
  gExpandedItems.Remove(aTitle);
}









nsresult
nsNavHistory::InternalAddNewPage(nsIURI* aURI, const nsAString& aTitle,
                                 PRBool aHidden, PRBool aTyped,
                                 PRInt32 aVisitCount, PRInt64* aPageID)
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
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  rv = mDBRecentVisitOfURL->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
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

  
  nsresult rv;
  mozStorageStatementScoper statementResetter(mDBGetURLPageInfo);
  rv = mDBGetURLPageInfo->BindUTF8StringParameter(0, aURIString);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool hasMore = PR_FALSE;
  rv = mDBGetURLPageInfo->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  if (! hasMore)
    return PR_FALSE;

  
  
  
  PRInt32 visitCount;
  rv = mDBGetURLPageInfo->GetInt32(kGetInfoIndex_VisitCount, &visitCount);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return visitCount > 0;
#endif
}




nsresult
nsNavHistory::LoadPrefs()
{
  if (! mPrefBranch)
    return NS_OK;

  mPrefBranch->GetIntPref(PREF_BROWSER_HISTORY_EXPIRE_DAYS, &mExpireDays);
  PRBool oldCompleteOnlyTyped = mAutoCompleteOnlyTyped;
  mPrefBranch->GetBoolPref(PREF_AUTOCOMPLETE_ONLY_TYPED,
                           &mAutoCompleteOnlyTyped);
  if (oldCompleteOnlyTyped != mAutoCompleteOnlyTyped) {
    
    nsresult rv = CreateAutoCompleteQuery();
    NS_ENSURE_SUCCESS(rv, rv);
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

  
  
  
  if (aOptions->MaxResults() > 0)
    return QUERYUPDATE_COMPLEX;

  PRBool nonTimeBasedItems = PR_FALSE;
  for (i = 0; i < aQueries.Count(); i ++) {
    nsNavHistoryQuery* query = aQueries[i];

    if (query->Folders().Length() > 0 || query->OnlyBookmarked()) {
      return QUERYUPDATE_COMPLEX_WITH_BOOKMARKS;
    }
    
    
    if (! query->SearchTerms().IsEmpty() ||
        ! query->Domain().IsVoid() ||
        query->Uri() != nsnull)
      nonTimeBasedItems = PR_TRUE;
  }

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
      nsCOMArray<nsNavHistoryResultNode> filteredSet;
      nsresult rv = FilterResultSet(inputSet, &filteredSet, query->SearchTerms());
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
      nsCAutoString host;
      if (NS_FAILED(nodeUri->GetAsciiHost(host)))
        continue;
      nsCAutoString asciiRequest;
      if (NS_FAILED(AsciiHostNameFromHostString(query->Domain(), asciiRequest)))
        continue;

      if (query->DomainIsHost()) {
        if (! asciiRequest.Equals(host))
          continue; 
      }
      
      nsCAutoString domain;
      DomainNameFromHostName(host, domain);
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
        NS_ENSURE_SUCCESS(rv, rv);
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
nsNavHistory::DomainNameFromHostName(const nsCString& aHostName,
                                     nsACString& aDomainName)
{
  if (IsNumericHostName(aHostName)) {
    
    aDomainName = aHostName;
  } else {
    
    PRInt32 tldLength = GetTLDCharCount(aHostName);
    if (tldLength < PRInt32(aHostName.Length())) {
      
      GetSubstringFromNthDot(aHostName,
                             aHostName.Length() - tldLength - 2,
                             1, PR_FALSE, aDomainName);
    }
  }
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




NS_IMETHODIMP
nsNavHistory::SetPageUserTitle(nsIURI* aURI, const nsAString& aUserTitle)
{
  return SetPageTitleInternal(aURI, PR_TRUE, aUserTitle);
}






NS_IMETHODIMP
nsNavHistory::MarkPageAsFollowedBookmark(nsIURI* aURI)
{
  
  if (IsHistoryDisabled())
    return NS_OK;

  nsCAutoString uriString;
  aURI->GetSpec(uriString);

  
  PRInt64 unusedEventTime;
  if (mRecentBookmark.Get(uriString, &unusedEventTime))
    mRecentBookmark.Remove(uriString);

  if (mRecentBookmark.Count() > RECENT_EVENT_QUEUE_MAX_LENGTH)
    ExpireNonrecentEvents(&mRecentBookmark);

  mRecentTyped.Put(uriString, GetNow());
  return NS_OK;
}










nsresult
nsNavHistory::CanAddURI(nsIURI* aURI, PRBool* canAdd)
{
  nsresult rv;

  nsCString scheme;
  rv = aURI->GetScheme(scheme);
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
nsNavHistory::SetPageDetails(nsIURI* aURI, const nsAString& aTitle,
                             const nsAString& aUserTitle, PRUint32 aVisitCount,
                             PRBool aHidden, PRBool aTyped)
{
  
  PRInt64 pageID;
  nsresult rv = GetUrlIdFor(aURI, &pageID, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET title = ?2, "
          "user_title = ?3, "
          "visit_count = ?4, "
          "hidden = ?5, "
          "typed = ?6 "
       "WHERE id = ?1"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindInt64Parameter(0, pageID);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (aTitle.IsVoid())
    statement->BindNullParameter(1);
  else
    statement->BindStringParameter(1, StringHead(aTitle, HISTORY_TITLE_LENGTH_MAX));
  NS_ENSURE_SUCCESS(rv, rv);
  if (aUserTitle.IsVoid())
    statement->BindNullParameter(2);
  else
    statement->BindStringParameter(2, StringHead(aUserTitle, HISTORY_TITLE_LENGTH_MAX));
  NS_ENSURE_SUCCESS(rv, rv);

  statement->BindInt32Parameter(3, aVisitCount);
  NS_ENSURE_SUCCESS(rv, rv);
  statement->BindInt32Parameter(4, aHidden ? 1 : 0);
  NS_ENSURE_SUCCESS(rv, rv);
  statement->BindInt32Parameter(5, aTyped ? 1 : 0);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}










NS_IMETHODIMP
nsNavHistory::AddVisit(nsIURI* aURI, PRTime aTime, PRInt64 aReferringVisit,
                       PRInt32 aTransitionType, PRBool aIsRedirect,
                       PRInt64 aSessionID, PRInt64* aVisitID)
{
  nsresult rv;

  
  PRBool canAdd = PR_FALSE;
  rv = CanAddURI(aURI, &canAdd);
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

  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE,
                                  mozIStorageConnection::TRANSACTION_EXCLUSIVE);

  
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

    PRInt32 oldHiddenState = 0;
    rv = mDBGetPageVisitStats->GetInt32(3, &oldHiddenState);
    NS_ENSURE_SUCCESS(rv, rv);

    
    mDBGetPageVisitStats->Reset();
    scoper.Abandon();

    
    
    
    
    
    
    
    
    hidden = oldHiddenState;
    if (hidden && ! aIsRedirect &&
        aTransitionType != nsINavHistoryService::TRANSITION_EMBED)
      hidden = PR_FALSE; 

    typed = oldTypedState || (aTransitionType == TRANSITION_TYPED);

    
    
    if (oldVisitCount == 0)
      newItem = PR_TRUE;

    
    mozStorageStatementScoper updateScoper(mDBUpdatePageVisitStats);
    rv = mDBUpdatePageVisitStats->BindInt64Parameter(0, pageID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBUpdatePageVisitStats->BindInt32Parameter(1, oldVisitCount + 1);
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

    
    
    hidden = (aTransitionType == TRANSITION_EMBED || aIsRedirect);

    typed = (aTransitionType == TRANSITION_TYPED);

    
    nsString voidString;
    voidString.SetIsVoid(PR_TRUE);
    rv = InternalAddNewPage(aURI, voidString, hidden, typed, 1, &pageID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = InternalAddVisit(pageID, aReferringVisit, aSessionID, aTime,
                        aTransitionType, aVisitID);

  
  
  
  transaction.Commit();
  if (! hidden && aTransitionType != TRANSITION_EMBED) {
    ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                        OnVisit(aURI, *aVisitID, aTime, aSessionID,
                                aReferringVisit, aTransitionType));
  }

  
  
  
  
  if (newItem && aIsRedirect) {
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
    nsCOMPtr<nsNavHistoryResultNode> tempRootNode;
    rv = bookmarks->ResultNodeForFolder(folderId, options,
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

  nsresult rv;

  PRInt32 sortingMode = aOptions->SortingMode();
  if (sortingMode < 0 ||
      sortingMode > nsINavHistoryQueryOptions::SORT_BY_ANNOTATION_DESCENDING) {
    return NS_ERROR_INVALID_ARG;
  }

  PRBool asVisits =
    (aOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_VISIT ||
     aOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_FULL_VISIT);

  nsCAutoString commonConditions;

  if (aOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS) {
    
    commonConditions.AssignLiteral("b.type = 1 ");
  } else if (!aOptions->IncludeHidden()) {
    
    commonConditions.AssignLiteral("h.hidden <> 1 ");

    
    
    
    
    
    if (asVisits)
      commonConditions.AppendLiteral("AND v.visit_type <> 4 "); 
  }

  
  
  
  
  
  nsCAutoString queryString;
  nsCAutoString groupBy;
  if (asVisits) {
    
    
    
    queryString = NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
             "v.visit_date, f.url, v.session, null "
      "FROM moz_places h "
      "JOIN moz_historyvisits v ON h.id = v.place_id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id ");
  } else {
    
    
    
    
    
    if (aOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY) {
      queryString = NS_LITERAL_CSTRING(
        "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
          "(SELECT MAX(visit_date) FROM moz_historyvisits WHERE place_id = h.id), "
          "f.url, null, null "
        "FROM moz_places h "
        "LEFT OUTER JOIN moz_historyvisits v ON h.id = v.place_id "
        "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id ");
      groupBy = NS_LITERAL_CSTRING(" GROUP BY h.id");
    } else if (aOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS) {
      queryString = NS_LITERAL_CSTRING(
        "SELECT b.fk, h.url, b.title, h.rev_host, h.visit_count, "
        "(SELECT MAX(visit_date) FROM moz_historyvisits WHERE place_id = b.fk), "
        "f.url, null, b.id, b.dateAdded, b.lastModified "
      "FROM moz_bookmarks b "
      "JOIN moz_places h ON b.fk = h.id "
      "LEFT OUTER JOIN moz_historyvisits v ON b.fk = v.place_id "
      "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id ");
      groupBy = NS_LITERAL_CSTRING(" GROUP BY b.id");
    }
    else {
      
      return NS_ERROR_NOT_IMPLEMENTED;
    }
  }

  PRInt32 numParameters = 0;
  nsCAutoString conditions;
  PRInt32 i;
  for (i = 0; i < aQueries.Count(); i ++) {
    nsCString queryClause;
    PRInt32 clauseParameters = 0;
    rv = QueryToSelectClause(aQueries[i], aOptions, numParameters,
                             &queryClause, &clauseParameters,
                             commonConditions);
    NS_ENSURE_SUCCESS(rv, rv);
    if (! queryClause.IsEmpty()) {
      if (! conditions.IsEmpty()) 
        conditions += NS_LITERAL_CSTRING(" OR ");
      conditions += NS_LITERAL_CSTRING("(") + queryClause +
        NS_LITERAL_CSTRING(")");
      numParameters += clauseParameters;
    }
  }

  
  
  if (!conditions.IsEmpty()) {
    queryString += "WHERE ";
    queryString += conditions;
  } else if (!commonConditions.IsEmpty()) {
    queryString += "WHERE ";
    queryString += commonConditions;
  }
  queryString += groupBy;

  PRBool hasSearchTerms;
  rv = aQueries[0]->GetHasSearchTerms(&hasSearchTerms);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  switch(sortingMode) {
    case nsINavHistoryQueryOptions::SORT_BY_NONE:
      break;
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING:
      
      
      
      
      
      
      if (aOptions->MaxResults() > 0)
        queryString += NS_LITERAL_CSTRING(" ORDER BY 6 DESC"); 
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_ASCENDING:
      queryString += NS_LITERAL_CSTRING(" ORDER BY 6 ASC"); 
      break;
    case nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING:
      queryString += NS_LITERAL_CSTRING(" ORDER BY 6 DESC"); 
      break;
    case nsINavHistoryQueryOptions::SORT_BY_URI_ASCENDING:
      queryString += NS_LITERAL_CSTRING(" ORDER BY 2 ASC"); 
      break;
    case nsINavHistoryQueryOptions::SORT_BY_URI_DESCENDING:
      queryString += NS_LITERAL_CSTRING(" ORDER BY 2 DESC"); 
      break;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_ASCENDING:
      queryString += NS_LITERAL_CSTRING(" ORDER BY 5 ASC"); 
      break;
    case nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING:
      queryString += NS_LITERAL_CSTRING(" ORDER BY 5 DESC"); 
      break;
    default:
      NS_NOTREACHED("Invalid sorting mode");
  }

  
  if (aOptions->MaxResults() > 0) {
    queryString += NS_LITERAL_CSTRING(" LIMIT ");
    queryString.AppendInt(aOptions->MaxResults());
    queryString.AppendLiteral(" ");
  }

#ifdef DEBUG_thunder
  printf("Constructed the query: %s\n", PromiseFlatCString(queryString).get());
#endif

  
  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(queryString, getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  
  numParameters = 0;
  for (i = 0; i < aQueries.Count(); i ++) {
    PRInt32 clauseParameters = 0;
    rv = BindQueryClauseParameters(statement, numParameters,
                                   aQueries[i], aOptions, &clauseParameters);
    NS_ENSURE_SUCCESS(rv, rv);
    numParameters += clauseParameters;
  }

  PRUint32 groupCount;
  const PRUint16 *groupings = aOptions->GroupingMode(&groupCount);

  if (groupCount == 0 && ! hasSearchTerms) {
    
    
    rv = ResultsAsList(statement, aOptions, aResults);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    nsCOMArray<nsNavHistoryResultNode> toplevel;
    rv = ResultsAsList(statement, aOptions, &toplevel);
    NS_ENSURE_SUCCESS(rv, rv);

    if (hasSearchTerms) {
      
      if (groupCount == 0) {
        
        FilterResultSet(toplevel, aResults, aQueries[0]->SearchTerms());
      } else {
        
        nsCOMArray<nsNavHistoryResultNode> filteredResults;
        FilterResultSet(toplevel, &filteredResults, aQueries[0]->SearchTerms());
        rv = RecursiveGroup(aResultNode, filteredResults, groupings, groupCount,
                            aResults);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    } else {
      
      rv = RecursiveGroup(aResultNode, toplevel, groupings, groupCount, aResults);
      NS_ENSURE_SUCCESS(rv, rv);
    }
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
  mBatchesInProgress ++;
  if (mBatchesInProgress == 1) {
    ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                        OnBeginUpdateBatch())
  }
  return NS_OK;
}




nsresult
nsNavHistory::EndUpdateBatch()
{
  if (mBatchesInProgress == 0)
    return NS_ERROR_FAILURE;
  if (--mBatchesInProgress == 0) {
    ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver, OnEndUpdateBatch())
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::RunInBatchMode(nsINavHistoryBatchCallback* aCallback,
                             nsISupports* aUserData) {
  NS_ENSURE_ARG_POINTER(aCallback);

  UpdateBatchScoper batch(*this);
  nsresult rv = aCallback->RunBatched(aUserData);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
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

  return SetPageTitleInternal(aURI, PR_FALSE, nsString(aTitle));
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







NS_IMETHODIMP
nsNavHistory::RemovePage(nsIURI *aURI)
{
  nsresult rv;
  mozStorageTransaction transaction(mDBConn, PR_FALSE);
  nsCOMPtr<mozIStorageStatement> statement;

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits WHERE place_id IN (SELECT id FROM moz_places WHERE url = ?1)"),
      getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = BindStatementURI(statement, 0, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  nsNavBookmarks* bookmarksService = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarksService, NS_ERROR_FAILURE);
  PRBool bookmarked = PR_FALSE;
  rv = bookmarksService->IsBookmarked(aURI, &bookmarked);
  NS_ENSURE_SUCCESS(rv, rv);

  if (! bookmarked) {
    
    
    

    

    
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "DELETE FROM moz_places WHERE url = ?1"),
        getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = BindStatementURI(statement, 0, aURI);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = statement->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  transaction.Commit();
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver, OnDeleteURI(aURI))
  return NS_OK;
}

















NS_IMETHODIMP
nsNavHistory::RemovePagesFromHost(const nsACString& aHost, PRBool aEntireDomain)
{
  nsresult rv;
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  
  if (aHost.IsEmpty())
    aEntireDomain = PR_FALSE;

  
  
  nsCString localFiles;
  TitleForDomain(EmptyCString(), localFiles);
  nsAutoString host16;
  if (! aHost.Equals(localFiles))
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
    conditionString.AssignLiteral("WHERE h.rev_host >= ?1 AND h.rev_host < ?2 ");
  else
    conditionString.AssignLiteral("WHERE h.rev_host = ?1");

  
  
  
  
  
  
  
  
  
  nsCStringArray deletedURIs;
  nsCOMPtr<mozIStorageStatement> statement;

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT url FROM moz_places h ") + conditionString,
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(0, revHostDot);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aEntireDomain) {
    rv = statement->BindStringParameter(1, revHostSlash);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool hasMore = PR_FALSE;
  while ((statement->ExecuteStep(&hasMore) == NS_OK) && hasMore) {
    nsCAutoString thisURIString;
    if (NS_FAILED(statement->GetUTF8String(0, thisURIString)) || 
        thisURIString.IsEmpty())
      continue; 
    if (! deletedURIs.AppendCString(thisURIString))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits WHERE place_id IN (SELECT id FROM moz_places h ") + conditionString + NS_LITERAL_CSTRING(")"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(0, revHostDot);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aEntireDomain) {
    rv = statement->BindStringParameter(1, revHostSlash);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_places WHERE id IN "
      "(SELECT id from moz_places h "
      " LEFT OUTER JOIN moz_bookmarks b ON h.id = b.fk WHERE b.type = ?3")
      + conditionString +
      NS_LITERAL_CSTRING("AND b.fk IS NULL)"),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringParameter(0, revHostDot);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aEntireDomain) {
    rv = statement->BindStringParameter(1, revHostSlash);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = statement->BindInt32Parameter(2, nsNavBookmarks::TYPE_BOOKMARK);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  transaction.Commit();

  
  UpdateBatchScoper batch(*this); 
  if (deletedURIs.Count()) {
    nsCOMPtr<nsIURI> thisURI;
    for (PRUint32 observerIndex = 0; observerIndex < mObservers.Length();
         observerIndex ++) {
      const nsCOMPtr<nsINavHistoryObserver> &obs = mObservers.ElementAt(observerIndex);
      if (! obs)
        continue;

      
      for (PRInt32 i = 0; i < deletedURIs.Count(); i ++) {
        if (NS_FAILED(NS_NewURI(getter_AddRefs(thisURI), *deletedURIs[i],
                                nsnull, nsnull)))
          continue; 
        obs->OnDeleteURI(thisURI);
      }
    }
  }
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
  aURI->GetSpec(uriString);

  
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

  PRTime now = PR_Now();

  nsresult rv;
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
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (aToplevel)
      transitionType = nsINavHistoryService::TRANSITION_LINK;
    else
      transitionType = nsINavHistoryService::TRANSITION_EMBED;

    
    
    
    
    
    visitTime = PR_Now();

    
    if (! FindLastVisit(aReferrer, &referringVisit, aSessionID)) {
      
      *aSessionID = GetNewSessionID();
    }
  } else {
    
    
    
    if (CheckIsRecentEvent(&mRecentTyped, spec))
      transitionType = nsINavHistoryService::TRANSITION_TYPED;
    else if (CheckIsRecentEvent(&mRecentBookmark, spec))
      transitionType = nsINavHistoryService::TRANSITION_BOOKMARK;

    visitTime = PR_Now();
    *aSessionID = GetNewSessionID();
  }

  
  return AddVisit(aURI, visitTime, referringVisit, transitionType,
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
nsNavHistory::SetPageTitle(nsIURI *aURI,
                           const nsAString & aTitle)
{
  if (aTitle.IsEmpty())
    return NS_OK;

#ifdef LAZY_ADD
  LazyMessage message;
  nsresult rv = message.Init(LazyMessage::Type_Title, aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  message.title = aTitle;
  return AddLazyMessage(message);
#else
  return SetPageTitleInternal(aURI, PR_FALSE, aTitle);
#endif
}

NS_IMETHODIMP
nsNavHistory::GetPageTitle(nsIURI *aURI, nsAString &aTitle)
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




NS_IMETHODIMP
nsNavHistory::Observe(nsISupports *aSubject, const char *aTopic,
                    const PRUnichar *aData)
{
  if (nsCRT::strcmp(aTopic, gQuitApplicationMessage) == 0) {
    if (gTldTypes) {
      delete gTldTypes;
      gTldTypes = nsnull;
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
  } else if (nsCRT::strcmp(aTopic, gXpcomShutdown) == 0) {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    observerService->RemoveObserver(this, gXpcomShutdown);
    observerService->RemoveObserver(this, gQuitApplicationMessage);
  } else if (nsCRT::strcmp(aTopic, "nsPref:changed") == 0) {
    PRInt32 oldDays = mExpireDays;
    LoadPrefs();
    if (oldDays != mExpireDays)
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
        SetPageTitleInternal(message.uri, PR_FALSE, message.title);
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
                                  PRInt32* aParamCount,
                                  const nsACString& aCommonConditions)
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

  
  if (aOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS) {
    
    
    if (aQuery->Folders().Length() == 1) {
      if (!aClause->IsEmpty())
        *aClause += NS_LITERAL_CSTRING(" AND ");

      nsCAutoString paramString;
      parameterString(aStartParameter + *aParamCount, paramString);
      (*aParamCount) ++;
      *aClause += NS_LITERAL_CSTRING(" b.parent = ") + paramString;
    }
  } else if (aQuery->OnlyBookmarked()) {
    
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
    
    
  } else {
    if (!(aClause->IsEmpty() || aCommonConditions.IsEmpty()))
      *aClause += NS_LITERAL_CSTRING(" AND ");
    aClause->Append(aCommonConditions);
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

  
  if (aOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS) {
    
    if (aQuery->Folders().Length() == 1) {
      rv = statement->BindInt64Parameter(aStartParameter + *aParamCount,
                                         aQuery->Folders()[0]);
      NS_ENSURE_SUCCESS(rv, rv);
      (*aParamCount) ++;
    }
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
    nsCOMPtr<nsNavHistoryResultNode> result;
    rv = RowToResult(row, aOptions, getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);
    aResults->AppendObject(result);
  }
  return NS_OK;
}















nsresult
nsNavHistory::RecursiveGroup(nsNavHistoryQueryResultNode *aResultNode,
                             const nsCOMArray<nsNavHistoryResultNode>& aSource,
                             const PRUint16* aGroupingMode, PRUint32 aGroupCount,
                             nsCOMArray<nsNavHistoryResultNode>* aDest)
{
  NS_ASSERTION(aGroupCount > 0, "Invalid group count");
  NS_ASSERTION(aDest->Count() == 0, "Destination array is not empty");
  NS_ASSERTION(&aSource != aDest, "Source and dest must be different for grouping");

  nsresult rv;
  switch (aGroupingMode[0]) {
    case nsINavHistoryQueryOptions::GROUP_BY_DAY:
      rv = GroupByDay(aResultNode, aSource, aDest);
      break;
    case nsINavHistoryQueryOptions::GROUP_BY_HOST:
      rv = GroupByHost(aResultNode, aSource, aDest, PR_FALSE);
      break;
    case nsINavHistoryQueryOptions::GROUP_BY_DOMAIN:
      rv = GroupByHost(aResultNode, aSource, aDest, PR_TRUE);
      break;
    case nsINavHistoryQueryOptions::GROUP_BY_FOLDER:
      
      
      return NS_ERROR_NOT_IMPLEMENTED;
    default:
      
      return NS_ERROR_INVALID_ARG;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  if (aGroupCount > 1) {
    
    
    for (PRInt32 i = 0; i < aDest->Count(); i ++) {
      nsNavHistoryResultNode* curNode = (*aDest)[i];
      if (curNode->IsContainer()) {
        nsNavHistoryContainerResultNode* container = curNode->GetAsContainer();
        nsCOMArray<nsNavHistoryResultNode> temp(container->mChildren);
        container->mChildren.Clear();
        rv = RecursiveGroup(aResultNode, temp, &aGroupingMode[1], aGroupCount - 1,
                            &container->mChildren);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  return NS_OK;
}






static const PRInt64 USECS_PER_DAY = LL_INIT(20, 500654080);
static PRInt64
GetAgeInDays(PRTime aNormalizedNow, PRTime aDate)
{
  PRTime dateMidnight = NormalizeTimeRelativeToday(aDate);
  
  
  if (dateMidnight > aNormalizedNow)
    return 0;
  else
    return ((aNormalizedNow - dateMidnight) / USECS_PER_DAY);
}

const PRInt64 UNDEFINED_AGE_IN_DAYS = -1;




nsresult
CreatePlacesPersistURN(nsNavHistoryQueryResultNode *aResultNode, 
                      PRInt64 aAgeInDays, const nsCString& aTitle, nsCString& aURN)
{
  nsCAutoString uri;
  nsresult rv = aResultNode->GetUri(uri);
  NS_ENSURE_SUCCESS(rv, rv);

  aURN.Assign(NS_LITERAL_CSTRING("urn:places-persist:"));
  aURN.Append(uri);

  aURN.Append(NS_LITERAL_CSTRING(","));
  if (aAgeInDays != UNDEFINED_AGE_IN_DAYS)
    aURN.AppendInt(aAgeInDays);

  aURN.Append(NS_LITERAL_CSTRING(","));
  if (!aTitle.IsEmpty()) {
    nsCAutoString escapedTitle;
    PRBool success = NS_Escape(aTitle, escapedTitle, url_XAlphas);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    aURN.Append(escapedTitle);
  }

  return NS_OK;
}






nsresult
nsNavHistory::GroupByDay(nsNavHistoryQueryResultNode *aResultNode,
                         const nsCOMArray<nsNavHistoryResultNode>& aSource,
                         nsCOMArray<nsNavHistoryResultNode>* aDest)
{
  
  const PRInt32 numDays = 8;

  nsNavHistoryContainerResultNode *dates[numDays];
  for (PRInt32 i = 0; i < numDays; i++)
    dates[i] = nsnull;

  nsCAutoString dateNames[numDays];
  
  GetStringFromName(NS_LITERAL_STRING("finduri-AgeInDays-is-0").get(),  
                    dateNames[0]);
  
  GetStringFromName(NS_LITERAL_STRING("finduri-AgeInDays-is-1").get(),  
                    dateNames[1]);
  for (PRInt32 curDay = 2; curDay <= numDays-2; curDay++) {
    
    GetAgeInDaysString(curDay, NS_LITERAL_STRING("finduri-AgeInDays-is").get(),
                       dateNames[curDay]);
  }
  
  GetAgeInDaysString(numDays-2, 
                     NS_LITERAL_STRING("finduri-AgeInDays-isgreater").get(),
                     dateNames[numDays-1]);
    
  PRTime normalizedNow = NormalizeTimeRelativeToday(PR_Now());

  for (PRInt32 i = 0; i < aSource.Count(); i ++) {
    if (!aSource[i]->IsURI()) {
      
      aDest->AppendObject(aSource[i]);
      continue;
    }

    
    nsCAutoString curDateName;
    PRInt64 ageInDays = GetAgeInDays(normalizedNow, aSource[i]->mTime);
    if (ageInDays > (numDays - 1))
      ageInDays = numDays - 1;
    curDateName = dateNames[ageInDays];

    if (!dates[ageInDays]) {
      nsCAutoString urn;
      nsresult rv = CreatePlacesPersistURN(aResultNode, ageInDays, EmptyCString(), urn);
      NS_ENSURE_SUCCESS(rv, rv);

      
      dates[ageInDays] = new nsNavHistoryContainerResultNode(urn, 
          curDateName,
          EmptyCString(),
          nsNavHistoryResultNode::RESULT_TYPE_DAY,
          PR_TRUE,
          EmptyCString());

      if (!dates[ageInDays])
        return NS_ERROR_OUT_OF_MEMORY;
    }
    if (!dates[ageInDays]->mChildren.AppendObject(aSource[i]))
      return NS_ERROR_OUT_OF_MEMORY;
  }

  for (PRInt32 i = 0; i < numDays; i++) {
    if (dates[i]) {
      nsresult rv = aDest->AppendObject(dates[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}














nsresult
nsNavHistory::GroupByHost(nsNavHistoryQueryResultNode *aResultNode,
                          const nsCOMArray<nsNavHistoryResultNode>& aSource,
                          nsCOMArray<nsNavHistoryResultNode>* aDest,
                          PRBool aIsDomain)
{
  nsDataHashtable<nsCStringHashKey, nsNavHistoryContainerResultNode*> hosts;
  if (! hosts.Init(512))
    return NS_ERROR_OUT_OF_MEMORY;

  for (PRInt32 i = 0; i < aSource.Count(); i ++) {
    if (! aSource[i]->IsURI()) {
      
      aDest->AppendObject(aSource[i]);
      continue;
    }

    
    nsCOMPtr<nsIURI> uri;
    nsCAutoString fullHostName;
    if (NS_FAILED(NS_NewURI(getter_AddRefs(uri), aSource[i]->mURI)) ||
        NS_FAILED(uri->GetHost(fullHostName))) {
      
      aDest->AppendObject(aSource[i]);
      continue;
    }

    nsCAutoString curHostName;
    if (aIsDomain) {
      DomainNameFromHostName(fullHostName, curHostName);
    } else {
      
      curHostName = fullHostName;
    }

    nsNavHistoryContainerResultNode* curTopGroup = nsnull;
    if (! hosts.Get(curHostName, &curTopGroup)) {
      
      nsCAutoString title;
      TitleForDomain(curHostName, title);

      nsCAutoString urn;
      nsresult rv = CreatePlacesPersistURN(aResultNode, UNDEFINED_AGE_IN_DAYS, title, urn);
      NS_ENSURE_SUCCESS(rv, rv);

      curTopGroup = new nsNavHistoryContainerResultNode(urn, title,
          EmptyCString(), nsNavHistoryResultNode::RESULT_TYPE_HOST, PR_TRUE,
          EmptyCString());
      if (! curTopGroup)
        return NS_ERROR_OUT_OF_MEMORY;

      if (! hosts.Put(curHostName, curTopGroup))
        return NS_ERROR_OUT_OF_MEMORY;
   
      rv = aDest->AppendObject(curTopGroup);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (! curTopGroup->mChildren.AppendObject(aSource[i]))
      return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}







nsresult
nsNavHistory::FilterResultSet(const nsCOMArray<nsNavHistoryResultNode>& aSet,
                              nsCOMArray<nsNavHistoryResultNode>* aFiltered,
                              const nsString& aSearch)
{
  nsStringArray terms;
  ParseSearchQuery(aSearch, &terms);

  
  if (terms.Count() == 0) {
    aFiltered->AppendObjects(aSet);
    return NS_OK;
  }

  nsCStringArray searchAnnotations;
  








  for (PRInt32 nodeIndex = 0; nodeIndex < aSet.Count(); nodeIndex ++) {
    PRBool allTermsFound = PR_TRUE;

    nsStringArray curAnnotations;
    













    for (PRInt32 termIndex = 0; termIndex < terms.Count(); termIndex ++) {
      PRBool termFound = PR_FALSE;
      
      if (CaseInsensitiveFindInReadable(*terms[termIndex],
                                        NS_ConvertUTF8toUTF16(aSet[nodeIndex]->mTitle)) ||
          (aSet[nodeIndex]->IsURI() &&
           CaseInsensitiveFindInReadable(*terms[termIndex],
                                  NS_ConvertUTF8toUTF16(aSet[nodeIndex]->mURI))))
        termFound = PR_TRUE;
      
      






      if (! termFound) {
        allTermsFound = PR_FALSE;
        break;
      }
    }
    if (allTermsFound)
      aFiltered->AppendObject(aSet[nodeIndex]);
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

  if (IsQueryURI(url)) {
    
    
    
    return QueryRowToResult(url, title, accessCount, time, favicon, aResult);
  } else if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_URI) {
    *aResult = new nsNavHistoryResultNode(url, title, accessCount, time,
                                          favicon);
    if (! *aResult)
      return NS_ERROR_OUT_OF_MEMORY;

    PRBool isNull;
    if (NS_SUCCEEDED(aRow->GetIsNull(kGetInfoIndex_ItemId, &isNull)) &&
        !isNull) {
      (*aResult)->mItemId = aRow->AsInt64(kGetInfoIndex_ItemId);
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

  
  NS_ASSERTION(aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_FULL_VISIT,
               "Invalid result type in RowToResult");

  NS_NOTREACHED("Full visits not supported yet.");
  
  



  
  



  
  









  return NS_OK;
}







nsresult
nsNavHistory::QueryRowToResult(const nsACString& aURI, const nsACString& aTitle,
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

      
      rv = bookmarks->ResultNodeForFolder(folderId, options, aNode);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      
      *aNode = new nsNavHistoryQueryResultNode(aTitle, EmptyCString(),
                                               queries, options);
      if (! *aNode)
        return NS_ERROR_OUT_OF_MEMORY;
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
  if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_VISIT ||
      aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_FULL_VISIT) {
    
    statement = mDBVisitToVisitResult;
  } else {
    
    statement = mDBVisitToURLResult;
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
nsNavHistory::UriToResultNode(nsIURI* aUri, nsNavHistoryQueryOptions* aOptions,
                              nsNavHistoryResultNode** aResult)
{
  
  
  NS_ASSERTION(aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_URI,
               "Can't make visits from URIs");
  mozStorageStatementScoper scoper(mDBUrlToUrlResult);
  nsresult rv = BindStatementURI(mDBUrlToUrlResult, 0, aUri);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = mDBUrlToUrlResult->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid URL");
    return NS_ERROR_INVALID_ARG;
  }

  return RowToResult(mDBUrlToUrlResult, aOptions, aResult);
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
nsNavHistory::SetPageTitleInternal(nsIURI* aURI, PRBool aIsUserTitle,
                                   const nsAString& aTitle)
{
  nsresult rv;

  mozStorageTransaction transaction(mDBConn, PR_TRUE);

  
  
  nsAutoString title;
  nsAutoString userTitle;
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

  
  
  
  
  if (aIsUserTitle && aTitle.IsVoid() == userTitle.IsVoid() &&
      aTitle == userTitle)
    return NS_OK;
  if (! aIsUserTitle && aTitle.IsVoid() == title.IsVoid() &&
      aTitle == title)
    return NS_OK;

  nsCOMPtr<mozIStorageStatement> dbModStatement;
  if (aIsUserTitle) {
    userTitle = aTitle;
    rv = mDBConn->CreateStatement(
        NS_LITERAL_CSTRING("UPDATE moz_places SET user_title = ?1 WHERE url = ?2"),
        getter_AddRefs(dbModStatement));
  } else {
    title = aTitle;
    rv = mDBConn->CreateStatement(
        NS_LITERAL_CSTRING("UPDATE moz_places SET title = ?1 WHERE url = ?2"),
        getter_AddRefs(dbModStatement));
  }
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
  transaction.Commit();

  
  ENUMERATE_WEAKARRAY(mObservers, nsINavHistoryObserver,
                      OnTitleChanged(aURI, title, userTitle, aIsUserTitle))

  return NS_OK;

}












nsresult
nsNavHistory::CreateLookupIndexes()
{
  nsresult rv;

  
  rv = mDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX moz_places_hostindex ON moz_places (rev_host)"));
  
  rv = mDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX moz_places_visitcount ON moz_places (visit_count)"));
  

  
  rv = mDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX moz_historyvisits_fromindex ON moz_historyvisits (from_visit)"));
  
  rv = mDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX moz_historyvisits_dateindex ON moz_historyvisits (visit_date)"));
  

#ifdef IN_MEMORY_LINKS
  
  rv = mMemDBConn->ExecuteSimpleSQL(
      NS_LITERAL_CSTRING("CREATE INDEX moz_memhistory_index ON moz_memhistory (url)"));
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  return NS_OK;
}

nsresult
nsNavHistory::AddPageWithVisit(nsIURI *aURI,
                               const nsString &aTitle,
                               const nsString &aUserTitle,
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
  rv = InternalAddNewPage(aURI, aTitle, aHidden, aTyped, aVisitCount, &pageID);
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
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("SELECT id, url FROM moz_places ORDER BY url"),
      getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsTArray<PRInt64> duplicates;
  nsCAutoString lastURI;
  PRBool hasMore;
  while (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
    nsCAutoString uri;
    statement->GetUTF8String(1, uri);
    if (uri.Equals(lastURI)) {
      duplicates.AppendElement(statement->AsInt64(0));
    } else {
      lastURI = uri;
    }
  }

  
  rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("DELETE FROM moz_places WHERE id = ?1"),
      getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> visitDelete;
  rv = mDBConn->CreateStatement(
      NS_LITERAL_CSTRING("DELETE FROM moz_historyvisits WHERE place_id = ?1"),
      getter_AddRefs(visitDelete));
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < duplicates.Length(); ++i) {
    PRInt64 id = duplicates[i];
    {
      mozStorageStatementScoper scope(statement);
      rv = statement->BindInt64Parameter(0, id);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = statement->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    {
      mozStorageStatementScoper scope(visitDelete);
      rv = visitDelete->BindInt64Parameter(0, id);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = visitDelete->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
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














PRBool IsNumericHostName(const nsCString& aHost)
{
  PRInt32 periodCount = 0;
  for (PRUint32 i = 0; i < aHost.Length(); i ++) {
    PRUnichar cur = aHost[i];
    if (cur == '.')
      periodCount ++;
    else if (cur < '0' || cur > '9')
      return PR_FALSE;
  }
  return (periodCount == 3);
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
  if (aOptions->MaxResults() > 0)
    return 0;

  
  

  return query->Folders()[0];
}









inline PRBool isQueryWhitespace(PRUnichar ch)
{
  return ch == ' ';
}

void ParseSearchQuery(const nsString& aQuery, nsStringArray* aTerms)
{
  PRInt32 lastBegin = -1;
  for (PRUint32 i = 0; i < aQuery.Length(); i ++) {
    if (isQueryWhitespace(aQuery[i]) || aQuery[i] == '"') {
      if (lastBegin >= 0) {
        
        aTerms->AppendString(Substring(aQuery, lastBegin, i - lastBegin));
        lastBegin = -1;
      }
    } else {
      if (lastBegin < 0) {
        
        lastBegin = i;
      }
    }
  }
  
  if (lastBegin >= 0)
    aTerms->AppendString(Substring(aQuery, lastBegin));
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










PRInt32
GetTLDCharCount(const nsCString& aHost)
{
  nsCAutoString trailing;
  GetSubstringFromNthDot(aHost, aHost.Length() - 1, 1,
                         PR_FALSE, trailing);

  switch (GetTLDType(trailing)) {
    case 0:
      
      return 0;
    case 1:
      
      return trailing.Length();
    case 2: {
      
      nsCAutoString trailingMore;
      GetSubstringFromNthDot(aHost, aHost.Length() - 1,
                             2, PR_FALSE, trailingMore);
      if (GetTLDType(trailingMore))
        return trailingMore.Length();
      else
        return trailing.Length();
    }
    default:
      NS_NOTREACHED("Invalid TLD type");
      return 0;
  }
}

















PRInt32
GetTLDType(const nsCString& aHostTail)
{
  
  if (! gTldTypes) {
    
    gTldTypes = new nsDataHashtable<nsCStringHashKey, int>();
    if (! gTldTypes)
      return 0;

    gTldTypes->Init(256);

    gTldTypes->Put(NS_LITERAL_CSTRING("com"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("org"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("net"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("edu"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("gov"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("mil"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("uk"), 2);
    gTldTypes->Put(NS_LITERAL_CSTRING("co.uk"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("kr"), 2);
    gTldTypes->Put(NS_LITERAL_CSTRING("co.kr"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("hu"), 1);
    gTldTypes->Put(NS_LITERAL_CSTRING("us"), 1);

    
  }

  PRInt32 type = 0;
  if (gTldTypes->Get(aHostTail, &type))
    return type;
  else
    return 0;
}










void GetSubstringFromNthDot(const nsCString& aInput, PRInt32 aStartingSpot,
                            PRInt32 aN, PRBool aIncludeDot, nsACString& aSubstr)
{
  PRInt32 dotsFound = 0;
  for (PRInt32 i = aStartingSpot; i >= 0; i --) {
    if (aInput[i] == '.') {
      dotsFound ++;
      if (dotsFound == aN) {
        if (aIncludeDot)
          aSubstr = Substring(aInput, i, aInput.Length() - i);
        else
          aSubstr = Substring(aInput, i + 1, aInput.Length() - i - 1);
        return;
      }
    }
  }
  aSubstr = aInput; 
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
