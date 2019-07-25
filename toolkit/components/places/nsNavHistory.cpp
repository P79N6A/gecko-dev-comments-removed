












































#include <stdio.h>

#include "nsNavHistory.h"

#include "nsTArray.h"
#include "nsCollationCID.h"
#include "nsILocaleService.h"
#include "nsIPrefBranch2.h"

#include "nsNetUtil.h"
#include "nsPrintfCString.h"
#include "nsPromiseFlatString.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "prsystem.h"
#include "prtime.h"
#include "nsEscape.h"
#include "nsIEffectiveTLDService.h"
#include "nsIClassInfoImpl.h"
#include "nsThreadUtils.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsMathUtils.h"
#include "mozIStorageAsyncStatement.h"
#include "mozIPlacesAutoComplete.h"

#include "nsNavBookmarks.h"
#include "nsAnnotationService.h"
#include "nsILivemarkService.h"
#include "nsFaviconService.h"

#include "nsPlacesTables.h"
#include "nsPlacesIndexes.h"
#include "nsPlacesTriggers.h"
#include "nsPlacesMacros.h"
#include "SQLFunctions.h"
#include "Helpers.h"
#include "History.h"

#include "mozilla/FunctionTimer.h"

#ifdef MOZ_XUL
#include "nsIAutoCompleteInput.h"
#include "nsIAutoCompletePopup.h"
#endif

using namespace mozilla::places;






#define RECENT_EVENT_QUEUE_MAX_LENGTH 128


#define PREF_PLACES_BRANCH_BASE                 "places."

#define PREF_HISTORY_ENABLED                    "history.enabled"

#define PREF_FRECENCY_NUM_VISITS                "frecency.numVisits"
#define PREF_FRECENCY_FIRST_BUCKET_CUTOFF       "frecency.firstBucketCutoff"
#define PREF_FRECENCY_SECOND_BUCKET_CUTOFF      "frecency.secondBucketCutoff"
#define PREF_FRECENCY_THIRD_BUCKET_CUTOFF       "frecency.thirdBucketCutoff"
#define PREF_FRECENCY_FOURTH_BUCKET_CUTOFF      "frecency.fourthBucketCutoff"
#define PREF_FRECENCY_FIRST_BUCKET_WEIGHT       "frecency.firstBucketWeight"
#define PREF_FRECENCY_SECOND_BUCKET_WEIGHT      "frecency.secondBucketWeight"
#define PREF_FRECENCY_THIRD_BUCKET_WEIGHT       "frecency.thirdBucketWeight"
#define PREF_FRECENCY_FOURTH_BUCKET_WEIGHT      "frecency.fourthBucketWeight"
#define PREF_FRECENCY_DEFAULT_BUCKET_WEIGHT     "frecency.defaultBucketWeight"
#define PREF_FRECENCY_EMBED_VISIT_BONUS         "frecency.embedVisitBonus"
#define PREF_FRECENCY_FRAMED_LINK_VISIT_BONUS   "frecency.framedLinkVisitBonus"
#define PREF_FRECENCY_LINK_VISIT_BONUS          "frecency.linkVisitBonus"
#define PREF_FRECENCY_TYPED_VISIT_BONUS         "frecency.typedVisitBonus"
#define PREF_FRECENCY_BOOKMARK_VISIT_BONUS      "frecency.bookmarkVisitBonus"
#define PREF_FRECENCY_DOWNLOAD_VISIT_BONUS      "frecency.downloadVisitBonus"
#define PREF_FRECENCY_PERM_REDIRECT_VISIT_BONUS "frecency.permRedirectVisitBonus"
#define PREF_FRECENCY_TEMP_REDIRECT_VISIT_BONUS "frecency.tempRedirectVisitBonus"
#define PREF_FRECENCY_DEFAULT_VISIT_BONUS       "frecency.defaultVisitBonus"
#define PREF_FRECENCY_UNVISITED_BOOKMARK_BONUS  "frecency.unvisitedBookmarkBonus"
#define PREF_FRECENCY_UNVISITED_TYPED_BONUS     "frecency.unvisitedTypedBonus"

#define PREF_CACHE_TO_MEMORY_PERCENTAGE         "database.cache_to_memory_percentage"

#define PREF_FORCE_DATABASE_REPLACEMENT         "database.replaceOnStartup"




#define DATABASE_DEFAULT_CACHE_TO_MEMORY_PERCENTAGE 6




#define DATABASE_MAX_WAL_SIZE_IN_KIBIBYTES 512

#define BYTES_PER_MEBIBYTE 1048576



#define DATABASE_SCHEMA_VERSION 11


#define DATABASE_FILENAME NS_LITERAL_STRING("places.sqlite")


#define DATABASE_CORRUPT_FILENAME NS_LITERAL_STRING("places.sqlite.corrupt")



#define RENEW_CACHED_NOW_TIMEOUT ((PRInt32)3 * PR_MSEC_PER_SEC)


static const PRInt64 USECS_PER_DAY = LL_INIT(20, 500654080);


#define CHARSET_ANNO NS_LITERAL_CSTRING("URIProperties/characterSet")


#define SYNCGUID_ANNO NS_LITERAL_CSTRING("sync/guid")



#define HISTORY_ADDITIONAL_DATE_CONT_NUM 3


#define HISTORY_DATE_CONT_NUM(_daysFromOldestVisit) \
  (HISTORY_ADDITIONAL_DATE_CONT_NUM + \
   NS_MIN(6, (PRInt32)NS_ceilf((float)_daysFromOldestVisit/30)))

#define HISTORY_DATE_CONT_MAX 10


#define EMBED_VISITS_INITIAL_CACHE_SIZE 128


#define RECENT_EVENTS_INITIAL_CACHE_SIZE 128


#ifdef MOZ_XUL
#define TOPIC_AUTOCOMPLETE_FEEDBACK_INCOMING "autocomplete-will-enter-text"
#endif
#define TOPIC_IDLE_DAILY "idle-daily"
#define TOPIC_PREF_CHANGED "nsPref:changed"
#define TOPIC_PROFILE_TEARDOWN "profile-change-teardown"
#define TOPIC_PROFILE_CHANGE "profile-before-change"

NS_IMPL_THREADSAFE_ADDREF(nsNavHistory)
NS_IMPL_THREADSAFE_RELEASE(nsNavHistory)

NS_IMPL_CLASSINFO(nsNavHistory, NULL, nsIClassInfo::SINGLETON,
                  NS_NAVHISTORYSERVICE_CID)
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
  NS_INTERFACE_MAP_ENTRY(mozIStorageVacuumParticipant)
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

namespace {

static PRInt64 GetSimpleBookmarksQueryFolder(
    const nsCOMArray<nsNavHistoryQuery>& aQueries,
    nsNavHistoryQueryOptions* aOptions);
static void ParseSearchTermsFromQueries(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                        nsTArray<nsTArray<nsString>*>* aTerms);

} 

namespace mozilla {
  namespace places {

    bool hasRecentCorruptDB()
    {
      nsCOMPtr<nsIFile> profDir;
      nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                           getter_AddRefs(profDir));
      NS_ENSURE_SUCCESS(rv, false);
      nsCOMPtr<nsISimpleEnumerator> entries;
      rv = profDir->GetDirectoryEntries(getter_AddRefs(entries));
      NS_ENSURE_SUCCESS(rv, false);
      PRBool hasMore;
      while (NS_SUCCEEDED(entries->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> next;
        rv = entries->GetNext(getter_AddRefs(next));
        NS_ENSURE_SUCCESS(rv, false);
        nsCOMPtr<nsIFile> currFile = do_QueryInterface(next, &rv);
        NS_ENSURE_SUCCESS(rv, false);

        nsAutoString leafName;
        rv = currFile->GetLeafName(leafName);
        NS_ENSURE_SUCCESS(rv, false);
        if (leafName.Length() >= DATABASE_CORRUPT_FILENAME.Length() &&
            leafName.Find(".corrupt", DATABASE_FILENAME.Length()) != -1) {
          PRInt64 lastMod;
          rv = currFile->GetLastModifiedTime(&lastMod);
          NS_ENSURE_SUCCESS(rv, false);
          if (PR_Now() - lastMod > (PRInt64)24 * 60 * 60 * 1000 * 1000)
           return true;
        }
      }
      return false;
    }

    void GetTagsSqlFragment(PRInt64 aTagsFolder,
                            const nsACString& aRelation,
                            PRBool aHasSearchTerms,
                            nsACString& _sqlFragment) {
      if (!aHasSearchTerms)
        _sqlFragment.AssignLiteral("null");
      else {
        _sqlFragment.Assign(NS_LITERAL_CSTRING(
             "(SELECT GROUP_CONCAT(tag_title, ', ') "
              "FROM ( "
                "SELECT t_t.title AS tag_title "
                "FROM moz_bookmarks b_t "
                "JOIN moz_bookmarks t_t ON t_t.id = b_t.parent  "
                "WHERE b_t.fk = ") +
                aRelation + NS_LITERAL_CSTRING(" "
                "AND t_t.parent = ") +
                nsPrintfCString("%lld", aTagsFolder) + NS_LITERAL_CSTRING(" "
                "ORDER BY t_t.title COLLATE NOCASE ASC "
              ") "
             ")"));
      }

      _sqlFragment.AppendLiteral(" AS tags ");
    }

  } 
} 


namespace {





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



class AsyncStatementCallbackNotifier : public AsyncStatementCallback
{
public:
  AsyncStatementCallbackNotifier(const char* aTopic)
    : mTopic(aTopic)
  {
  }

  NS_IMETHOD HandleCompletion(PRUint16 aReason);

private:
  const char* mTopic;
};

NS_IMETHODIMP
AsyncStatementCallbackNotifier::HandleCompletion(PRUint16 aReason)
{
  if (aReason != mozIStorageStatementCallback::REASON_FINISHED)
    return NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    (void)obs->NotifyObservers(nsnull, mTopic, nsnull);
  }

  return NS_OK;
}

} 




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
const PRInt32 nsNavHistory::kGetInfoIndex_ItemParentId = 11;
const PRInt32 nsNavHistory::kGetInfoIndex_ItemTags = 12;
const PRInt32 nsNavHistory::kGetInfoIndex_Frecency = 13;

PLACES_FACTORY_SINGLETON_IMPLEMENTATION(nsNavHistory, gHistoryService)


nsNavHistory::nsNavHistory()
: mBatchLevel(0)
, mBatchDBTransaction(nsnull)
, mDBPageSize(0)
, mCurrentJournalMode(JOURNAL_DELETE)
, mCachedNow(0)
, mExpireNowTimer(nsnull)
, mLastSessionID(0)
, mHistoryEnabled(PR_TRUE)
, mNumVisitsForFrecency(10)
, mTagsFolder(-1)
, mInPrivateBrowsing(PRIVATEBROWSING_NOTINITED)
, mDatabaseStatus(DATABASE_STATUS_OK)
, mHasHistoryEntries(-1)
, mCanNotify(true)
, mCacheObservers("history-observers")
{
  NS_ASSERTION(!gHistoryService,
               "Attempting to create two instances of the service!");
  gHistoryService = this;
}


nsNavHistory::~nsNavHistory()
{
  
  
  NS_ASSERTION(gHistoryService == this,
               "Deleting a non-singleton instance of the service");
  if (gHistoryService == this)
    gHistoryService = nsnull;
}


nsresult
nsNavHistory::Init()
{
  NS_TIME_FUNCTION;

  nsCOMPtr<nsIPrefService> prefService =
    do_GetService(NS_PREFSERVICE_CONTRACTID);
  nsCOMPtr<nsIPrefBranch> placesBranch;
  NS_ENSURE_TRUE(prefService, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = prefService->GetBranch(PREF_PLACES_BRANCH_BASE,
                                       getter_AddRefs(placesBranch));
  NS_ENSURE_SUCCESS(rv, rv);
  mPrefBranch = do_QueryInterface(placesBranch);
  LoadPrefs();

  
  
  rv = InitDBFile(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = InitDB();
  if (NS_FAILED(rv)) {
    
    rv = InitDBFile(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = InitDB();
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = InitAdditionalDBItems();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsRefPtr<PlacesEvent> completeEvent =
    new PlacesEvent(TOPIC_PLACES_INIT_COMPLETE);
  rv = NS_DispatchToMainThread(completeEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NS_ENSURE_TRUE(mRecentTyped.Init(RECENT_EVENTS_INITIAL_CACHE_SIZE),
                 NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentLink.Init(RECENT_EVENTS_INITIAL_CACHE_SIZE),
                 NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentBookmark.Init(RECENT_EVENTS_INITIAL_CACHE_SIZE),
                 NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mRecentRedirects.Init(RECENT_EVENTS_INITIAL_CACHE_SIZE),
                 NS_ERROR_OUT_OF_MEMORY);

  
  NS_ENSURE_TRUE(mEmbedVisits.Init(EMBED_VISITS_INITIAL_CACHE_SIZE),
                 NS_ERROR_OUT_OF_MEMORY);

  







  
  if (mPrefBranch)
    mPrefBranch->AddObserver("", this, PR_FALSE);

  nsCOMPtr<nsIObserverService> obsSvc =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
  if (obsSvc) {
    (void)obsSvc->AddObserver(this, TOPIC_PROFILE_TEARDOWN, PR_FALSE);
    (void)obsSvc->AddObserver(this, TOPIC_PROFILE_CHANGE, PR_FALSE);
    (void)obsSvc->AddObserver(this, TOPIC_IDLE_DAILY, PR_FALSE);
    (void)obsSvc->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, PR_FALSE);
#ifdef MOZ_XUL
    (void)obsSvc->AddObserver(this, TOPIC_AUTOCOMPLETE_FEEDBACK_INCOMING, PR_FALSE);
#endif
  }

  
  
  
  if (mDatabaseStatus == DATABASE_STATUS_CREATE ||
      mDatabaseStatus == DATABASE_STATUS_UPGRADED) {
    if (mDatabaseStatus == DATABASE_STATUS_CREATE) {
      
      nsCOMPtr<nsIFile> historyFile;
      rv = NS_GetSpecialDirectory(NS_APP_HISTORY_50_FILE,
                                  getter_AddRefs(historyFile));
      if (NS_SUCCEEDED(rv) && historyFile) {
        (void)ImportHistory(historyFile);
      }
    }

    
    
    
    if (obsSvc)
      (void)obsSvc->AddObserver(this, TOPIC_PLACES_INIT_COMPLETE, PR_FALSE);
  }

  
  

  return NS_OK;
}


nsresult
nsNavHistory::InitDBFile(PRBool aForceInit)
{
  if (!mDBService) {
    mDBService = do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
    NS_ENSURE_STATE(mDBService);
  }

  
  nsCOMPtr<nsIFile> profDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(profDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = profDir->Clone(getter_AddRefs(mDBFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBFile->Append(DATABASE_FILENAME);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aForceInit) {
    
    
    
    
    
    if (!hasRecentCorruptDB()) {
      
      nsCOMPtr<nsIFile> backup;
      rv = mDBService->BackupDatabaseFile(mDBFile, DATABASE_CORRUPT_FILENAME,
                                          profDir, getter_AddRefs(backup));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    if (mDBConn) {
      
      
      rv = mDBConn->Close();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    rv = mDBFile->Remove(PR_FALSE);
    if (NS_FAILED(rv)) {
      
      
      
      
      
      
      nsRefPtr<PlacesEvent> lockedEvent =
        new PlacesEvent(TOPIC_DATABASE_LOCKED);
      (void)NS_DispatchToMainThread(lockedEvent);
    }
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    mDatabaseStatus = DATABASE_STATUS_CORRUPT;
  }
  else {
    
    PRBool dbExists = PR_TRUE;
    rv = mDBFile->Exists(&dbExists);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!dbExists) {
      mDatabaseStatus = DATABASE_STATUS_CREATE;
    }
    else {
      
      PRBool forceDatabaseReplacement;
      if (NS_SUCCEEDED(mPrefBranch->GetBoolPref(PREF_FORCE_DATABASE_REPLACEMENT,
                                                &forceDatabaseReplacement)) &&
          forceDatabaseReplacement) {
        
        rv = mPrefBranch->ClearUserPref(PREF_FORCE_DATABASE_REPLACEMENT);
        NS_ENSURE_SUCCESS(rv, rv);
        
        rv = InitDBFile(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
        return NS_OK;
      }
    }
  }

  
  
  rv = mDBService->OpenUnsharedDatabase(mDBFile, getter_AddRefs(mDBConn));
  if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    mDatabaseStatus = DATABASE_STATUS_CORRUPT;

    
    nsCOMPtr<nsIFile> backup;
    rv = mDBService->BackupDatabaseFile(mDBFile, DATABASE_CORRUPT_FILENAME,
                                        profDir, getter_AddRefs(backup));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = profDir->Clone(getter_AddRefs(mDBFile));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBFile->Append(DATABASE_FILENAME);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBService->OpenUnsharedDatabase(mDBFile, getter_AddRefs(mDBConn));
  }
 
  if (rv != NS_OK && rv != NS_ERROR_FILE_CORRUPTED) {
    
    
    
    nsRefPtr<PlacesEvent> lockedEvent =
      new PlacesEvent(TOPIC_DATABASE_LOCKED);
    (void)NS_DispatchToMainThread(lockedEvent);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsNavHistory::SetJournalMode(enum JournalMode aJournalMode)
{
  nsCAutoString journalMode;
  switch (aJournalMode) {
    default:
      NS_NOTREACHED("Trying to set an unknown journal mode.");
      
    case JOURNAL_DELETE:
      journalMode.AssignLiteral("delete");
      break;
    case JOURNAL_TRUNCATE:
      journalMode.AssignLiteral("truncate");
      break;
    case JOURNAL_MEMORY:
      journalMode.AssignLiteral("memory");
      break;
    case JOURNAL_WAL:
      journalMode.AssignLiteral("wal");
      break;
  }

  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "PRAGMA journal_mode = ") + journalMode,
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  mozStorageStatementScoper scoper(statement);
  PRBool hasResult;
  rv = statement->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(hasResult, NS_ERROR_FAILURE);

  nsCAutoString currentJournalMode;
  rv = statement->GetUTF8String(0, currentJournalMode);
  NS_ENSURE_SUCCESS(rv, rv);
  bool succeeded = currentJournalMode.Equals(journalMode);
  if (succeeded) {
    mCurrentJournalMode = aJournalMode;
  }
  else {
    NS_WARNING(nsPrintfCString(128, "Setting journal mode failed: %s",
                               PromiseFlatCString(journalMode).get()).get());
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}


nsresult
nsNavHistory::InitDB()
{
  {
    
    
    nsCOMPtr<mozIStorageStatement> statement;
    nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING("PRAGMA page_size"),
                                  getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    mozStorageStatementScoper scoper(statement);
    rv = statement->ExecuteStep(&hasResult);
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && hasResult, NS_ERROR_FAILURE);
    rv = statement->GetInt32(0, &mDBPageSize);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(mDBPageSize > 0, NS_ERROR_UNEXPECTED);
  }

  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "PRAGMA temp_store = MEMORY"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  PRInt32 cachePercentage;
  if (NS_FAILED(mPrefBranch->GetIntPref(PREF_CACHE_TO_MEMORY_PERCENTAGE,
                                        &cachePercentage)))
    cachePercentage = DATABASE_DEFAULT_CACHE_TO_MEMORY_PERCENTAGE;
  
  if (cachePercentage > 50)
    cachePercentage = 50;
  if (cachePercentage < 0)
    cachePercentage = 0;

  static PRInt64 physMem = PR_GetPhysicalMemorySize();
  PRInt64 cacheSize = physMem * cachePercentage / 100;

  
  PRInt64 cachePages = cacheSize / mDBPageSize;
  nsCAutoString cacheSizePragma("PRAGMA cache_size = ");
  cacheSizePragma.AppendInt(cachePages);
  rv = mDBConn->ExecuteSimpleSQL(cacheSizePragma);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (NS_SUCCEEDED(SetJournalMode(JOURNAL_WAL))) {
    
    
    
    
    PRInt32 checkpointPages =
      static_cast<PRInt32>(DATABASE_MAX_WAL_SIZE_IN_KIBIBYTES * 1024 / mDBPageSize);
    nsCAutoString checkpointPragma("PRAGMA wal_autocheckpoint = ");
    checkpointPragma.AppendInt(checkpointPages);
    rv = mDBConn->ExecuteSimpleSQL(checkpointPragma);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    
    
    (void)SetJournalMode(JOURNAL_TRUNCATE);

    
    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "PRAGMA synchronous = FULL"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  (void)mDBConn->SetGrowthIncrement(10 * BYTES_PER_MEBIBYTE, EmptyCString());

  
  rv = InitFunctions();
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt32 currentSchemaVersion;
  rv = mDBConn->GetSchemaVersion(&currentSchemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);
  bool databaseInitialized = currentSchemaVersion > 0;

  if (databaseInitialized && currentSchemaVersion == DATABASE_SCHEMA_VERSION) {
    
    return NS_OK;
  }

  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  if (databaseInitialized) {
    
    
    
    
    
    
    
    
    
    

    if (currentSchemaVersion < DATABASE_SCHEMA_VERSION) {
      mDatabaseStatus = DATABASE_STATUS_UPGRADED;

      

      if (currentSchemaVersion < 7) {
        rv = MigrateV7Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 8) {
        rv = MigrateV8Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 9) {
        rv = MigrateV9Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 10) {
        rv = MigrateV10Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 11) {
        rv = MigrateV11Up(mDBConn);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      
    }
  }
  else {
    
    rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_PLACES);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_URL);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_FAVICON);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_REVHOST);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_VISITCOUNT);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_FRECENCY);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_LASTVISITDATE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_GUID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_HISTORYVISITS);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_HISTORYVISITS_PLACEDATE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_HISTORYVISITS_FROMVISIT);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_HISTORYVISITS_VISITDATE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(CREATE_MOZ_INPUTHISTORY);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = nsNavBookmarks::InitTables(mDBConn);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = nsFaviconService::InitTables(mDBConn);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = nsAnnotationService::InitTables(mDBConn);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = UpdateSchemaVersion();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  ForceWALCheckpoint(mDBConn);

  
  
  
  

  return NS_OK;
}


nsresult
nsNavHistory::InitAdditionalDBItems()
{
  nsresult rv = InitTriggers();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  (void*)GetStatement(mDBPageInfoForFrecency);
  (void*)GetStatement(mDBAsyncThreadPageInfoForFrecency);
  (void*)GetStatement(mDBVisitsForFrecency);
  (void*)GetStatement(mDBAsyncThreadVisitsForFrecency);

  return NS_OK;
}


nsresult
nsNavHistory::CheckAndUpdateGUIDs()
{
  
  nsCOMPtr<mozIStorageStatement> updateStmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks "
    "SET guid = :guid "
    "WHERE id = :item_id "
  ), getter_AddRefs(updateStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT item_id, content "
    "FROM moz_items_annos "
    "JOIN moz_anno_attributes "
    "WHERE name = :anno_name "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("anno_name"),
                                  SYNCGUID_ANNO);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResult;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    PRInt64 itemId;
    rv = stmt->GetInt64(0, &itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString guid;
    rv = stmt->GetUTF8String(1, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!IsValidGUID(guid)) {
      continue;
    }

    mozStorageStatementScoper scoper(updateStmt);
    rv = updateStmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"), itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), guid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStmt->Execute();
    if (rv == NS_ERROR_STORAGE_CONSTRAINT) {
      
      
      continue;
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_items_annos "
    "WHERE anno_attribute_id = ( "
      "SELECT id "
      "FROM moz_anno_attributes "
      "WHERE name = :anno_name "
    ") "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("anno_name"),
                                  SYNCGUID_ANNO);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks "
    "SET guid = GENERATE_GUID() "
    "WHERE guid IS NULL "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_places "
    "SET guid = :guid "
    "WHERE id = :place_id "
  ), getter_AddRefs(updateStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT place_id, content "
    "FROM moz_annos "
    "JOIN moz_anno_attributes "
    "WHERE name = :anno_name "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("anno_name"),
                                  SYNCGUID_ANNO);
  NS_ENSURE_SUCCESS(rv, rv);

  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    PRInt64 placeId;
    rv = stmt->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCAutoString guid;
    rv = stmt->GetUTF8String(1, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!IsValidGUID(guid)) {
      continue;
    }

    mozStorageStatementScoper scoper(updateStmt);
    rv = updateStmt->BindInt64ByName(NS_LITERAL_CSTRING("place_id"), placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), guid);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStmt->Execute();
    if (rv == NS_ERROR_STORAGE_CONSTRAINT) {
      
      
      continue;
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_annos "
    "WHERE anno_attribute_id = ( "
      "SELECT id "
      "FROM moz_anno_attributes "
      "WHERE name = :anno_name "
    ") "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("anno_name"),
                                  SYNCGUID_ANNO);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_places "
    "SET guid = GENERATE_GUID() "
    "WHERE guid IS NULL "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::GetDatabaseStatus(PRUint16 *aDatabaseStatus)
{
  NS_ENSURE_ARG_POINTER(aDatabaseStatus);
  *aDatabaseStatus = mDatabaseStatus;
  return NS_OK;
}





nsresult
nsNavHistory::UpdateSchemaVersion()
{
  return mDBConn->SetSchemaVersion(DATABASE_SCHEMA_VERSION);
}


PRUint32
nsNavHistory::GetRecentFlags(nsIURI *aURI)
{
  PRUint32 result = 0;
  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to get aURI's spec");

  if (NS_SUCCEEDED(rv)) {
    if (CheckIsRecentEvent(&mRecentTyped, spec))
      result |= RECENT_TYPED;
    if (CheckIsRecentEvent(&mRecentLink, spec))
      result |= RECENT_ACTIVATED;
    if (CheckIsRecentEvent(&mRecentBookmark, spec))
      result |= RECENT_BOOKMARKED;
  }

  return result;
}





class mozStorageFunctionGetUnreversedHost: public mozIStorageFunction
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_MOZISTORAGEFUNCTION
};

NS_IMPL_ISUPPORTS1(mozStorageFunctionGetUnreversedHost, mozIStorageFunction)

NS_IMETHODIMP
mozStorageFunctionGetUnreversedHost::OnFunctionCall(
  mozIStorageValueArray* aFunctionArguments,
  nsIVariant** _retval)
{
  NS_ASSERTION(aFunctionArguments, "Must have non-null function args");
  NS_ASSERTION(_retval, "Must have non-null return pointer");

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
    result->SetAsAString(EmptyString());
  }
  NS_ADDREF(*_retval = result);
  return NS_OK;
}

nsresult
nsNavHistory::InitFunctions()
{
  nsCOMPtr<mozIStorageFunction> func =
    new mozStorageFunctionGetUnreversedHost;
  NS_ENSURE_TRUE(func, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = mDBConn->CreateFunction(
    NS_LITERAL_CSTRING("get_unreversed_host"), 1, func
  );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = MatchAutoCompleteFunction::create(mDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CalculateFrecencyFunction::create(mDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GenerateGUIDFunction::create(mDBConn);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsNavHistory::InitTriggers()
{
  nsresult rv = mDBConn->ExecuteSimpleSQL(CREATE_HISTORYVISITS_AFTERINSERT_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(CREATE_HISTORYVISITS_AFTERDELETE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

mozIStorageStatement*
nsNavHistory::GetStatement(const nsCOMPtr<mozIStorageStatement>& aStmt)
{
  
  if (!mCanNotify)
    return nsnull;

  RETURN_IF_STMT(mDBGetURLPageInfo, NS_LITERAL_CSTRING(
    "SELECT id, url, title, rev_host, visit_count "
    "FROM moz_places "
    "WHERE url = :page_url "
  ));

  RETURN_IF_STMT(mDBGetIdPageInfo, NS_LITERAL_CSTRING(
    "SELECT id, url, title, rev_host, visit_count "
    "FROM moz_places "
    "WHERE id = :page_id "
  ));

  RETURN_IF_STMT(mDBRecentVisitOfURL, NS_LITERAL_CSTRING(
    "SELECT id, session, visit_date "
    "FROM moz_historyvisits "
    "WHERE place_id = (SELECT id FROM moz_places WHERE url = :page_url) "
    "ORDER BY visit_date DESC "
  ));

  RETURN_IF_STMT(mDBRecentVisitOfPlace, NS_LITERAL_CSTRING(
    "SELECT id FROM moz_historyvisits "
    "WHERE place_id = :page_id "
      "AND visit_date = :visit_date "
      "AND session = :session "
  ));

  RETURN_IF_STMT(mDBInsertVisit, NS_LITERAL_CSTRING(
    "INSERT INTO moz_historyvisits "
      "(from_visit, place_id, visit_date, visit_type, session) "
    "VALUES (:from_visit, :page_id, :visit_date, :visit_type, :session) "
  ));

  RETURN_IF_STMT(mDBGetPageVisitStats, NS_LITERAL_CSTRING(
    "SELECT id, visit_count, typed, hidden "
    "FROM moz_places "
    "WHERE url = :page_url "
  ));

  RETURN_IF_STMT(mDBIsPageVisited, NS_LITERAL_CSTRING(
    "SELECT h.id "
    "FROM moz_places h "
    "WHERE url = ?1 "
      "AND EXISTS(SELECT id FROM moz_historyvisits WHERE place_id = h.id LIMIT 1) "
  ));

  RETURN_IF_STMT(mDBUpdatePageVisitStats, NS_LITERAL_CSTRING(
    "UPDATE moz_places "
    "SET hidden = :hidden, typed = :typed "
    "WHERE id = :page_id "
  ));

  
  
  
  
  RETURN_IF_STMT(mDBAddNewPage, NS_LITERAL_CSTRING(
    "INSERT OR IGNORE INTO moz_places "
      "(url, title, rev_host, hidden, typed, frecency, guid) "
    "VALUES (:page_url, :page_title, :rev_host, :hidden, :typed, :frecency, "
             "GENERATE_GUID()) "
  ));

  RETURN_IF_STMT(mDBGetTags, NS_LITERAL_CSTRING(
    "/* do not warn (bug 487594) */ "
    "SELECT GROUP_CONCAT(tag_title, ', ') "
    "FROM ( "
      "SELECT t.title AS tag_title "
      "FROM moz_bookmarks b "
      "JOIN moz_bookmarks t ON t.id = b.parent "
      "WHERE b.fk = (SELECT id FROM moz_places WHERE url = :page_url) "
        "AND t.parent = :tags_folder "
      "ORDER BY t.title COLLATE NOCASE ASC "
    ") "
  ));

  RETURN_IF_STMT(mDBGetItemsWithAnno, NS_LITERAL_CSTRING(
    "SELECT a.item_id, a.content "
    "FROM moz_anno_attributes n "
    "JOIN moz_items_annos a ON n.id = a.anno_attribute_id "
    "WHERE n.name = :anno_name "
  ));

  RETURN_IF_STMT(mDBSetPlaceTitle, NS_LITERAL_CSTRING(
    "UPDATE moz_places "
    "SET title = :page_title "
    "WHERE url = :page_url "
  ));

  
  
  
  
  
  
  
  
  nsCAutoString visitsForFrecencySQL(NS_LITERAL_CSTRING(
    "SELECT "
      "ROUND((strftime('%s','now','localtime','utc') - v.visit_date/1000000)/86400), "
      "COALESCE( "
        "(SELECT r.visit_type FROM moz_historyvisits r WHERE v.visit_type IN ") +
          nsPrintfCString("(%d,%d) ", TRANSITION_REDIRECT_PERMANENT,
                                      TRANSITION_REDIRECT_TEMPORARY) +
          NS_LITERAL_CSTRING(" AND r.id = v.from_visit), "
        "visit_type), "
      "visit_date "
    "FROM moz_historyvisits v "
    "WHERE v.place_id = :page_id "
    "ORDER BY visit_date DESC LIMIT ") +
      nsPrintfCString("%d", mNumVisitsForFrecency)
  );

  RETURN_IF_STMT(mDBVisitsForFrecency, visitsForFrecencySQL);

  RETURN_IF_STMT(mDBAsyncThreadVisitsForFrecency, visitsForFrecencySQL);

  RETURN_IF_STMT(mDBUpdateFrecency, NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET frecency = CALCULATE_FRECENCY(:page_id) "
      "WHERE id = :page_id"
  ));

  RETURN_IF_STMT(mDBUpdateHiddenOnFrecency, NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET hidden = 0 "
      "WHERE id = :page_id AND frecency <> 0"
  ));

  RETURN_IF_STMT(mDBGetPlaceVisitStats, NS_LITERAL_CSTRING(
    "SELECT typed, hidden, frecency "
    "FROM moz_places "
    "WHERE id = :page_id "
  ));

  
  nsCAutoString pageInfoForFrecencySQL(NS_LITERAL_CSTRING(
    "SELECT typed, hidden, visit_count, "
           "(SELECT count(*) FROM moz_historyvisits WHERE place_id = :page_id), "
           "(SELECT id FROM moz_bookmarks "
            "WHERE fk = :page_id "
              "AND parent NOT IN ("
                "SELECT a.item_id "
                "FROM moz_items_annos a "
                "JOIN moz_anno_attributes n ON a.anno_attribute_id = n.id "
                "WHERE n.name = :anno_name"
              ") "
            "LIMIT 1), "
           "(url > 'place:' AND url < 'place;') "
    "FROM moz_places "
    "WHERE id = :page_id "
  ));

  RETURN_IF_STMT(mDBPageInfoForFrecency, pageInfoForFrecencySQL);

  RETURN_IF_STMT(mDBAsyncThreadPageInfoForFrecency, pageInfoForFrecencySQL);

#ifdef MOZ_XUL
  RETURN_IF_STMT(mDBFeedbackIncrease, NS_LITERAL_CSTRING(
    
    "INSERT OR REPLACE INTO moz_inputhistory "
      
      "SELECT h.id, IFNULL(i.input, :input_text), IFNULL(i.use_count, 0) * .9 + 1 "
      "FROM moz_places h "
      "LEFT JOIN moz_inputhistory i ON i.place_id = h.id AND i.input = :input_text "
      "WHERE url = :page_url "));
#endif

  
  RETURN_IF_STMT(mDBVisitToVisitResult, NS_LITERAL_CSTRING(
    "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
           "v.visit_date, f.url, v.session, null, null, null, null, "
           "null, h.frecency "
    "FROM moz_places h "
    "JOIN moz_historyvisits v ON h.id = v.place_id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE v.id = :visit_id "
  ));

  
  RETURN_IF_STMT(mDBVisitToURLResult, NS_LITERAL_CSTRING(
    "SELECT h.id, h.url, h.title, h.rev_host, h.visit_count, "
           "h.last_visit_date, f.url, null, null, null, null, null, "
           "null, h.frecency "
    "FROM moz_places h "
    "JOIN moz_historyvisits v ON h.id = v.place_id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE v.id = :visit_id "
  ));

  
  RETURN_IF_STMT(mDBBookmarkToUrlResult, NS_LITERAL_CSTRING(
    "SELECT b.fk, h.url, COALESCE(b.title, h.title), "
           "h.rev_host, h.visit_count, h.last_visit_date, f.url, null, b.id, "
           "b.dateAdded, b.lastModified, b.parent, "
           "null, h.frecency "
    "FROM moz_bookmarks b "
    "JOIN moz_places h ON b.fk = h.id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    "WHERE b.id = :item_id "
  ));

  return nsnull;
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
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PLACELASTMODIFIED);
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

    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_HISTORYVISITS_PLACEDATE);
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

    
    
    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_FRECENCY);
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
        "WHERE root_name = :root_name "
      ") "
      "WHERE type = :item_type "
      "AND parent = ("
        "SELECT folder_id "
        "FROM moz_bookmarks_roots "
        "WHERE root_name = :parent_name "
      ")"),
    getter_AddRefs(moveUnfiledBookmarks));
  rv = moveUnfiledBookmarks->BindUTF8StringByName(
    NS_LITERAL_CSTRING("root_name"), NS_LITERAL_CSTRING("unfiled")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = moveUnfiledBookmarks->BindInt32ByName(
    NS_LITERAL_CSTRING("item_type"), nsINavBookmarksService::TYPE_BOOKMARK
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = moveUnfiledBookmarks->BindUTF8StringByName(
    NS_LITERAL_CSTRING("parent_name"), NS_LITERAL_CSTRING("places")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = moveUnfiledBookmarks->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> triggerDetection;
  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT name "
      "FROM sqlite_master "
      "WHERE type = 'trigger' "
      "AND name = :trigger_name"),
    getter_AddRefs(triggerDetection));
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRBool triggerExists;
  rv = triggerDetection->BindUTF8StringByName(
    NS_LITERAL_CSTRING("trigger_name"),
    NS_LITERAL_CSTRING("moz_historyvisits_afterinsert_v1_trigger")
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
              nsPrintfCString("(0,%d,%d,%d) ",
                              nsINavHistoryService::TRANSITION_EMBED,
                              nsINavHistoryService::TRANSITION_FRAMED_LINK,
                              nsINavHistoryService::TRANSITION_DOWNLOAD) +
          NS_LITERAL_CSTRING(")"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
  }

  
  rv = triggerDetection->BindUTF8StringByName(
    NS_LITERAL_CSTRING("trigger_name"),
    NS_LITERAL_CSTRING("moz_bookmarks_beforedelete_v1_trigger")
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


  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP INDEX IF EXISTS moz_places_titleindex"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP INDEX IF EXISTS moz_annos_item_idindex"));
  NS_ENSURE_SUCCESS(rv, rv);


  
  PRBool oldIndexExists = PR_FALSE;
  rv = mDBConn->IndexExists(NS_LITERAL_CSTRING("moz_annos_attributesindex"), &oldIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (oldIndexExists) {
    
    rv = mDBConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX moz_annos_attributesindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_ANNOS_PLACEATTRIBUTE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_items_annos_attributesindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_ITEMSANNOS_PLACEATTRIBUTE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return transaction.Commit();
}


nsresult
nsNavHistory::MigrateV9Up(mozIStorageConnection *aDBConn)
{
  mozStorageTransaction transaction(aDBConn, PR_FALSE);
  
  
  
  
  
  PRBool oldIndexExists = PR_FALSE;
  nsresult rv = mDBConn->IndexExists(
    NS_LITERAL_CSTRING("moz_places_lastvisitdateindex"), &oldIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!oldIndexExists) {
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_places ADD last_visit_date INTEGER"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_LASTVISITDATE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "UPDATE moz_places SET last_visit_date = "
          "(SELECT MAX(visit_date) "
           "FROM moz_historyvisits "
           "WHERE place_id = moz_places.id)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return transaction.Commit();
}


nsresult
nsNavHistory::MigrateV10Up(mozIStorageConnection *aDBConn)
{
  
  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET lastModified = dateAdded "
      "WHERE lastModified IS NULL"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
nsNavHistory::MigrateV11Up(mozIStorageConnection *aDBConn)
{
  
  
  
  nsresult rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_places SET visit_count = "
      "(SELECT count(*) FROM moz_historyvisits "
       "WHERE place_id = moz_places.id "
        "AND visit_type NOT IN ") +
          nsPrintfCString("(0,%d,%d,%d) ",
                          nsINavHistoryService::TRANSITION_EMBED,
                          nsINavHistoryService::TRANSITION_FRAMED_LINK,
                          nsINavHistoryService::TRANSITION_DOWNLOAD) +
      NS_LITERAL_CSTRING(")")
  );
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> hasGuidStatement;
  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT guid FROM moz_bookmarks"),
    getter_AddRefs(hasGuidStatement));

  if (NS_FAILED(rv)) {
    
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_bookmarks "
      "ADD COLUMN guid TEXT"
    ));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_GUID);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = aDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_places "
      "ADD COLUMN guid TEXT"
    ));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aDBConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_GUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = CheckAndUpdateGUIDs();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}













nsresult
nsNavHistory::GetUrlIdFor(nsIURI* aURI, PRInt64* aEntryID,
                          PRBool aAutoCreate)
{
  *aEntryID = 0;
  {
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetURLPageInfo);
    nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasEntry = PR_FALSE;
    rv = stmt->ExecuteStep(&hasEntry);
    NS_ENSURE_SUCCESS(rv, rv);

    if (hasEntry)
      return stmt->GetInt64(kGetInfoIndex_PageID, aEntryID);
  }

  if (aAutoCreate) {
    
    nsAutoString voidString;
    voidString.SetIsVoid(PR_TRUE);
    return InternalAddNewPage(aURI, voidString, PR_TRUE, PR_FALSE, 0, PR_TRUE, aEntryID);
  }

  
  return NS_OK;
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
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBAddNewPage);
  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aTitle.IsVoid()) {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("page_title"));
  }
  else {
    rv = stmt->BindStringByName(
      NS_LITERAL_CSTRING("page_title"), StringHead(aTitle, TITLE_LENGTH_MAX)
    );
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString revHost;
  rv = GetReversedHostname(aURI, revHost);
  
  if (NS_SUCCEEDED(rv)) {
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("rev_host"), revHost);
  } else {
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("rev_host"));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), aHidden);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), aTyped);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("frecency"),
                             IsQueryURI(spec) ? 0 : -1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 pageId = 0;
  {
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(getIdStmt, mDBGetURLPageInfo);
    rv = URIBinder::Bind(getIdStmt, NS_LITERAL_CSTRING("page_url"), aURI);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult = PR_FALSE;
    rv = getIdStmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(hasResult, "hasResult is false but the call succeeded?");
    pageId = getIdStmt->AsInt64(0);
  }

  if (aCalculateFrecency) {
    rv = UpdateFrecency(pageId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (aPageID) {
    *aPageID = pageId;
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
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBInsertVisit);
  
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("from_visit"), aReferringVisit);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPageID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("visit_date"), aTime);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("visit_type"), aTransitionType);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("session"), aSessionID);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  {
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBRecentVisitOfPlace);

    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPageID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("visit_date"), aTime);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("session"), aSessionID);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasResult;
    rv = stmt->ExecuteStep(&hasResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ASSERTION(hasResult, "hasResult is false but the call succeeded?");

    rv = stmt->GetInt64(0, visitID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  mHasHistoryEntries = -1;

  return NS_OK;
}










PRBool
nsNavHistory::FindLastVisit(nsIURI* aURI,
                            PRInt64* aVisitID,
                            PRTime* aTime,
                            PRInt64* aSessionID)
{
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBRecentVisitOfURL);
  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool hasMore;
  rv = stmt->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  if (hasMore) {
    rv = stmt->GetInt64(0, aVisitID);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    rv = stmt->GetInt64(1, aSessionID);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    rv = stmt->GetInt64(2, aTime);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    return PR_TRUE;
  }
  return PR_FALSE;
}








PRBool nsNavHistory::IsURIStringVisited(const nsACString& aURIString)
{
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBIsPageVisited);
  nsresult rv = URIBinder::Bind(stmt, 0, aURIString);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRBool hasMore = PR_FALSE;
  rv = stmt->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);
  return hasMore;
}


void
nsNavHistory::LoadPrefs()
{
  if (!mPrefBranch)
    return;

  
  
  nsCOMPtr<nsIPrefBranch> prefSvc = do_GetService(NS_PREFSERVICE_CONTRACTID);
  PRInt32 oldDaysPref = 0;
  if (prefSvc &&
      NS_SUCCEEDED(prefSvc->GetIntPref("browser.history_expire_days",
                                       &oldDaysPref))) {
    if (!oldDaysPref) {
      
      mPrefBranch->SetBoolPref(PREF_HISTORY_ENABLED, PR_FALSE);
      mHistoryEnabled = PR_FALSE;
    }
    
    prefSvc->ClearUserPref("browser.history_expire_days");
  }
  else
    mPrefBranch->GetBoolPref(PREF_HISTORY_ENABLED, &mHistoryEnabled);

  
  mPrefBranch->GetIntPref(PREF_FRECENCY_NUM_VISITS,
    &mNumVisitsForFrecency);
  mPrefBranch->GetIntPref(PREF_FRECENCY_FIRST_BUCKET_CUTOFF,
    &mFirstBucketCutoffInDays);
  mPrefBranch->GetIntPref(PREF_FRECENCY_SECOND_BUCKET_CUTOFF,
    &mSecondBucketCutoffInDays);
  mPrefBranch->GetIntPref(PREF_FRECENCY_THIRD_BUCKET_CUTOFF,
    &mThirdBucketCutoffInDays);
  mPrefBranch->GetIntPref(PREF_FRECENCY_FOURTH_BUCKET_CUTOFF,
    &mFourthBucketCutoffInDays);
  mPrefBranch->GetIntPref(PREF_FRECENCY_EMBED_VISIT_BONUS,
    &mEmbedVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_FRAMED_LINK_VISIT_BONUS,
    &mFramedLinkVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_LINK_VISIT_BONUS,
    &mLinkVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_TYPED_VISIT_BONUS,
    &mTypedVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_BOOKMARK_VISIT_BONUS,
    &mBookmarkVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_DOWNLOAD_VISIT_BONUS,
    &mDownloadVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_PERM_REDIRECT_VISIT_BONUS,
    &mPermRedirectVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_TEMP_REDIRECT_VISIT_BONUS,
    &mTempRedirectVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_DEFAULT_VISIT_BONUS,
    &mDefaultVisitBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_UNVISITED_BOOKMARK_BONUS,
    &mUnvisitedBookmarkBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_UNVISITED_TYPED_BONUS,
    &mUnvisitedTypedBonus);
  mPrefBranch->GetIntPref(PREF_FRECENCY_FIRST_BUCKET_WEIGHT,
    &mFirstBucketWeight);
  mPrefBranch->GetIntPref(PREF_FRECENCY_SECOND_BUCKET_WEIGHT,
    &mSecondBucketWeight);
  mPrefBranch->GetIntPref(PREF_FRECENCY_THIRD_BUCKET_WEIGHT,
    &mThirdBucketWeight);
  mPrefBranch->GetIntPref(PREF_FRECENCY_FOURTH_BUCKET_WEIGHT,
    &mFourthBucketWeight);
  mPrefBranch->GetIntPref(PREF_FRECENCY_DEFAULT_BUCKET_WEIGHT,
    &mDefaultWeight);
}


PRInt64
nsNavHistory::GetNewSessionID()
{
  
  if (mLastSessionID)
    return ++mLastSessionID;

  
  
  nsCOMPtr<mozIStorageStatement> selectSession;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT session FROM moz_historyvisits "
    "ORDER BY visit_date DESC "
  ), getter_AddRefs(selectSession));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool hasSession;
  if (NS_SUCCEEDED(selectSession->ExecuteStep(&hasSession)) && hasSession)
    mLastSessionID = selectSession->AsInt64(0) + 1;
  else
    mLastSessionID = 1;

  return mLastSessionID;
}


void
nsNavHistory::NotifyOnVisit(nsIURI* aURI,
                          PRInt64 aVisitID,
                          PRTime aTime,
                          PRInt64 aSessionID,
                          PRInt64 referringVisitID,
                          PRInt32 aTransitionType)
{
  PRUint32 added = 0;
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver,
                   OnVisit(aURI, aVisitID, aTime, aSessionID,
                           referringVisitID, aTransitionType, &added));
}

void
nsNavHistory::NotifyTitleChange(nsIURI* aURI, const nsString& aTitle)
{
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver, OnTitleChanged(aURI, aTitle));
}

PRInt32
nsNavHistory::GetDaysOfHistory() {
  PRInt32 daysOfHistory = 0;
  nsCOMPtr<mozIStorageStatement> statement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT ROUND(( "
        "strftime('%s','now','localtime','utc') - "
        "( "
          "SELECT visit_date FROM moz_historyvisits "
          "ORDER BY visit_date ASC LIMIT 1 "
        ")/1000000 "
      ")/86400) AS daysOfHistory "),
    getter_AddRefs(statement));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Unable to create statement.");
  NS_ENSURE_SUCCESS(rv, 0);
  PRBool hasResult;
  if (NS_SUCCEEDED(statement->ExecuteStep(&hasResult)) && hasResult)
    statement->GetInt32(0, &daysOfHistory);

  return daysOfHistory;
}


PRTime
nsNavHistory::GetNow()
{
  if (!mCachedNow) {
    mCachedNow = PR_Now();
    if (!mExpireNowTimer)
      mExpireNowTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mExpireNowTimer)
      mExpireNowTimer->InitWithFuncCallback(expireNowTimerCallback, this,
                                            RENEW_CACHED_NOW_TIMEOUT,
                                            nsITimer::TYPE_ONE_SHOT);
  }
  return mCachedNow;
}


void nsNavHistory::expireNowTimerCallback(nsITimer* aTimer, void* aClosure)
{
  nsNavHistory *history = static_cast<nsNavHistory *>(aClosure);
  if (history) {
    history->mCachedNow = 0;
    history->mExpireNowTimer = 0;
  }
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
  PRBool queryContainsTransitions = PR_FALSE;

  for (i = 0; i < aQueries.Count(); i ++) {
    nsNavHistoryQuery* query = aQueries[i];

    if (query->Folders().Length() > 0 ||
        query->OnlyBookmarked() ||
        query->Tags().Length() > 0) {
      return QUERYUPDATE_COMPLEX_WITH_BOOKMARKS;
    }

    if (query->Transitions().Length() > 0)
      queryContainsTransitions = PR_TRUE;

    
    
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

  if (queryContainsTransitions)
    return QUERYUPDATE_COMPLEX;

  
  
  
  
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
  NS_ENSURE_ARG_POINTER(aHasEntries);

  
  if (mHasHistoryEntries != -1) {
    *aHasEntries = (mHasHistoryEntries == 1);
    return NS_OK;
  }

  nsCOMPtr<mozIStorageStatement> dbSelectStatement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT 1 FROM moz_historyvisits "),
    getter_AddRefs(dbSelectStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = dbSelectStatement->ExecuteStep(aHasEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  mHasHistoryEntries = *aHasEntries ? 1 : 0;
  return NS_OK;
}


nsresult
nsNavHistory::FixInvalidFrecenciesForExcludedPlaces()
{
  
  
  
  nsCOMPtr<mozIStorageStatement> dbUpdateStatement;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET frecency = 0 WHERE id IN ("
        "SELECT h.id FROM moz_places h "
        "WHERE h.url >= 'place:' AND h.url < 'place;' "
        "UNION "
        
        "SELECT b.fk FROM moz_bookmarks b "
        "JOIN moz_bookmarks bp ON bp.id = b.parent "
        "JOIN moz_items_annos a ON a.item_id = bp.id "
        "JOIN moz_anno_attributes n ON n.id = a.anno_attribute_id "
        "WHERE n.name = :anno_name "
        "AND b.fk IN( "
          "SELECT id FROM moz_places WHERE visit_count = 0 AND frecency < 0 "
        ") "
      ")"),
    getter_AddRefs(dbUpdateStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbUpdateStatement->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_name"), NS_LITERAL_CSTRING(LMANNO_FEEDURI)
  );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbUpdateStatement->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}









NS_IMETHODIMP
nsNavHistory::MarkPageAsFollowedBookmark(nsIURI* aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
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
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(canAdd);

  
  if (IsHistoryDisabled()) {
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
      scheme.EqualsLiteral("resource") ||
      scheme.EqualsLiteral("data") ||
      scheme.EqualsLiteral("wyciwyg") ||
      scheme.EqualsLiteral("javascript")) {
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
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aVisitID);

  
  PRBool canAdd = PR_FALSE;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd) {
    *aVisitID = 0;
    return NS_OK;
  }

  
  
  if (aTransitionType == TRANSITION_EMBED) {
    registerEmbedVisit(aURI, GetNow());
    *aVisitID = 0;
    return NS_OK;
  }

  
  
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetPageVisitStats);
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool alreadyVisited = PR_FALSE;
  rv = stmt->ExecuteStep(&alreadyVisited);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 pageID = 0;
  PRInt32 hidden;
  PRInt32 typed;
  PRBool newItem = PR_FALSE; 
  if (alreadyVisited) {
    
    rv = stmt->GetInt64(0, &pageID);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 oldVisitCount = 0;
    rv = stmt->GetInt32(1, &oldVisitCount);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 oldTypedState = 0;
    rv = stmt->GetInt32(2, &oldTypedState);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool oldHiddenState = 0;
    rv = stmt->GetInt32(3, &oldHiddenState);
    NS_ENSURE_SUCCESS(rv, rv);

    
    stmt->Reset();
    scoper.Abandon();

    
    
    
    
    hidden = oldHiddenState;
    if (hidden == 1 &&
        (!GetHiddenState(aIsRedirect, aTransitionType) ||
         aTransitionType == TRANSITION_TYPED)) {
      hidden = 0; 
    }

    typed = (PRInt32)(oldTypedState == 1 || (aTransitionType == TRANSITION_TYPED));

    
    
    if (oldVisitCount == 0)
      newItem = PR_TRUE;

    
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(updateStmt, mDBUpdatePageVisitStats);
    rv = updateStmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), pageID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = updateStmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), hidden);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = updateStmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), typed);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = updateStmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    newItem = PR_TRUE;

    
    stmt->Reset();
    scoper.Abandon();

    
    
    hidden = (PRInt32)(aTransitionType == TRANSITION_EMBED ||
                       aTransitionType == TRANSITION_FRAMED_LINK ||
                       aIsRedirect);

    typed = (PRInt32)(aTransitionType == TRANSITION_TYPED);

    
    nsString voidString;
    voidString.SetIsVoid(PR_TRUE);
    rv = InternalAddNewPage(aURI, voidString, hidden == 1, typed == 1, 1,
                            PR_TRUE, &pageID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRInt64 referringVisitID = 0;
  PRInt64 referringSessionID;
  PRTime referringTime;
  PRBool referrerIsSame;
  if (aReferringURI &&
      NS_SUCCEEDED(aReferringURI->Equals(aURI, &referrerIsSame)) &&
      !referrerIsSame &&
      !FindLastVisit(aReferringURI, &referringVisitID, &referringTime, &referringSessionID)) {
    
    
    rv = AddVisit(aReferringURI, aTime - 1, nsnull, TRANSITION_LINK, PR_FALSE,
                  aSessionID, &referringVisitID);
    if (NS_FAILED(rv))
      referringVisitID = 0;
  }

  rv = InternalAddVisit(pageID, referringVisitID, aSessionID, aTime,
                        aTransitionType, aVisitID);
  transaction.Commit();

  
  
  
  (void)UpdateFrecency(pageID);

  
  
  
  if (!hidden) {
    NotifyOnVisit(aURI, *aVisitID, aTime, aSessionID, referringVisitID,
                  aTransitionType);
  }

  
  
  
  
  
  if (newItem && (aIsRedirect || aTransitionType == TRANSITION_DOWNLOAD)) {
    nsCOMPtr<nsIObserverService> obsService =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    if (obsService)
      obsService->NotifyObservers(aURI, NS_LINK_VISITED_EVENT_TOPIC, nsnull);
  }

  
  History::GetService()->NotifyVisited(aURI);

  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::GetNewQuery(nsINavHistoryQuery **_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = new nsNavHistoryQuery();
  if (! *_retval)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*_retval);
  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::GetNewQueryOptions(nsINavHistoryQueryOptions **_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG_POINTER(_retval);

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
  NS_ENSURE_ARG(aQuery);
  NS_ENSURE_ARG(aOptions);
  NS_ENSURE_ARG_POINTER(_retval);

  return ExecuteQueries(&aQuery, 1, aOptions, _retval);
}














NS_IMETHODIMP
nsNavHistory::ExecuteQueries(nsINavHistoryQuery** aQueries, PRUint32 aQueryCount,
                             nsINavHistoryQueryOptions *aOptions,
                             nsINavHistoryResult** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aQueries);
  NS_ENSURE_ARG(aOptions);
  NS_ENSURE_ARG(aQueryCount);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  
  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions);
  NS_ENSURE_TRUE(options, NS_ERROR_INVALID_ARG);

  
  nsCOMArray<nsNavHistoryQuery> queries;
  for (PRUint32 i = 0; i < aQueryCount; i ++) {
    nsCOMPtr<nsNavHistoryQuery> query = do_QueryInterface(aQueries[i], &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    queries.AppendObject(query);
  }

  nsNavBookmarks* bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);
  
  nsRefPtr<nsNavHistoryContainerResultNode> rootNode;
  PRInt64 folderId = GetSimpleBookmarksQueryFolder(queries, options);
  if (folderId) {
    
    
    nsRefPtr<nsNavHistoryResultNode> tempRootNode;
    rv = bookmarks->ResultNodeForContainer(folderId, options,
                                           getter_AddRefs(tempRootNode));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                     "Generating a generic empty node for a broken query!");
    if (NS_SUCCEEDED(rv)) {
      rootNode = tempRootNode->GetAsContainer();
    }
  }

  if (!rootNode) {
    
    
    rootNode = new nsNavHistoryQueryResultNode(EmptyCString(), EmptyCString(),
                                               queries, options);
  }

  
  nsRefPtr<nsNavHistoryResult> result;
  rv = nsNavHistoryResult::NewHistoryResult(aQueries, aQueryCount, options,
                                            rootNode, isBatching(),
                                            getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = result);
  return NS_OK;
}









static
PRBool IsOptimizableHistoryQuery(const nsCOMArray<nsNavHistoryQuery>& aQueries,
                                 nsNavHistoryQueryOptions *aOptions,
                                 PRUint16 aSortMode)
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

  if (aQuery->Tags().Length() > 0)
    return PR_FALSE;

  if (aQuery->Transitions().Length() > 0)
    return PR_FALSE;

  return PR_TRUE;
}

static
PRBool NeedToFilterResultSet(const nsCOMArray<nsNavHistoryQuery>& aQueries, 
                             nsNavHistoryQueryOptions *aOptions)
{
  
  PRUint16 resultType = aOptions->ResultType();
  if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ||
      resultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_SITE_QUERY ||
      resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_QUERY ||
      resultType == nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY)
    return PR_FALSE;

  
  
  if (aOptions->QueryType() == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS)
    return PR_TRUE;

  nsCString parentAnnotationToExclude;
  nsresult rv = aOptions->GetExcludeItemIfParentHasAnnotation(parentAnnotationToExclude);
  NS_ENSURE_SUCCESS(rv, PR_TRUE);
  if (!parentAnnotationToExclude.IsEmpty())
    return PR_TRUE;

  
  for (PRInt32 i = 0; i < aQueries.Count(); ++i) {
    if (aQueries[i]->Folders().Length() != 0) {
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
                        nsNavHistory::StringHash& aAddParams,
                        PRBool aHasSearchTerms);

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
  
  void OrderByTextColumnIndexAsc(PRInt32 aIndex);
  void OrderByTextColumnIndexDesc(PRInt32 aIndex);

  const nsCString& mConditions;
  PRBool mUseLimit;
  PRBool mHasSearchTerms;

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
    nsNavHistory::StringHash& aAddParams,
    PRBool aHasSearchTerms)
: mConditions(aConditions)
, mUseLimit(aUseLimit)
, mHasSearchTerms(aHasSearchTerms)
, mResultType(aOptions->ResultType())
, mQueryType(aOptions->QueryType())
, mIncludeHidden(aOptions->IncludeHidden())
, mRedirectsMode(aOptions->RedirectsMode())
, mSortingMode(aOptions->SortingMode())
, mMaxResults(aOptions->MaxResults())
, mSkipOrderBy(PR_FALSE)
, mAddParams(aAddParams)
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
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsCAutoString tagsSqlFragment;

  switch (mQueryType) {
    case nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY:
      GetTagsSqlFragment(history->GetTagsFolder(),
                         NS_LITERAL_CSTRING("h.id"),
                         mHasSearchTerms,
                         tagsSqlFragment);

      mQueryString = NS_LITERAL_CSTRING(
        "SELECT h.id, h.url, h.title AS page_title, h.rev_host, h.visit_count, "
        "h.last_visit_date, f.url, null, null, null, null, null, ") +
        tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency "
        "FROM moz_places h "
        "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
        
        "WHERE 1 "
          "{QUERY_OPTIONS_VISITS} {QUERY_OPTIONS_PLACES} "
          "{ADDITIONAL_CONDITIONS} ");
      break;

    case nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS:
      if (mResultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS) {
        
        
        
        
        mSkipOrderBy = PR_TRUE;

        GetTagsSqlFragment(history->GetTagsFolder(),
                           NS_LITERAL_CSTRING("b2.fk"),
                           mHasSearchTerms,
                           tagsSqlFragment);

        mQueryString = NS_LITERAL_CSTRING(
          "SELECT b2.fk, h.url, COALESCE(b2.title, h.title) AS page_title, "
            "h.rev_host, h.visit_count, h.last_visit_date, f.url, null, b2.id, "
            "b2.dateAdded, b2.lastModified, b2.parent, ") +
            tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency "
          "FROM moz_bookmarks b2 "
          "JOIN (SELECT b.fk "
                "FROM moz_bookmarks b "
                
                "WHERE b.type = 1 {ADDITIONAL_CONDITIONS} "
                ") AS seed ON b2.fk = seed.fk "
          "JOIN moz_places h ON h.id = b2.fk "
          "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
          "WHERE NOT EXISTS ( "
            "SELECT id FROM moz_bookmarks WHERE id = b2.parent AND parent = ") +
                nsPrintfCString("%lld", history->GetTagsFolder()) +
          NS_LITERAL_CSTRING(") "
          "ORDER BY b2.fk DESC, b2.lastModified DESC");
      }
      else {
        GetTagsSqlFragment(history->GetTagsFolder(),
                           NS_LITERAL_CSTRING("b.fk"),
                           mHasSearchTerms,
                           tagsSqlFragment);
        mQueryString = NS_LITERAL_CSTRING(
          "SELECT b.fk, h.url, COALESCE(b.title, h.title) AS page_title, "
            "h.rev_host, h.visit_count, h.last_visit_date, f.url, null, b.id, "
            "b.dateAdded, b.lastModified, b.parent, ") +
            tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency "
          "FROM moz_bookmarks b "
          "JOIN moz_places h ON b.fk = h.id "
          "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
          "WHERE NOT EXISTS "
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
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_TRUE(history, NS_ERROR_OUT_OF_MEMORY);
  nsCAutoString tagsSqlFragment;
  GetTagsSqlFragment(history->GetTagsFolder(),
                     NS_LITERAL_CSTRING("h.id"),
                     mHasSearchTerms,
                     tagsSqlFragment);
  mQueryString = NS_LITERAL_CSTRING(
    "SELECT h.id, h.url, h.title AS page_title, h.rev_host, h.visit_count, "
      "v.visit_date, f.url, v.session, null, null, null, null, ") +
      tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency "
    "FROM moz_places h "
    "JOIN moz_historyvisits v ON h.id = v.place_id "
    "LEFT JOIN moz_favicons f ON h.favicon_id = f.id "
    
    "WHERE 1 "
      "{QUERY_OPTIONS_VISITS} {QUERY_OPTIONS_PLACES} "
      "{ADDITIONAL_CONDITIONS} ");

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsDay()
{
  mSkipOrderBy = PR_TRUE;

  
  
  PRUint16 sortingMode = nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING;
  if (mSortingMode != nsINavHistoryQueryOptions::SORT_BY_NONE &&
      mResultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY)
    sortingMode = mSortingMode;

  PRUint16 resultType =
    mResultType == nsINavHistoryQueryOptions::RESULTS_AS_DATE_QUERY ?
      (PRUint16)nsINavHistoryQueryOptions::RESULTS_AS_URI :
      (PRUint16)nsINavHistoryQueryOptions::RESULTS_AS_SITE_QUERY;

  
  
  
  mQueryString = nsPrintfCString(1024,
     "SELECT null, "
       "'place:type=%ld&sort=%ld&beginTime='||beginTime||'&endTime='||endTime, "
      "dayTitle, null, null, beginTime, null, null, null, null, null, null "
     "FROM (", 
     resultType,
     sortingMode);

  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  PRInt32 daysOfHistory = history->GetDaysOfHistory();
  for (PRInt32 i = 0; i <= HISTORY_DATE_CONT_NUM(daysOfHistory); i++) {
    nsCAutoString dateName;
    
    
    
    
    
    
    nsCAutoString sqlFragmentContainerBeginTime, sqlFragmentContainerEndTime;
    
    nsCAutoString sqlFragmentSearchBeginTime, sqlFragmentSearchEndTime;
    switch(i) {
       case 0:
        
         history->GetStringFromName(
          NS_LITERAL_STRING("finduri-AgeInDays-is-0").get(), dateName);
        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','utc')*1000000)");
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','+1 day','utc')*1000000)");
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = sqlFragmentContainerEndTime;
         break;
       case 1:
        
         history->GetStringFromName(
          NS_LITERAL_STRING("finduri-AgeInDays-is-1").get(), dateName);
        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','-1 day','utc')*1000000)");
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','utc')*1000000)");
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = sqlFragmentContainerEndTime;
        break;
      case 2:
        
        history->GetAgeInDaysString(7,
          NS_LITERAL_STRING("finduri-AgeInDays-last-is").get(), dateName);
        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','-7 days','utc')*1000000)");
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','+1 day','utc')*1000000)");
        
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','-2 days','utc')*1000000)");
        break;
      case 3:
        
        history->GetStringFromName(
          NS_LITERAL_STRING("finduri-AgeInMonths-is-0").get(), dateName);
        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of month','utc')*1000000)");
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','+1 day','utc')*1000000)");
        
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of day','-7 days','utc')*1000000)");
         break;
       default:
        if (i == HISTORY_ADDITIONAL_DATE_CONT_NUM + 6) {
          
          history->GetAgeInDaysString(6,
            NS_LITERAL_STRING("finduri-AgeInMonths-isgreater").get(), dateName);
          
          sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
            "(datetime(0, 'unixepoch')*1000000)");
          
          sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
            "(strftime('%s','now','localtime','start of month','-5 months','utc')*1000000)");
          
          sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
          sqlFragmentSearchEndTime = sqlFragmentContainerEndTime;
          break;
        }
        PRInt32 MonthIndex = i - HISTORY_ADDITIONAL_DATE_CONT_NUM;
        
        
        PRExplodedTime tm;
        PR_ExplodeTime(PR_Now(), PR_LocalTimeParameters, &tm);
        PRUint16 currentYear = tm.tm_year;
        
        
        
        
        
        tm.tm_mday = 2;
        tm.tm_month -= MonthIndex;
        
        
        
        PR_NormalizeTime(&tm, PR_GMTParameters);
        
        
        
        if (tm.tm_year < currentYear) {
          history->GetMonthYear(tm.tm_month + 1, tm.tm_year, dateName);
        }
        else {
          history->GetMonthName(tm.tm_month + 1, dateName);
        }

        
        sqlFragmentContainerBeginTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of month','-");
        sqlFragmentContainerBeginTime.AppendInt(MonthIndex);
        sqlFragmentContainerBeginTime.Append(NS_LITERAL_CSTRING(
            " months','utc')*1000000)"));
        
        sqlFragmentContainerEndTime = NS_LITERAL_CSTRING(
          "(strftime('%s','now','localtime','start of month','-");
        sqlFragmentContainerEndTime.AppendInt(MonthIndex - 1);
        sqlFragmentContainerEndTime.Append(NS_LITERAL_CSTRING(
            " months','utc')*1000000)"));
        
        sqlFragmentSearchBeginTime = sqlFragmentContainerBeginTime;
        sqlFragmentSearchEndTime = sqlFragmentContainerEndTime;
        break;
    }

    nsPrintfCString dateParam("dayTitle%d", i);
    mAddParams.Put(dateParam, dateName);

    nsPrintfCString dayRange(1024,
      "SELECT :%s AS dayTitle, "
             "%s AS beginTime, "
             "%s AS endTime "
       "WHERE EXISTS ( "
        "SELECT id FROM moz_historyvisits "
        "WHERE visit_date >= %s "
          "AND visit_date < %s "
           "AND visit_type NOT IN (0,%d,%d) "
           "{QUERY_OPTIONS_VISITS} "
         "LIMIT 1 "
      ") ",
      dateParam.get(),
      sqlFragmentContainerBeginTime.get(),
      sqlFragmentContainerEndTime.get(),
      sqlFragmentSearchBeginTime.get(),
      sqlFragmentSearchEndTime.get(),
      nsINavHistoryService::TRANSITION_EMBED,
      nsINavHistoryService::TRANSITION_FRAMED_LINK
    );

    mQueryString.Append(dayRange);

    if (i < HISTORY_DATE_CONT_NUM(daysOfHistory))
      mQueryString.Append(NS_LITERAL_CSTRING(" UNION ALL "));
  }

  mQueryString.Append(NS_LITERAL_CSTRING(") ")); 

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsSite()
{
  nsCAutoString localFiles;

  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  history->GetStringFromName(NS_LITERAL_STRING("localhost").get(), localFiles);
  mAddParams.Put(NS_LITERAL_CSTRING("localhost"), localFiles);

  
  nsCAutoString visitsJoin;
  nsCAutoString additionalConditions;
  nsCAutoString timeConstraints;
  if (!mConditions.IsEmpty()) {
    visitsJoin.AssignLiteral("JOIN moz_historyvisits v ON v.place_id = h.id ");
    additionalConditions.AssignLiteral("{QUERY_OPTIONS_VISITS} "
                                       "{QUERY_OPTIONS_PLACES} "
                                       "{ADDITIONAL_CONDITIONS} ");
    timeConstraints.AssignLiteral("||'&beginTime='||:begin_time||"
                                    "'&endTime='||:end_time");
  }

  mQueryString = nsPrintfCString(2048,
    "SELECT null, 'place:type=%ld&sort=%ld&domain=&domainIsHost=true'%s, "
           ":localhost, :localhost, null, null, null, null, null, null, null "
    "WHERE EXISTS ( "
      "SELECT h.id FROM moz_places h "
      "%s "
      "WHERE h.hidden = 0 "
        "AND h.visit_count > 0 "
        "AND h.url BETWEEN 'file://' AND 'file:/~' "
      "%s "
      "LIMIT 1 "
    ") "
    "UNION ALL "
    "SELECT null, "
           "'place:type=%ld&sort=%ld&domain='||host||'&domainIsHost=true'%s, "
           "host, host, null, null, null, null, null, null, null "
    "FROM ( "
      "SELECT get_unreversed_host(h.rev_host) AS host "
      "FROM moz_places h "
      "%s "
      "WHERE h.hidden = 0 "
        "AND h.rev_host <> '.' "
        "AND h.visit_count > 0 "
        "%s "
      "GROUP BY h.rev_host "
      "ORDER BY host ASC "
    ") ",
    nsINavHistoryQueryOptions::RESULTS_AS_URI,
    mSortingMode,
    PromiseFlatCString(timeConstraints).get(),
    PromiseFlatCString(visitsJoin).get(),
    PromiseFlatCString(additionalConditions).get(),
    nsINavHistoryQueryOptions::RESULTS_AS_URI,
    mSortingMode,
    PromiseFlatCString(timeConstraints).get(),
    PromiseFlatCString(visitsJoin).get(),
    PromiseFlatCString(additionalConditions).get()
  );

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::SelectAsTag()
{
  nsNavHistory *history = nsNavHistory::GetHistoryService();
  NS_ENSURE_STATE(history);

  
  
  mHasDateColumns = PR_TRUE; 

  mQueryString = nsPrintfCString(2048,
    "SELECT null, 'place:folder=' || id || '&queryType=%d&type=%ld', "
           "title, null, null, null, null, null, null, dateAdded, "
           "lastModified, null, null "
    "FROM moz_bookmarks "
    "WHERE parent = %lld",
    nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS,
    nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS,
    history->GetTagsFolder()
  );

  return NS_OK;
}

nsresult
PlacesSQLQueryBuilder::Where()
{

  
  nsCAutoString additionalVisitsConditions;
  nsCAutoString additionalPlacesConditions;

  if (mRedirectsMode == nsINavHistoryQueryOptions::REDIRECTS_MODE_SOURCE) {
    additionalVisitsConditions += NS_LITERAL_CSTRING(
      "AND visit_type NOT IN ") +
      nsPrintfCString("(%d,%d) ", nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                                  nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY);
  }
  else if (mRedirectsMode == nsINavHistoryQueryOptions::REDIRECTS_MODE_TARGET) {
    additionalPlacesConditions += NS_LITERAL_CSTRING(
      "AND NOT EXISTS ( "
        "SELECT 1 FROM moz_historyvisits v1 "
        "JOIN moz_historyvisits v2 ON v2.from_visit = v1.id "
        "WHERE v1.place_id = h.id "
        "AND v2.visit_type IN ") +
        nsPrintfCString("(%d,%d) ", nsINavHistoryService::TRANSITION_REDIRECT_PERMANENT,
                                    nsINavHistoryService::TRANSITION_REDIRECT_TEMPORARY) +
      NS_LITERAL_CSTRING(") ");
  }

  if (!mIncludeHidden) {
    additionalPlacesConditions += NS_LITERAL_CSTRING("AND hidden = 0 ");
  }

  if (mQueryType == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY) {
    
    
    additionalPlacesConditions += NS_LITERAL_CSTRING(
      "AND last_visit_date NOTNULL "
    );
  }

  if (mResultType == nsINavHistoryQueryOptions::RESULTS_AS_URI &&
      !additionalVisitsConditions.IsEmpty()) {
    
    nsCAutoString tmp = additionalVisitsConditions;
    additionalVisitsConditions = "AND EXISTS (SELECT 1 FROM moz_historyvisits WHERE place_id = h.id ";
    additionalVisitsConditions.Append(tmp);
    additionalVisitsConditions.Append("LIMIT 1)");
  }

  mQueryString.ReplaceSubstring("{QUERY_OPTIONS_VISITS}",
                                additionalVisitsConditions.get());
  mQueryString.ReplaceSubstring("{QUERY_OPTIONS_PLACES}",
                                additionalPlacesConditions.get());

  
  
  if (mQueryString.Find("{ADDITIONAL_CONDITIONS}", 0) != kNotFound) {
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
      
      if (mResultType == nsINavHistoryQueryOptions::RESULTS_AS_URI) {
        if (mQueryType == nsINavHistoryQueryOptions::QUERY_TYPE_BOOKMARKS)
          mQueryString += NS_LITERAL_CSTRING(" ORDER BY b.id ASC ");
        else if (mQueryType == nsINavHistoryQueryOptions::QUERY_TYPE_HISTORY)
          mQueryString += NS_LITERAL_CSTRING(" ORDER BY h.id ASC ");
      }
      break;
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING:
    case nsINavHistoryQueryOptions::SORT_BY_TITLE_DESCENDING:
      
      
      
      
      
      
      if (mMaxResults > 0)
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_VisitDate);
      else if (mSortingMode == nsINavHistoryQueryOptions::SORT_BY_TITLE_ASCENDING)
        OrderByTextColumnIndexAsc(nsNavHistory::kGetInfoIndex_Title);
      else
        OrderByTextColumnIndexDesc(nsNavHistory::kGetInfoIndex_Title);
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
    case nsINavHistoryQueryOptions::SORT_BY_FRECENCY_ASCENDING:
        OrderByColumnIndexAsc(nsNavHistory::kGetInfoIndex_Frecency);
      break;
    case nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING:
        OrderByColumnIndexDesc(nsNavHistory::kGetInfoIndex_Frecency);
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

void PlacesSQLQueryBuilder::OrderByTextColumnIndexAsc(PRInt32 aIndex)
{
  mQueryString += nsPrintfCString(128, " ORDER BY %d COLLATE NOCASE ASC",
                                  aIndex+1);
}

void PlacesSQLQueryBuilder::OrderByTextColumnIndexDesc(PRInt32 aIndex)
{
  mQueryString += nsPrintfCString(128, " ORDER BY %d COLLATE NOCASE DESC",
                                  aIndex+1);
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
  NS_ASSERTION(sortingMode >= nsINavHistoryQueryOptions::SORT_BY_NONE &&
               sortingMode <= nsINavHistoryQueryOptions::SORT_BY_FRECENCY_DESCENDING,
               "Invalid sortingMode found while building query!");

  PRBool hasSearchTerms = PR_FALSE;
  for (PRInt32 i = 0; i < aQueries.Count() && !hasSearchTerms; i++) {
    aQueries[i]->GetHasSearchTerms(&hasSearchTerms);
  }

  nsCAutoString tagsSqlFragment;
  GetTagsSqlFragment(GetTagsFolder(),
                     NS_LITERAL_CSTRING("h.id"),
                     hasSearchTerms,
                     tagsSqlFragment);

  if (IsOptimizableHistoryQuery(aQueries, aOptions,
        nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING) ||
      IsOptimizableHistoryQuery(aQueries, aOptions,
        nsINavHistoryQueryOptions::SORT_BY_VISITCOUNT_DESCENDING)) {
    
    
    queryString = NS_LITERAL_CSTRING(
      "SELECT h.id, h.url, h.title AS page_title, h.rev_host, h.visit_count, h.last_visit_date, "
          "f.url, null, null, null, null, null, ") +
          tagsSqlFragment + NS_LITERAL_CSTRING(", h.frecency "
        "FROM moz_places h "
        "LEFT OUTER JOIN moz_favicons f ON h.favicon_id = f.id "
        "WHERE h.hidden = 0 "
          "AND EXISTS (SELECT id FROM moz_historyvisits WHERE place_id = h.id "
                       "AND visit_type NOT IN ") +
                       nsPrintfCString("(0,%d,%d) ",
                                       nsINavHistoryService::TRANSITION_EMBED,
                                       nsINavHistoryService::TRANSITION_FRAMED_LINK) +
                       NS_LITERAL_CSTRING("LIMIT 1) "
          "{QUERY_OPTIONS} "
        );

    queryString.Append(NS_LITERAL_CSTRING("ORDER BY "));
    if (sortingMode == nsINavHistoryQueryOptions::SORT_BY_DATE_DESCENDING)
      queryString.Append(NS_LITERAL_CSTRING("last_visit_date DESC "));
    else
      queryString.Append(NS_LITERAL_CSTRING("visit_count DESC "));

    queryString.Append(NS_LITERAL_CSTRING("LIMIT "));
    queryString.AppendInt(aOptions->MaxResults());

    nsCAutoString additionalQueryOptions;
    if (aOptions->RedirectsMode() ==
          nsINavHistoryQueryOptions::REDIRECTS_MODE_SOURCE) {
      additionalQueryOptions +=  nsPrintfCString(256,
        "AND NOT EXISTS ( "
          "SELECT id FROM moz_historyvisits WHERE place_id = h.id "
                                             "AND visit_type IN (%d,%d)"
        ") ",
        TRANSITION_REDIRECT_PERMANENT,
        TRANSITION_REDIRECT_TEMPORARY);
    }
    else if (aOptions->RedirectsMode() ==
              nsINavHistoryQueryOptions::REDIRECTS_MODE_TARGET) {
      additionalQueryOptions += nsPrintfCString(1024,
        "AND NOT EXISTS ( "
          "SELECT id "
          "FROM moz_historyvisits v "
          "WHERE place_id = h.id "
            "AND EXISTS(SELECT id FROM moz_historyvisits "
                           "WHERE from_visit = v.id AND visit_type IN (%d,%d)) "
        ") ",
        TRANSITION_REDIRECT_PERMANENT,
        TRANSITION_REDIRECT_TEMPORARY);
    }
    queryString.ReplaceSubstring("{QUERY_OPTIONS}",
                                  additionalQueryOptions.get());
    return NS_OK;
  }

  nsCAutoString conditions;
  for (PRInt32 i = 0; i < aQueries.Count(); i++) {
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
                                           useLimitClause, aAddParams,
                                           hasSearchTerms);
  rv = queryStringBuilder.GetQueryString(queryString);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PLDHashOperator BindAdditionalParameter(nsNavHistory::StringHash::KeyType aParamName, 
                                        nsCString aParamValue,
                                        void* aStatement)
{
  mozIStorageStatement* stmt = static_cast<mozIStorageStatement*>(aStatement);

  nsresult rv = stmt->BindUTF8StringByName(aParamName, aParamValue);
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
  addParams.Init(HISTORY_DATE_CONT_MAX);
  nsresult rv = ConstructQueryString(aQueries, aOptions, queryString, 
                                     paramsPresent, addParams);
  NS_ENSURE_SUCCESS(rv,rv);

  
  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(queryString, getter_AddRefs(statement));
#ifdef DEBUG
  if (NS_FAILED(rv)) {
    nsCAutoString lastErrorString;
    (void)mDBConn->GetLastErrorString(lastErrorString);
    PRInt32 lastError = 0;
    (void)mDBConn->GetLastError(&lastError);
    printf("Places failed to create a statement from this query:\n%s\nStorage error (%d): %s\n",
           PromiseFlatCString(queryString).get(),
           lastError,
           PromiseFlatCString(lastErrorString).get());
  }
#endif
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
  NS_ENSURE_ARG(aObserver);

  return mObservers.AppendWeakElement(aObserver, aOwnsWeak);
}




NS_IMETHODIMP
nsNavHistory::RemoveObserver(nsINavHistoryObserver* aObserver)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aObserver);

  return mObservers.RemoveWeakElement(aObserver);
}



nsresult
nsNavHistory::BeginUpdateBatch()
{
  if (mBatchLevel++ == 0) {
    mBatchDBTransaction = new mozStorageTransaction(mDBConn, PR_FALSE);

    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver, OnBeginUpdateBatch());
  }
  return NS_OK;
}


nsresult
nsNavHistory::EndUpdateBatch()
{
  if (--mBatchLevel == 0) {
    if (mBatchDBTransaction) {
      nsresult rv = mBatchDBTransaction->Commit();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Batch failed to commit transaction");
      delete mBatchDBTransaction;
      mBatchDBTransaction = nsnull;
    }

    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver, OnEndUpdateBatch());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::RunInBatchMode(nsINavHistoryBatchCallback* aCallback,
                             nsISupports* aUserData)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aCallback);

  UpdateBatchScoper batch(*this);
  return aCallback->RunBatched(aUserData);
}

NS_IMETHODIMP
nsNavHistory::GetHistoryDisabled(PRBool *_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = IsHistoryDisabled();
  return NS_OK;
}











NS_IMETHODIMP
nsNavHistory::AddPageWithDetails(nsIURI *aURI, const PRUnichar *aTitle,
                                 PRInt64 aLastVisited)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
  if (InPrivateBrowsingMode())
    return NS_OK;

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
      "SELECT url FROM moz_places "
      "WHERE hidden = 0 "
        "AND last_visit_date NOTNULL "
      "ORDER BY last_visit_date DESC "),
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMatch = PR_FALSE;
  if (NS_SUCCEEDED(statement->ExecuteStep(&hasMatch)) && hasMatch)
    return statement->GetUTF8String(0, aLastPageVisited);

  aLastPageVisited.Truncate(0);
  return NS_OK;
}









NS_IMETHODIMP
nsNavHistory::GetCount(PRUint32 *aCount)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG_POINTER(aCount);

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

  nsresult rv = PreparePlacesForVisitsDelete(aPlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits WHERE place_id IN (") +
        aPlaceIdsQueryString +
        NS_LITERAL_CSTRING(")"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CleanupPlacesOnVisitsDelete(aPlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mHasHistoryEntries = -1;

  return transaction.Commit();
}












nsresult
nsNavHistory::PreparePlacesForVisitsDelete(const nsCString& aPlaceIdsQueryString)
{
  
  if (aPlaceIdsQueryString.IsEmpty())
    return NS_OK;

  
  
  
  
  
  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET frecency = -MAX(visit_count, 1) "
      "WHERE id IN ( "
        "SELECT h.id " 
        "FROM moz_places h "
        "WHERE h.id IN ( ") + aPlaceIdsQueryString + NS_LITERAL_CSTRING(") "
          "AND ( "
            "EXISTS (SELECT b.id FROM moz_bookmarks b WHERE b.fk =h.id) "
            "OR EXISTS (SELECT a.id FROM moz_annos a WHERE a.place_id = h.id) "
          ") "        
      ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}













nsresult
nsNavHistory::CleanupPlacesOnVisitsDelete(const nsCString& aPlaceIdsQueryString)
{
  
  if (aPlaceIdsQueryString.IsEmpty())
    return NS_OK;

  
  
  
  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_places WHERE id IN ( "
        "SELECT h.id FROM moz_places h "
        "WHERE h.id IN ( ") + aPlaceIdsQueryString + NS_LITERAL_CSTRING(") "
          "AND SUBSTR(h.url, 1, 6) <> 'place:' "
          "AND NOT EXISTS "
            "(SELECT b.id FROM moz_bookmarks b WHERE b.fk = h.id LIMIT 1) "
    ")"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = FixInvalidFrecenciesForExcludedPlaces();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}










NS_IMETHODIMP
nsNavHistory::RemovePages(nsIURI **aURIs, PRUint32 aLength, PRBool aDoBatchNotify)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURIs);

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

  
  clearEmbedVisits();

  
  if (aDoBatchNotify)
    UpdateBatchScoper batch(*this); 

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::RemovePage(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver, OnBeforeDeleteURI(aURI));

  nsIURI** URIs = &aURI;
  nsresult rv = RemovePages(URIs, 1, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver, OnDeleteURI(aURI));
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
      "SELECT id FROM moz_places "
      "WHERE ") + conditionString,
    getter_AddRefs(statement));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = statement->BindStringByIndex(0, revHostDot);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aEntireDomain) {
    rv = statement->BindStringByIndex(1, revHostSlash);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCString hostPlaceIds;
  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(statement->ExecuteStep(&hasMore)) && hasMore) {
    if (!hostPlaceIds.IsEmpty())
      hostPlaceIds.AppendLiteral(",");
    PRInt64 placeId;
    rv = statement->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    hostPlaceIds.AppendInt(placeId);
  }

  rv = RemovePagesInternal(hostPlaceIds);
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  
  UpdateBatchScoper batch(*this); 

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
      "SELECT h.id FROM moz_places h WHERE "
        "EXISTS "
          "(SELECT id FROM moz_historyvisits v WHERE v.place_id = h.id "
            "AND v.visit_date >= :from_date AND v.visit_date <= :to_date LIMIT 1)"),
    getter_AddRefs(selectByTime));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = selectByTime->BindInt64ByName(NS_LITERAL_CSTRING("from_date"), aBeginTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = selectByTime->BindInt64ByName(NS_LITERAL_CSTRING("to_date"), aEndTime);
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

  
  clearEmbedVisits();

  
  UpdateBatchScoper batch(*this); 

  return NS_OK;
}
















NS_IMETHODIMP
nsNavHistory::RemoveVisitsByTimeframe(PRTime aBeginTime, PRTime aEndTime)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  nsresult rv;

  
  
  
  nsCString deletePlaceIdsQueryString;
  {
    nsCOMPtr<mozIStorageStatement> selectByTime;
    mozStorageStatementScoper scope(selectByTime);
    rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT place_id "
        "FROM moz_historyvisits "
        "WHERE :from_date <= visit_date AND visit_date <= :to_date "
        "EXCEPT "
        "SELECT place_id "
        "FROM moz_historyvisits "
        "WHERE visit_date < :from_date OR :to_date < visit_date"),
      getter_AddRefs(selectByTime));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = selectByTime->BindInt64ByName(NS_LITERAL_CSTRING("from_date"), aBeginTime);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = selectByTime->BindInt64ByName(NS_LITERAL_CSTRING("to_date"), aEndTime);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore = PR_FALSE;
    while (NS_SUCCEEDED(selectByTime->ExecuteStep(&hasMore)) && hasMore) {
      PRInt64 placeId;
      rv = selectByTime->GetInt64(0, &placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      
      if (placeId > 0) {
        if (!deletePlaceIdsQueryString.IsEmpty())
          deletePlaceIdsQueryString.AppendLiteral(",");
        deletePlaceIdsQueryString.AppendInt(placeId);
      }
    }
  }

  
  UpdateBatchScoper batch(*this); 

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  rv = PreparePlacesForVisitsDelete(deletePlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageStatement> deleteVisitsStmt;
  rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits "
      "WHERE :from_date <= visit_date AND visit_date <= :to_date"),
    getter_AddRefs(deleteVisitsStmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteVisitsStmt->BindInt64ByName(NS_LITERAL_CSTRING("from_date"), aBeginTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteVisitsStmt->BindInt64ByName(NS_LITERAL_CSTRING("to_date"), aEndTime);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteVisitsStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CleanupPlacesOnVisitsDelete(deletePlaceIdsQueryString);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  
  mHasHistoryEntries = -1;

  return NS_OK;
}






NS_IMETHODIMP
nsNavHistory::RemoveAllPages()
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  
  
  
  
  nsresult rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_places SET frecency = -MAX(visit_count, 1) "
    "WHERE id IN(SELECT b.fk FROM moz_bookmarks b WHERE b.fk NOTNULL)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mDBConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DELETE FROM moz_historyvisits"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  rv = FixInvalidFrecenciesForExcludedPlaces();
  if (NS_FAILED(rv))
    NS_WARNING("failed to fix invalid frecencies");

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  
  clearEmbedVisits();

  
  mHasHistoryEntries = -1;

  
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver, OnClearHistory());

  
  nsCOMPtr<nsIFile> oldHistoryFile;
  rv = NS_GetSpecialDirectory(NS_APP_HISTORY_50_FILE,
                              getter_AddRefs(oldHistoryFile));
  NS_ENSURE_SUCCESS(rv, rv);

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
  NS_ENSURE_ARG(aURI);

  return NS_ERROR_NOT_IMPLEMENTED;
}









NS_IMETHODIMP
nsNavHistory::MarkPageAsTyped(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
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
nsNavHistory::MarkPageAsFollowedLink(nsIURI *aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
  if (IsHistoryDisabled())
    return NS_OK;

  nsCAutoString uriString;
  nsresult rv = aURI->GetSpec(uriString);
  NS_ENSURE_SUCCESS(rv, rv);

  
  PRInt64 unusedEventTime;
  if (mRecentLink.Get(uriString, &unusedEventTime))
    mRecentLink.Remove(uriString);

  if (mRecentLink.Count() > RECENT_EVENT_QUEUE_MAX_LENGTH)
    ExpireNonrecentEvents(&mRecentLink);

  mRecentLink.Put(uriString, GetNow());
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistory::RegisterOpenPage(nsIURI* aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  nsCOMPtr<mozIPlacesAutoComplete> ac =
    do_GetService("@mozilla.org/autocomplete/search;1?name=history");
  NS_ENSURE_STATE(ac);

  nsresult rv = ac->RegisterOpenPage(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP
nsNavHistory::UnregisterOpenPage(nsIURI* aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  nsCOMPtr<mozIPlacesAutoComplete> ac =
    do_GetService("@mozilla.org/autocomplete/search;1?name=history");
  NS_ENSURE_STATE(ac);

  nsresult rv = ac->UnregisterOpenPage(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}







NS_IMETHODIMP
nsNavHistory::SetCharsetForURI(nsIURI* aURI,
                               const nsAString& aCharset)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

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
    if (rv == NS_ERROR_INVALID_ARG) {
      
      return NS_OK;
    }
    else if (NS_FAILED(rv))
      return rv;
  }

  return NS_OK;
}






NS_IMETHODIMP
nsNavHistory::GetCharsetForURI(nsIURI* aURI, 
                               nsAString& aCharset)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

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
  NS_ENSURE_ARG(aURI);

  
  PRBool canAdd = PR_FALSE;
  nsresult rv = CanAddURI(aURI, &canAdd);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!canAdd)
    return NS_OK;

  PRTime now = PR_Now();

  rv = AddURIInternal(aURI, now, aRedirect, aToplevel, aReferrer);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}






nsresult
nsNavHistory::AddURIInternal(nsIURI* aURI, PRTime aTime, PRBool aRedirect,
                             PRBool aToplevel, nsIURI* aReferrer)
{
  mozStorageTransaction transaction(mDBConn, PR_FALSE);

  PRInt64 visitID = 0;
  PRInt64 sessionID = 0;
  nsresult rv = AddVisitChain(aURI, aTime, aToplevel, aRedirect, aReferrer,
                              &visitID, &sessionID);
  NS_ENSURE_SUCCESS(rv, rv);

  return transaction.Commit();
}
















nsresult
nsNavHistory::AddVisitChain(nsIURI* aURI,
                            PRTime aTime,
                            PRBool aToplevel,
                            PRBool aIsRedirect,
                            nsIURI* aReferrerURI,
                            PRInt64* aVisitID,
                            PRInt64* aSessionID)
{
  
  
  nsCOMPtr<nsIURI> fromVisitURI = aReferrerURI;

  nsCAutoString spec;
  nsresult rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  PRBool isEmbedVisit = !aToplevel &&
                        !CheckIsRecentEvent(&mRecentLink, spec);

  
  PRUint32 transitionType = 0;
  PRTime redirectTime = 0;
  nsCAutoString redirectSourceUrl;
  if (GetRedirectFor(spec, redirectSourceUrl, &redirectTime, &transitionType)) {
    
    
    nsCOMPtr<nsIURI> redirectSourceURI;
    rv = NS_NewURI(getter_AddRefs(redirectSourceURI), redirectSourceUrl);
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRBool redirectIsSame;
    if (NS_SUCCEEDED(aURI->Equals(redirectSourceURI, &redirectIsSame)) &&
        redirectIsSame)
      return NS_OK;

    
    
    
    
    PRTime sourceTime = NS_MIN(redirectTime, aTime - 1);
    PRInt64 sourceVisitId = 0;
    rv = AddVisitChain(redirectSourceURI, sourceTime, aToplevel,
                       PR_TRUE, 
                       aReferrerURI, 
                       &sourceVisitId, 
                       aSessionID);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (isEmbedVisit)
      transitionType = nsINavHistoryService::TRANSITION_EMBED;
    else if (!aToplevel)
      transitionType = nsINavHistoryService::TRANSITION_FRAMED_LINK;

    
    
    
    
    
    fromVisitURI = redirectSourceURI;
  }
  else if (aReferrerURI) {
    

    
    PRTime lastVisitTime;
    PRInt64 referringVisitId;
    PRBool referrerHasPreviousVisit =
      FindLastVisit(aReferrerURI, &referringVisitId, &lastVisitTime, aSessionID);

    
    
    
    
    PRBool referrerIsSame;
    if (NS_SUCCEEDED(aURI->Equals(aReferrerURI, &referrerIsSame)) &&
        referrerIsSame && referrerHasPreviousVisit) {
      
      if (aIsRedirect)
        *aSessionID = GetNewSessionID();
      return NS_OK;
    }

    if (!referrerHasPreviousVisit ||
        aTime - lastVisitTime > RECENT_EVENT_THRESHOLD) {
      
      
      *aSessionID = GetNewSessionID();
    }

    
    
    
    
    
    if (isEmbedVisit)
      transitionType = nsINavHistoryService::TRANSITION_EMBED;
    else if (!aToplevel)
      transitionType = nsINavHistoryService::TRANSITION_FRAMED_LINK;
    else
      transitionType = nsINavHistoryService::TRANSITION_LINK;
  }
  else {
    
    
    
    
    
    
    
    if (CheckIsRecentEvent(&mRecentTyped, spec))
      transitionType = nsINavHistoryService::TRANSITION_TYPED;
    else if (CheckIsRecentEvent(&mRecentBookmark, spec))
      transitionType = nsINavHistoryService::TRANSITION_BOOKMARK;
    else if (isEmbedVisit)
      transitionType = nsINavHistoryService::TRANSITION_EMBED;
    else if (!aToplevel)
      transitionType = nsINavHistoryService::TRANSITION_FRAMED_LINK;
    else
      transitionType = nsINavHistoryService::TRANSITION_LINK;

    
    
    *aSessionID = GetNewSessionID();
  }

  NS_WARN_IF_FALSE(transitionType > 0, "Visit must have a transition type");

  
  return AddVisit(aURI, aTime, fromVisitURI, transitionType,
                  aIsRedirect, *aSessionID, aVisitID);
}








NS_IMETHODIMP
nsNavHistory::IsVisited(nsIURI *aURI, PRBool *_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  
  if (IsHistoryDisabled()) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  nsCAutoString utf8URISpec;
  nsresult rv = aURI->GetSpec(utf8URISpec);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = hasEmbedVisit(aURI) ? PR_TRUE : IsURIStringVisited(utf8URISpec);
  return NS_OK;
}













NS_IMETHODIMP
nsNavHistory::SetPageTitle(nsIURI* aURI,
                           const nsAString& aTitle)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  
  if (InPrivateBrowsingMode())
    return NS_OK;

  
  
  

  nsresult rv;
  if (aTitle.IsEmpty()) {
    
    nsString voidString;
    voidString.SetIsVoid(PR_TRUE);
    rv = SetPageTitleInternal(aURI, voidString);
  }
  else {
    rv = SetPageTitleInternal(aURI, aTitle);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsNavHistory::GetPageTitle(nsIURI* aURI, nsAString& aTitle)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

  aTitle.Truncate(0);

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetURLPageInfo);
  nsresult rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasResults = PR_FALSE;
  rv = stmt->ExecuteStep(&hasResults);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!hasResults) {
    aTitle.SetIsVoid(PR_TRUE);
    return NS_OK; 
  }

  rv = stmt->GetString(nsNavHistory::kGetInfoIndex_Title, aTitle);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





NS_IMETHODIMP
nsNavHistory::GetURIGeckoFlags(nsIURI* aURI, PRUint32* aResult)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG_POINTER(aResult);

  return NS_ERROR_NOT_IMPLEMENTED;
}






NS_IMETHODIMP
nsNavHistory::SetURIGeckoFlags(nsIURI* aURI, PRUint32 aFlags)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aURI);

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
                                  PRBool aToplevel)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aOldChannel);
  NS_ENSURE_ARG(aNewChannel);

  
  
  
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





NS_IMETHODIMP
nsNavHistory::GetDatabaseConnection(mozIStorageConnection** _DBConnection)
{
  return GetDBConnection(_DBConnection);
}


NS_IMETHODIMP
nsNavHistory::GetExpectedDatabasePageSize(PRInt32* _expectedPageSize)
{
  *_expectedPageSize = mozIStorageConnection::DEFAULT_PAGE_SIZE;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistory::OnBeginVacuum(PRBool* _vacuumGranted)
{
  
  
  *_vacuumGranted = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP
nsNavHistory::OnEndVacuum(PRBool aSucceeded)
{
  NS_WARN_IF_FALSE(aSucceeded, "Places.sqlite vacuum failed.");
  return NS_OK;
}





NS_IMETHODIMP
nsNavHistory::AddDownload(nsIURI* aSource, nsIURI* aReferrer,
                          PRTime aStartTime)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aSource);

  
  if (IsHistoryDisabled())
    return NS_OK;

  PRInt64 visitID;
  return AddVisit(aSource, aStartTime, aReferrer, TRANSITION_DOWNLOAD, PR_FALSE,
                  0, &visitID);
}





NS_IMETHODIMP
nsNavHistory::GetDBConnection(mozIStorageConnection **_DBConnection)
{
  NS_ENSURE_ARG_POINTER(_DBConnection);
  NS_ADDREF(*_DBConnection = mDBConn);
  return NS_OK;
}


nsresult
nsNavHistory::FinalizeInternalStatements()
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  
  nsresult rv = FinalizeStatements();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
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
nsNavHistory::AsyncExecuteLegacyQueries(nsINavHistoryQuery** aQueries,
                                        PRUint32 aQueryCount,
                                        nsINavHistoryQueryOptions* aOptions,
                                        mozIStorageStatementCallback* aCallback,
                                        mozIStoragePendingStatement** _stmt)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");
  NS_ENSURE_ARG(aQueries);
  NS_ENSURE_ARG(aOptions);
  NS_ENSURE_ARG(aCallback);
  NS_ENSURE_ARG_POINTER(_stmt);

  nsCOMArray<nsNavHistoryQuery> queries;
  for (PRUint32 i = 0; i < aQueryCount; i ++) {
    nsCOMPtr<nsNavHistoryQuery> query = do_QueryInterface(aQueries[i]);
    NS_ENSURE_STATE(query);
    queries.AppendObject(query);
  }
  NS_ENSURE_ARG_MIN(queries.Count(), 1);

  nsCOMPtr<nsNavHistoryQueryOptions> options = do_QueryInterface(aOptions);
  NS_ENSURE_ARG(options);

  nsCString queryString;
  PRBool paramsPresent = PR_FALSE;
  nsNavHistory::StringHash addParams;
  addParams.Init(HISTORY_DATE_CONT_MAX);
  nsresult rv = ConstructQueryString(queries, options, queryString,
                                     paramsPresent, addParams);
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<mozIStorageStatement> statement;
  rv = mDBConn->CreateStatement(queryString, getter_AddRefs(statement));
#ifdef DEBUG
  if (NS_FAILED(rv)) {
    nsCAutoString lastErrorString;
    (void)mDBConn->GetLastErrorString(lastErrorString);
    PRInt32 lastError = 0;
    (void)mDBConn->GetLastError(&lastError);
    printf("Places failed to create a statement from this query:\n%s\nStorage error (%d): %s\n",
           PromiseFlatCString(queryString).get(),
           lastError,
           PromiseFlatCString(lastErrorString).get());
  }
#endif
  NS_ENSURE_SUCCESS(rv, rv);

  if (paramsPresent) {
    
    PRInt32 i;
    for (i = 0; i < queries.Count(); i++) {
      rv = BindQueryClauseParameters(statement, i, queries[i], options);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  addParams.EnumerateRead(BindAdditionalParameter, statement.get());

  rv = statement->ExecuteAsync(aCallback, _stmt);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




NS_IMETHODIMP
nsNavHistory::NotifyOnPageExpired(nsIURI *aURI, PRTime aVisitTime,
                                  PRBool aWholeEntry)
{
  
  mHasHistoryEntries = -1;

  if (aWholeEntry) {
    
    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver, OnDeleteURI(aURI));
  }
  else {
    
    NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                     nsINavHistoryObserver, OnDeleteVisits(aURI, aVisitTime));
  }

  return NS_OK;
}



NS_IMETHODIMP
nsNavHistory::Observe(nsISupports *aSubject, const char *aTopic,
                    const PRUnichar *aData)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  if (strcmp(aTopic, TOPIC_PROFILE_TEARDOWN) == 0) {
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (!os) {
      NS_WARNING("Unable to shutdown Places: Observer Service unavailable.");
      return NS_OK;
    }

    (void)os->RemoveObserver(this, TOPIC_PROFILE_TEARDOWN);
    (void)os->RemoveObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC);
    (void)os->RemoveObserver(this, TOPIC_IDLE_DAILY);
#ifdef MOZ_XUL
    (void)os->RemoveObserver(this, TOPIC_AUTOCOMPLETE_FEEDBACK_INCOMING);
#endif

    
    
    
    
    nsCOMPtr<nsISimpleEnumerator> e;
    nsresult rv = os->EnumerateObservers(TOPIC_PLACES_INIT_COMPLETE,
                                         getter_AddRefs(e));
    if (NS_SUCCEEDED(rv) && e) {
      nsCOMPtr<nsIObserver> observer;
      PRBool loop = PR_TRUE;
      while(NS_SUCCEEDED(e->HasMoreElements(&loop)) && loop) {
        e->GetNext(getter_AddRefs(observer));
        (void)observer->Observe(observer, TOPIC_PLACES_INIT_COMPLETE, nsnull);
      }
    }

    
    (void)os->NotifyObservers(nsnull, TOPIC_PLACES_SHUTDOWN, nsnull);
  }

  else if (strcmp(aTopic, TOPIC_PROFILE_CHANGE) == 0) {
    
    nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
    if (os) {
      (void)os->RemoveObserver(this, TOPIC_PROFILE_CHANGE);
      
      
      
      
      (void)os->NotifyObservers(nsnull, TOPIC_PLACES_WILL_CLOSE_CONNECTION, nsnull);
      (void)os->NotifyObservers(nsnull, TOPIC_PLACES_CONNECTION_CLOSING, nsnull);
    }

    
    
    

    
    
    mCanNotify = false;

    
    if (mPrefBranch)
      mPrefBranch->RemoveObserver("", this);

    
    nsresult rv = FinalizeInternalStatements();
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
    { 
      nsCOMPtr<mozIStorageStatement> stmt;
      nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT * "
        "FROM moz_places "
        "WHERE guid IS NULL "
      ), getter_AddRefs(stmt));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool haveNullGuids;
      rv = stmt->ExecuteStep(&haveNullGuids);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ASSERTION(!haveNullGuids,
                   "Someone added a place without adding a GUID!");
    }
#endif

    
    nsRefPtr<PlacesEvent> closeListener =
      new PlacesEvent(TOPIC_PLACES_CONNECTION_CLOSED);
    (void)mDBConn->AsyncClose(closeListener);
  }

#ifdef MOZ_XUL
  else if (strcmp(aTopic, TOPIC_AUTOCOMPLETE_FEEDBACK_INCOMING) == 0) {
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
  else if (strcmp(aTopic, TOPIC_PREF_CHANGED) == 0) {
    LoadPrefs();
  }

  else if (strcmp(aTopic, TOPIC_IDLE_DAILY) == 0) {
    
    
    
    NS_ENSURE_TRUE(mDBConn, NS_OK);

    (void)DecayFrecency();
  }

  else if (strcmp(aTopic, NS_PRIVATE_BROWSING_SWITCH_TOPIC) == 0) {
    if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(aData)) {
      mInPrivateBrowsing = PR_TRUE;
    }
    else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(aData)) {
      mInPrivateBrowsing = PR_FALSE;
    }
  }

  else if (strcmp(aTopic, TOPIC_PLACES_INIT_COMPLETE) == 0) {
    nsCOMPtr<nsIObserverService> os =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);
    (void)os->RemoveObserver(this, TOPIC_PLACES_INIT_COMPLETE);

    
    
    (void)FixInvalidFrecencies();
  }

  return NS_OK;
}


nsresult
nsNavHistory::DecayFrecency()
{
  nsresult rv = FixInvalidFrecencies();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  nsCOMPtr<mozIStorageAsyncStatement> decayFrecency;
  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places SET frecency = ROUND(frecency * .975) "
      "WHERE frecency > 0"),
    getter_AddRefs(decayFrecency));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<mozIStorageAsyncStatement> decayAdaptive;
  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_inputhistory SET use_count = use_count * .975"),
    getter_AddRefs(decayAdaptive));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageAsyncStatement> deleteAdaptive;
  rv = mDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_inputhistory WHERE use_count < .01"),
    getter_AddRefs(deleteAdaptive));
  NS_ENSURE_SUCCESS(rv, rv);

  mozIStorageBaseStatement *stmts[] = {
    decayFrecency,
    decayAdaptive,
    deleteAdaptive
  };
  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = mDBConn->ExecuteAsync(stmts, NS_ARRAY_LENGTH(stmts), nsnull,
                             getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}











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

  if ((NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) ||
    (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt)) {
    clause.Condition("EXISTS (SELECT 1 FROM moz_historyvisits "
                              "WHERE place_id = h.id");
    
    if (NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) 
      clause.Condition("visit_date >=").Param(":begin_time");
    
    if (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt)
      clause.Condition("visit_date <=").Param(":end_time");
    clause.Str(" LIMIT 1)");
  }

  
  PRBool hasSearchTerms;
  if (NS_SUCCEEDED(aQuery->GetHasSearchTerms(&hasSearchTerms)) && hasSearchTerms) {
    
    
    clause.Condition("AUTOCOMPLETE_MATCH(").Param(":search_string")
          .Str(", h.url, page_title, tags, ")
          .Str(nsPrintfCString(17, "0, 0, 0, 0, %d, 0)",
                               mozIPlacesAutoComplete::MATCH_ANYWHERE).get());
  }

  
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
    if (aQuery->UriIsPrefix()) {
      clause.Condition("h.url >= ").Param(":uri")
            .Condition("h.url <= ").Param(":uri_upper");
    }
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

  
  const nsTArray<nsString> &tags = aQuery->Tags();
  if (tags.Length() > 0) {
    clause.Condition("h.id");
    if (aQuery->TagsAreNot())
      clause.Str("NOT");
    clause.Str(
      "IN "
        "(SELECT bms.fk "
         "FROM moz_bookmarks bms "
         "JOIN moz_bookmarks tags ON bms.parent = tags.id "
         "WHERE tags.parent =").
           Param(":tags_folder").
           Str("AND tags.title IN (");
    for (PRUint32 i = 0; i < tags.Length(); ++i) {
      nsPrintfCString param(":tag%d_", i);
      clause.Param(param.get());
      if (i < tags.Length() - 1)
        clause.Str(",");
    }
    clause.Str(")");
    if (!aQuery->TagsAreNot())
      clause.Str("GROUP BY bms.fk HAVING count(*) >=").Param(":tag_count");
    clause.Str(")");
  }

  
  const nsTArray<PRUint32>& transitions = aQuery->Transitions();
  for (PRUint32 i = 0; i < transitions.Length(); ++i) {
    nsPrintfCString param(":transition%d_", i);
    clause.Condition("EXISTS (SELECT 1 FROM moz_historyvisits "
                             "WHERE place_id = h.id AND visit_type = "
              ).Param(param.get()).Str(" LIMIT 1)");
  }

  
  
  if (aOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS &&
      aQuery->Folders().Length() == 1) {
    clause.Condition("b.parent =").Param(":parent");
  }

  clause.GetClauseString(*aClause);
  return NS_OK;
}






nsresult
nsNavHistory::BindQueryClauseParameters(mozIStorageStatement* statement,
                                        PRInt32 aQueryIndex,
                                        nsNavHistoryQuery* aQuery, 
                                        nsNavHistoryQueryOptions* aOptions)
{
  nsresult rv;

  PRBool hasIt;
  
  
  
  nsCAutoString qIndex;
  if (aQueryIndex > 0)
    qIndex.AppendInt(aQueryIndex);

  
  if (NS_SUCCEEDED(aQuery->GetHasBeginTime(&hasIt)) && hasIt) {
    PRTime time = NormalizeTime(aQuery->BeginTimeReference(),
                                aQuery->BeginTime());
    rv = statement->BindInt64ByName(
      NS_LITERAL_CSTRING("begin_time") + qIndex, time);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasEndTime(&hasIt)) && hasIt) {
    PRTime time = NormalizeTime(aQuery->EndTimeReference(),
                                aQuery->EndTime());
    rv = statement->BindInt64ByName(
      NS_LITERAL_CSTRING("end_time") + qIndex, time
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasSearchTerms(&hasIt)) && hasIt) {
    rv = statement->BindStringByName(
      NS_LITERAL_CSTRING("search_string") + qIndex,
      aQuery->SearchTerms()
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  PRInt32 visits = aQuery->MinVisits();
  if (visits >= 0) {
    rv = statement->BindInt32ByName(
      NS_LITERAL_CSTRING("min_visits") + qIndex, visits
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  visits = aQuery->MaxVisits();
  if (visits >= 0) {
    rv = statement->BindInt32ByName(
      NS_LITERAL_CSTRING("max_visits") + qIndex, visits
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  if (NS_SUCCEEDED(aQuery->GetHasDomain(&hasIt)) && hasIt) {
    nsString revDomain;
    GetReversedHostname(NS_ConvertUTF8toUTF16(aQuery->Domain()), revDomain);

    if (aQuery->DomainIsHost()) {
      rv = statement->BindStringByName(
        NS_LITERAL_CSTRING("domain_lower") + qIndex, revDomain
      );
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      
      
      
      NS_ASSERTION(revDomain[revDomain.Length() - 1] == '.', "Invalid rev. host");
      rv = statement->BindStringByName(
        NS_LITERAL_CSTRING("domain_lower") + qIndex, revDomain
      );
      NS_ENSURE_SUCCESS(rv, rv);
      revDomain.Truncate(revDomain.Length() - 1);
      revDomain.Append(PRUnichar('/'));
      rv = statement->BindStringByName(
        NS_LITERAL_CSTRING("domain_upper") + qIndex, revDomain
      );
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  if (aQuery->Uri()) {
    rv = URIBinder::Bind(
      statement, NS_LITERAL_CSTRING("uri") + qIndex, aQuery->Uri()
    );
    NS_ENSURE_SUCCESS(rv, rv);
    if (aQuery->UriIsPrefix()) {
      nsCAutoString uriString;
      aQuery->Uri()->GetSpec(uriString);
      uriString.Append(char(0x7F)); 
      rv = URIBinder::Bind(
        statement, NS_LITERAL_CSTRING("uri_upper") + qIndex, uriString
      );
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  if (!aQuery->Annotation().IsEmpty()) {
    rv = statement->BindUTF8StringByName(
      NS_LITERAL_CSTRING("anno") + qIndex, aQuery->Annotation()
    );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  const nsTArray<nsString> &tags = aQuery->Tags();
  if (tags.Length() > 0) {
    for (PRUint32 i = 0; i < tags.Length(); ++i) {
      nsPrintfCString paramName("tag%d_", i);
      NS_ConvertUTF16toUTF8 tag(tags[i]);
      rv = statement->BindUTF8StringByName(paramName + qIndex, tag);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    PRInt64 tagsFolder = GetTagsFolder();
    rv = statement->BindInt64ByName(
      NS_LITERAL_CSTRING("tags_folder") + qIndex, tagsFolder
    );
    NS_ENSURE_SUCCESS(rv, rv);
    if (!aQuery->TagsAreNot()) {
      rv = statement->BindInt32ByName(
        NS_LITERAL_CSTRING("tag_count") + qIndex, tags.Length()
      );
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  const nsTArray<PRUint32>& transitions = aQuery->Transitions();
  if (transitions.Length() > 0) {
    for (PRUint32 i = 0; i < transitions.Length(); ++i) {
      nsPrintfCString paramName("transition%d_", i);
      rv = statement->BindInt64ByName(paramName + qIndex, transitions[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  if (aOptions->ResultType() == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS &&
      aQuery->Folders().Length() == 1) {
    rv = statement->BindInt64ByName(
      NS_LITERAL_CSTRING("parent") + qIndex, aQuery->Folders()[0]
    );
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
    nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
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

  
  nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
  NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

  
  nsTArray<nsTArray<nsString>*> terms;
  ParseSearchTermsFromQueries(aQueries, &terms);

  
  
  nsTArray< nsTArray<PRInt64>* > includeFolders;
  nsTArray< nsTArray<PRInt64>* > excludeFolders;
  for (PRInt32 queryIndex = 0;
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
    
    
    
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetItemsWithAnno);
    rv = stmt->BindUTF8StringByName(
      NS_LITERAL_CSTRING("anno_name"), parentAnnotationToExclude
    );
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore = PR_FALSE;
    while (NS_SUCCEEDED(stmt->ExecuteStep(&hasMore)) && hasMore) {
      PRInt64 folderId = 0;
      rv = stmt->GetInt64(0, &folderId);
      NS_ENSURE_SUCCESS(rv, rv);
      parentFoldersToExclude.AppendElement(folderId);
    }
  }

  PRUint16 resultType = aOptions->ResultType();
  for (PRInt32 nodeIndex = 0; nodeIndex < aSet.Count(); nodeIndex++) {
    
    
    if (!aSet[nodeIndex]->IsURI())
      continue;

    
    
    
    if (resultType == nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS &&
        nodeIndex > 0 && aSet[nodeIndex]->mURI == aSet[nodeIndex-1]->mURI)
      continue;

    PRInt64 parentId = -1;
    if (aSet[nodeIndex]->mItemId != -1) {
      if (aQueryNode && aQueryNode->mItemId == aSet[nodeIndex]->mItemId)
        continue;
      parentId = aSet[nodeIndex]->mFolderId;
    }

    
    
    if (!parentAnnotationToExclude.IsEmpty() &&
        parentFoldersToExclude.Contains(parentId))
      continue;

    
    PRBool appendNode = PR_FALSE;
    for (PRInt32 queryIndex = 0;
         queryIndex < aQueries.Count() && !appendNode; queryIndex++) {

      if (terms[queryIndex]->Length()) {
        
        
        NS_ConvertUTF8toUTF16 nodeTitle(aSet[nodeIndex]->mTitle);
        
        nsCAutoString cNodeURL(aSet[nodeIndex]->mURI);
        NS_ConvertUTF8toUTF16 nodeURL(NS_UnescapeURL(cNodeURL));

        
        
        PRBool matchAll = PR_TRUE;
        for (PRInt32 termIndex = terms[queryIndex]->Length() - 1;
             termIndex >= 0 && matchAll;
             termIndex--) {
          nsString& term = terms[queryIndex]->ElementAt(termIndex);

          
          matchAll = CaseInsensitiveFindInReadable(term, nodeTitle) ||
                     CaseInsensitiveFindInReadable(term, nodeURL) ||
                     CaseInsensitiveFindInReadable(term, aSet[nodeIndex]->mTags);
        }

        
        if (!matchAll)
          continue;
      }

      
      
      
      if (includeFolders[queryIndex]->Length() != 0 &&
          resultType != nsINavHistoryQueryOptions::RESULTS_AS_TAG_CONTENTS) {
        
        
        if (excludeFolders[queryIndex]->Contains(parentId))
          continue;

        if (!includeFolders[queryIndex]->Contains(parentId)) {
          
          
          PRInt64 ancestor = parentId;
          PRBool belongs = PR_FALSE;
          nsTArray<PRInt64> ancestorFolders;

          while (!belongs) {
            
            ancestorFolders.AppendElement(ancestor);

            
            if (NS_FAILED(bookmarks->GetFolderIdForItem(ancestor, &ancestor))) {
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

      
      appendNode = PR_TRUE;
    }

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

void
nsNavHistory::registerEmbedVisit(nsIURI* aURI,
                                 PRInt64 aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  VisitHashKey* visit = mEmbedVisits.PutEntry(aURI);
  if (!visit) {
    NS_WARNING("Unable to register a EMBED visit.");
    return;
  }
  visit->visitTime = aTime;
}

bool
nsNavHistory::hasEmbedVisit(nsIURI* aURI) {
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  return !!mEmbedVisits.GetEntry(aURI);
}

void
nsNavHistory::clearEmbedVisits() {
  NS_ASSERTION(NS_IsMainThread(), "This can only be called on the main thread");

  mEmbedVisits.Clear();
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
                             nsACString& aSource,
                             PRTime* aTime,
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
  NS_ASSERTION(aRow && aOptions && aResult, "Null pointer in RowToResult");
  *aResult = nsnull;

  
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

  
  PRInt64 itemId = aRow->AsInt64(kGetInfoIndex_ItemId);
  PRInt64 parentId = -1;
  if (itemId == 0) {
    
    
    itemId = -1;
  }
  else {
    
    PRInt64 itemParentId = aRow->AsInt64(kGetInfoIndex_ItemParentId);
    if (itemParentId > 0) {
      
      
      parentId = itemParentId;
    }
  }

  if (IsQueryURI(url)) {
    
      
    
    
    
    
    
    if (itemId != -1) {
      nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

      rv = bookmarks->GetItemTitle(itemId, title);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = QueryRowToResult(itemId, url, title, accessCount, time, favicon, aResult);
    NS_ENSURE_STATE(*aResult);
    if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_TAG_QUERY) {
      
      (*aResult)->mDateAdded = aRow->AsInt64(kGetInfoIndex_ItemDateAdded);
      (*aResult)->mLastModified = aRow->AsInt64(kGetInfoIndex_ItemLastModified);
    }
    else if ((*aResult)->IsFolder()) {
      
      
      
      (*aResult)->GetAsContainer()->mOptions = aOptions;
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
      (*aResult)->mFolderId = parentId;
      (*aResult)->mDateAdded = aRow->AsInt64(kGetInfoIndex_ItemDateAdded);
      (*aResult)->mLastModified = aRow->AsInt64(kGetInfoIndex_ItemLastModified);
    }

    nsAutoString tags;
    rv = aRow->GetString(kGetInfoIndex_ItemTags, tags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!tags.IsVoid())
      (*aResult)->mTags.Assign(tags);

    NS_ADDREF(*aResult);
    return NS_OK;
  }
  

  
  PRInt64 session = aRow->AsInt64(kGetInfoIndex_SessionId);

  if (aOptions->ResultType() == nsNavHistoryQueryOptions::RESULTS_AS_VISIT) {
    *aResult = new nsNavHistoryVisitResultNode(url, title, accessCount, time,
                                               favicon, session);
    if (! *aResult)
      return NS_ERROR_OUT_OF_MEMORY;

    nsAutoString tags;
    rv = aRow->GetString(kGetInfoIndex_ItemTags, tags);
    if (!tags.IsVoid())
      (*aResult)->mTags.Assign(tags);

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
  
  
  if (NS_SUCCEEDED(rv)) {
    
    PRInt64 folderId = GetSimpleBookmarksQueryFolder(queries, options);
    if (folderId) {
      nsNavBookmarks *bookmarks = nsNavBookmarks::GetBookmarksService();
      NS_ENSURE_TRUE(bookmarks, NS_ERROR_OUT_OF_MEMORY);

      
      rv = bookmarks->ResultNodeForContainer(folderId, options, aNode);
      
      
      if (NS_SUCCEEDED(rv)) {
        
        (*aNode)->GetAsFolder()->mQueryItemId = itemId;

        
        
        if (!aTitle.IsVoid()) {
          (*aNode)->mTitle = aTitle;
        }
      }
    }
    else {
      
      *aNode = new nsNavHistoryQueryResultNode(aTitle, EmptyCString(), aTime,
                                               queries, options);
      (*aNode)->mItemId = itemId;
      NS_ADDREF(*aNode);
    }
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Generating a generic empty node for a broken query!");
    
    
    
    *aNode = new nsNavHistoryQueryResultNode(aTitle, aFavicon, aURI);
    (*aNode)->mItemId = itemId;
    NS_ADDREF(*aNode);
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
      
      statement = GetStatement(mDBVisitToVisitResult);
      break;

    case nsNavHistoryQueryOptions::RESULTS_AS_URI:
      
    statement = GetStatement(mDBVisitToURLResult);  
      break;

    default:
      
      
      return NS_OK;
  }
  NS_ENSURE_STATE(statement);

  mozStorageStatementScoper scoper(statement);
  nsresult rv = statement->BindInt64ByName(NS_LITERAL_CSTRING("visit_id"),
                                           visitId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = statement->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (! hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid visit");
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(statement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return RowToResult(row, aOptions, aResult);
}

nsresult
nsNavHistory::BookmarkIdToResultNode(PRInt64 aBookmarkId, nsNavHistoryQueryOptions* aOptions,
                                     nsNavHistoryResultNode** aResult)
{
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBBookmarkToUrlResult);
  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("item_id"),
                                      aBookmarkId);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore = PR_FALSE;
  rv = stmt->ExecuteStep(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!hasMore) {
    NS_NOTREACHED("Trying to get a result node for an invalid bookmark identifier");
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<mozIStorageValueArray> row = do_QueryInterface(stmt, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return RowToResult(row, aOptions, aResult);
}

void
nsNavHistory::SendPageChangedNotification(nsIURI* aURI, PRUint32 aWhat,
                                          const nsAString& aValue)
{
  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver, OnPageChanged(aURI, aWhat, aValue));
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
  if (bundle) {
    nsAutoString intString;
    intString.AppendInt(aInt);
    const PRUnichar* strings[1] = { intString.get() };
    nsXPIDLString value;
    nsresult rv = bundle->FormatStringFromName(aName, strings,
                                               1, getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      CopyUTF16toUTF8(value, aResult);
      return;
    }
  }
  CopyUTF16toUTF8(nsDependentString(aName), aResult);
}

void
nsNavHistory::GetStringFromName(const PRUnichar *aName, nsACString& aResult)
{
  nsIStringBundle *bundle = GetBundle();
  if (bundle) {
    nsXPIDLString value;
    nsresult rv = bundle->GetStringFromName(aName, getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      CopyUTF16toUTF8(value, aResult);
      return;
    }
  }
  CopyUTF16toUTF8(nsDependentString(aName), aResult);
}

void
nsNavHistory::GetMonthName(PRInt32 aIndex, nsACString& aResult)
{
  nsIStringBundle *bundle = GetDateFormatBundle();
  if (bundle) {
    nsCString name = nsPrintfCString("month.%d.name", aIndex);
    nsXPIDLString value;
    nsresult rv = bundle->GetStringFromName(NS_ConvertUTF8toUTF16(name).get(),
                                            getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      CopyUTF16toUTF8(value, aResult);
      return;
    }
  }
  aResult = nsPrintfCString("[%d]", aIndex);
}

void
nsNavHistory::GetMonthYear(PRInt32 aMonth, PRInt32 aYear, nsACString& aResult)
{
  nsIStringBundle *bundle = GetBundle();
  if (bundle) {
    nsCAutoString monthName;
    GetMonthName(aMonth, monthName);
    nsAutoString yearString;
    yearString.AppendInt(aYear);
    const PRUnichar* strings[2] = {
      NS_ConvertUTF8toUTF16(monthName).get()
    , yearString.get()
    };
    nsXPIDLString value;
    if (NS_SUCCEEDED(bundle->FormatStringFromName(
          NS_LITERAL_STRING("finduri-MonthYear").get(), strings, 2,
          getter_Copies(value)
        ))) {
      CopyUTF16toUTF8(value, aResult);
      return;
    }
  }
  aResult.AppendLiteral("finduri-MonthYear");
}










nsresult
nsNavHistory::SetPageTitleInternal(nsIURI* aURI, const nsAString& aTitle)
{
  nsresult rv;

  
  
  nsAutoString title;
  {
    DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetURLPageInfo);
    rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hasURL = PR_FALSE;
    rv = stmt->ExecuteStep(&hasURL);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasURL) {
      
      
      if (hasEmbedVisit(aURI)) {
        return NS_OK;
      }
      return NS_ERROR_NOT_AVAILABLE;
    }

    rv = stmt->GetString(kGetInfoIndex_Title, title);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  if ((aTitle.IsVoid() && title.IsVoid()) || aTitle == title)
    return NS_OK;

  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBSetPlaceTitle);
  if (aTitle.IsVoid())
    rv = stmt->BindNullByName(NS_LITERAL_CSTRING("page_title"));
  else {
    rv = stmt->BindStringByName(NS_LITERAL_CSTRING("page_title"),
                                StringHead(aTitle, TITLE_LENGTH_MAX));
  }
  NS_ENSURE_SUCCESS(rv, rv);
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  NOTIFY_OBSERVERS(mCanNotify, mCacheObservers, mObservers,
                   nsINavHistoryObserver, OnTitleChanged(aURI, aTitle));

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

  
  DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(stmt, mDBGetPageVisitStats);
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"), aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool alreadyVisited = PR_FALSE;
  rv = stmt->ExecuteStep(&alreadyVisited);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 placeId = 0;
  PRInt32 typed = 0;
  PRInt32 hidden = 0;

  if (alreadyVisited) {
    
    rv = stmt->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = stmt->GetInt32(2, &typed);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->GetInt32(3, &hidden);
    NS_ENSURE_SUCCESS(rv, rv);

    if (typed == 0 && aTransitionType == TRANSITION_TYPED) {
      typed = 1;
      
      DECLARE_AND_ASSIGN_SCOPED_LAZY_STMT(updateStmt, mDBUpdatePageVisitStats);
      rv = updateStmt->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), placeId);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = updateStmt->BindInt32ByName(NS_LITERAL_CSTRING("hidden"), hidden);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = updateStmt->BindInt32ByName(NS_LITERAL_CSTRING("typed"), typed);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = updateStmt->Execute();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    
    rv = InternalAddNewPage(aURI, aTitle, hidden == 1,
                            aTransitionType == TRANSITION_TYPED, 0,
                            PR_FALSE, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(placeId != 0, "Cannot add a visit to a nonexistent page");

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


namespace {











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
  if (query->Tags().Length() > 0)
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

} 






nsresult
nsNavHistory::UpdateFrecency(PRInt64 aPlaceId)
{
  DECLARE_AND_ASSIGN_LAZY_STMT(updateFrecencyStmt, mDBUpdateFrecency);
  DECLARE_AND_ASSIGN_LAZY_STMT(updateHiddenStmt, mDBUpdateHiddenOnFrecency);

#define ASYNC_BIND(_stmt) \
  PR_BEGIN_MACRO \
    nsCOMPtr<mozIStorageBindingParamsArray> paramsArray; \
    nsresult rv = _stmt->NewBindingParamsArray(getter_AddRefs(paramsArray)); \
    NS_ENSURE_SUCCESS(rv, rv); \
    nsCOMPtr<mozIStorageBindingParams> params; \
    rv = paramsArray->NewBindingParams(getter_AddRefs(params)); \
    NS_ENSURE_SUCCESS(rv, rv); \
    rv = params->BindInt64ByName(NS_LITERAL_CSTRING("page_id"), aPlaceId); \
    NS_ENSURE_SUCCESS(rv, rv); \
    rv = paramsArray->AddParams(params); \
    NS_ENSURE_SUCCESS(rv, rv); \
    rv = _stmt->BindParameters(paramsArray); \
    NS_ENSURE_SUCCESS(rv, rv); \
  PR_END_MACRO

  ASYNC_BIND(updateFrecencyStmt);
  ASYNC_BIND(updateHiddenStmt);

  mozIStorageBaseStatement *stmts[] = {
    updateFrecencyStmt
  , updateHiddenStmt
  };

  nsCOMPtr<AsyncStatementCallbackNotifier> callback =
    new AsyncStatementCallbackNotifier(TOPIC_FRECENCY_UPDATED);
  nsCOMPtr<mozIStoragePendingStatement> ps;
  nsresult rv = mDBConn->ExecuteAsync(stmts, NS_ARRAY_LENGTH(stmts), callback,
                                      getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
#undef ASYNC_BIND
}


nsresult
nsNavHistory::FixInvalidFrecencies()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsCOMPtr<mozIStorageStatement> fixInvalidFrecenciesStmt;
  nsresult rv = mDBConn->CreateStatement(NS_LITERAL_CSTRING(
      "UPDATE moz_places "
      "SET frecency = CALCULATE_FRECENCY(id) "
      "WHERE frecency < 0"),
    getter_AddRefs(fixInvalidFrecenciesStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<AsyncStatementCallbackNotifier> callback =
    new AsyncStatementCallbackNotifier(TOPIC_FRECENCY_UPDATED);
  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = fixInvalidFrecenciesStmt->ExecuteAsync(callback, getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


#ifdef MOZ_XUL

nsresult
nsNavHistory::AutoCompleteFeedback(PRInt32 aIndex,
                                   nsIAutoCompleteController *aController)
{
  
  if (InPrivateBrowsingMode())
    return NS_OK;

  DECLARE_AND_ASSIGN_LAZY_STMT(stmt, mDBFeedbackIncrease);

  nsAutoString input;
  nsresult rv = aController->GetSearchString(input);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("input_text"), input);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString url;
  rv = aController->GetValueAt(aIndex, url);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = URIBinder::Bind(stmt, NS_LITERAL_CSTRING("page_url"),
                       NS_ConvertUTF16toUTF8(url));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<AsyncStatementCallbackNotifier> callback =
    new AsyncStatementCallbackNotifier(TOPIC_AUTOCOMPLETE_FEEDBACK_UPDATED);
  nsCOMPtr<mozIStoragePendingStatement> canceler;
  rv = stmt->ExecuteAsync(callback, getter_AddRefs(canceler));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

#endif


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
      mozilla::services::GetStringBundleService();
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
      mozilla::services::GetStringBundleService();
    NS_ENSURE_TRUE(bundleService, nsnull);
    nsresult rv = bundleService->CreateBundle(
        "chrome://global/locale/dateFormat.properties",
        getter_AddRefs(mDateFormatBundle));
    NS_ENSURE_SUCCESS(rv, nsnull);
  }
  return mDateFormatBundle;
}


nsresult
nsNavHistory::FinalizeStatements() {
  mozIStorageStatement* stmts[] = {
#ifdef MOZ_XUL
    mDBFeedbackIncrease,
#endif
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
    mDBGetItemsWithAnno,
    mDBSetPlaceTitle,
    mDBVisitToURLResult,
    mDBVisitToVisitResult,
    mDBBookmarkToUrlResult,
    mDBVisitsForFrecency,
    mDBUpdateFrecency,
    mDBUpdateHiddenOnFrecency,
    mDBGetPlaceVisitStats,
    mDBPageInfoForFrecency,
    mDBAsyncThreadPageInfoForFrecency,
    mDBAsyncThreadVisitsForFrecency,
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
  NS_ENSURE_ARG(aChannel);
  NS_ENSURE_ARG_POINTER(aWantCharset);
  NS_ENSURE_ARG_POINTER(aClosure);

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
