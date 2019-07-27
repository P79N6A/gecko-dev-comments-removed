



#include "mozilla/ArrayUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "Database.h"

#include "nsINavBookmarksService.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIFile.h"

#include "nsNavHistory.h"
#include "nsPlacesTables.h"
#include "nsPlacesIndexes.h"
#include "nsPlacesTriggers.h"
#include "nsPlacesMacros.h"
#include "SQLFunctions.h"
#include "Helpers.h"

#include "nsAppDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "prsystem.h"
#include "nsPrintfCString.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "prtime.h"

#include "nsXULAppAPI.h"


#define RECENT_BACKUP_TIME_MICROSEC (int64_t)86400 * PR_USEC_PER_SEC // 24H


#define DATABASE_FILENAME NS_LITERAL_STRING("places.sqlite")

#define DATABASE_CORRUPT_FILENAME NS_LITERAL_STRING("places.sqlite.corrupt")


#define PREF_FORCE_DATABASE_REPLACEMENT "places.database.replaceOnStartup"


#define PREF_GROWTH_INCREMENT_KIB "places.database.growthIncrementKiB"




#define DATABASE_MAX_WAL_SIZE_IN_KIBIBYTES 512

#define BYTES_PER_KIBIBYTE 1024


#define DATABASE_BUSY_TIMEOUT_MS 100


#define SYNCGUID_ANNO NS_LITERAL_CSTRING("sync/guid")


#define PLACES_BUNDLE "chrome://places/locale/places.properties"


#define LMANNO_FEEDURI "livemark/feedURI"
#define LMANNO_SITEURI "livemark/siteURI"

using namespace mozilla;

namespace mozilla {
namespace places {

namespace {








bool
hasRecentCorruptDB()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIFile> profDir;
  NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(profDir));
  NS_ENSURE_TRUE(profDir, false);
  nsCOMPtr<nsISimpleEnumerator> entries;
  profDir->GetDirectoryEntries(getter_AddRefs(entries));
  NS_ENSURE_TRUE(entries, false);
  bool hasMore;
  while (NS_SUCCEEDED(entries->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> next;
    entries->GetNext(getter_AddRefs(next));
    NS_ENSURE_TRUE(next, false);
    nsCOMPtr<nsIFile> currFile = do_QueryInterface(next);
    NS_ENSURE_TRUE(currFile, false);

    nsAutoString leafName;
    if (NS_SUCCEEDED(currFile->GetLeafName(leafName)) &&
        leafName.Length() >= DATABASE_CORRUPT_FILENAME.Length() &&
        leafName.Find(".corrupt", DATABASE_FILENAME.Length()) != -1) {
      PRTime lastMod = 0;
      currFile->GetLastModifiedTime(&lastMod);
      NS_ENSURE_TRUE(lastMod > 0, false);
      return (PR_Now() - lastMod) > RECENT_BACKUP_TIME_MICROSEC;
    }
  }
  return false;
}









nsresult
updateSQLiteStatistics(mozIStorageConnection* aDBConn)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<mozIStorageAsyncStatement> analyzePlacesStmt;
  aDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "ANALYZE moz_places"
  ), getter_AddRefs(analyzePlacesStmt));
  NS_ENSURE_STATE(analyzePlacesStmt);
  nsCOMPtr<mozIStorageAsyncStatement> analyzeBookmarksStmt;
  aDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "ANALYZE moz_bookmarks"
  ), getter_AddRefs(analyzeBookmarksStmt));
  NS_ENSURE_STATE(analyzeBookmarksStmt);
  nsCOMPtr<mozIStorageAsyncStatement> analyzeVisitsStmt;
  aDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "ANALYZE moz_historyvisits"
  ), getter_AddRefs(analyzeVisitsStmt));
  NS_ENSURE_STATE(analyzeVisitsStmt);
  nsCOMPtr<mozIStorageAsyncStatement> analyzeInputStmt;
  aDBConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "ANALYZE moz_inputhistory"
  ), getter_AddRefs(analyzeInputStmt));
  NS_ENSURE_STATE(analyzeInputStmt);

  mozIStorageBaseStatement *stmts[] = {
    analyzePlacesStmt,
    analyzeBookmarksStmt,
    analyzeVisitsStmt,
    analyzeInputStmt
  };

  nsCOMPtr<mozIStoragePendingStatement> ps;
  (void)aDBConn->ExecuteAsync(stmts, ArrayLength(stmts), nullptr,
                              getter_AddRefs(ps));
  return NS_OK;
}












enum JournalMode
SetJournalMode(nsCOMPtr<mozIStorageConnection>& aDBConn,
                             enum JournalMode aJournalMode)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsAutoCString journalMode;
  switch (aJournalMode) {
    default:
      MOZ_ASSERT(false, "Trying to set an unknown journal mode.");
      
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
  nsAutoCString query(MOZ_STORAGE_UNIQUIFY_QUERY_STR
		      "PRAGMA journal_mode = ");
  query.Append(journalMode);
  aDBConn->CreateStatement(query, getter_AddRefs(statement));
  NS_ENSURE_TRUE(statement, JOURNAL_DELETE);

  bool hasResult = false;
  if (NS_SUCCEEDED(statement->ExecuteStep(&hasResult)) && hasResult &&
      NS_SUCCEEDED(statement->GetUTF8String(0, journalMode))) {
    if (journalMode.EqualsLiteral("delete")) {
      return JOURNAL_DELETE;
    }
    if (journalMode.EqualsLiteral("truncate")) {
      return JOURNAL_TRUNCATE;
    }
    if (journalMode.EqualsLiteral("memory")) {
      return JOURNAL_MEMORY;
    }
    if (journalMode.EqualsLiteral("wal")) {
      return JOURNAL_WAL;
    }
    
    MOZ_ASSERT(true);
  }

  return JOURNAL_DELETE;
}

nsresult
CreateRoot(nsCOMPtr<mozIStorageConnection>& aDBConn,
           const nsCString& aRootName, const nsCString& aGuid,
           const nsXPIDLString& titleString)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  static int32_t itemPosition = 0;

  
  
  static PRTime timestamp = 0;
  if (!timestamp)
    timestamp = RoundedPRNow();

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_bookmarks "
      "(type, position, title, dateAdded, lastModified, guid, parent) "
    "VALUES (:item_type, :item_position, :item_title,"
            ":date_added, :last_modified, :guid,"
            "IFNULL((SELECT id FROM moz_bookmarks WHERE parent = 0), 0))"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) return rv;

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_type"),
                             nsINavBookmarksService::TYPE_FOLDER);
  if (NS_FAILED(rv)) return rv;
  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("item_position"), itemPosition);
  if (NS_FAILED(rv)) return rv;
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("item_title"),
                                  NS_ConvertUTF16toUTF8(titleString));
  if (NS_FAILED(rv)) return rv;
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("date_added"), timestamp);
  if (NS_FAILED(rv)) return rv;
  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("last_modified"), timestamp);
  if (NS_FAILED(rv)) return rv;
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"), aGuid);
  if (NS_FAILED(rv)) return rv;
  rv = stmt->Execute();
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<mozIStorageStatement> newRootStmt;
  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_bookmarks_roots (root_name, folder_id) "
    "VALUES (:root_name, (SELECT id from moz_bookmarks WHERE guid = :guid))"
  ), getter_AddRefs(newRootStmt));
  if (NS_FAILED(rv)) return rv;

  rv = newRootStmt->BindUTF8StringByName(NS_LITERAL_CSTRING("root_name"),
                                         aRootName);
  if (NS_FAILED(rv)) return rv;
  rv = newRootStmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"),
                                         aGuid);
  if (NS_FAILED(rv)) return rv;
  rv = newRootStmt->Execute();
  if (NS_FAILED(rv)) return rv;

  
  
  if (!aRootName.EqualsLiteral("places"))
    ++itemPosition;

  return NS_OK;
}


} 




class DatabaseShutdown final:
    public nsIAsyncShutdownBlocker,
    public nsIAsyncShutdownCompletionCallback,
    public mozIStorageCompletionCallback
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIASYNCSHUTDOWNBLOCKER
  NS_DECL_NSIASYNCSHUTDOWNCOMPLETIONCALLBACK
  NS_DECL_MOZISTORAGECOMPLETIONCALLBACK

  explicit DatabaseShutdown(Database* aDatabase);

  already_AddRefed<nsIAsyncShutdownClient> GetClient();

  



  bool IsStarted() const {
    return mIsStarted;
  }

private:
  nsCOMPtr<nsIAsyncShutdownBarrier> mBarrier;
  nsCOMPtr<nsIAsyncShutdownClient> mParentClient;

  
  
  
  nsRefPtr<Database> mDatabase;

  
  
  enum State {
    NOT_STARTED,

    
    
    RECEIVED_BLOCK_SHUTDOWN,
    
    CALLED_WAIT_CLIENTS,

    
    
    RECEIVED_DONE,
    
    NOTIFIED_OBSERVERS_PLACES_WILL_CLOSE_CONNECTION,
    
    CALLED_STORAGESHUTDOWN,

    
    
    RECEIVED_STORAGESHUTDOWN_COMPLETE,
    
    NOTIFIED_OBSERVERS_PLACES_CONNECTION_CLOSED,
  };
  State mState;
  bool mIsStarted;

  
  
  uint16_t mCounter;
  static uint16_t sCounter;

  ~DatabaseShutdown() {}
};
uint16_t DatabaseShutdown::sCounter = 0;

DatabaseShutdown::DatabaseShutdown(Database* aDatabase)
  : mDatabase(aDatabase)
  , mState(NOT_STARTED)
  , mIsStarted(false)
  , mCounter(sCounter++)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIAsyncShutdownService> asyncShutdownSvc = services::GetAsyncShutdown();
  MOZ_ASSERT(asyncShutdownSvc);

  if (asyncShutdownSvc) {
    DebugOnly<nsresult> rv = asyncShutdownSvc->MakeBarrier(
      NS_LITERAL_STRING("Places Database shutdown"),
      getter_AddRefs(mBarrier)
    );
    MOZ_ASSERT(NS_SUCCEEDED(rv));
  }
}

already_AddRefed<nsIAsyncShutdownClient>
DatabaseShutdown::GetClient()
{
  nsCOMPtr<nsIAsyncShutdownClient> client;
  if (mBarrier) {
    DebugOnly<nsresult> rv = mBarrier->GetClient(getter_AddRefs(client));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
  }
  return client.forget();
}


NS_IMETHODIMP
DatabaseShutdown::GetName(nsAString& aName)
{
  if (mCounter > 0) {
    
    
    nsPrintfCString name("Places DatabaseShutdown: Blocking profile-before-change (%x)", this);
    aName = NS_ConvertUTF8toUTF16(name);
  } else {
    aName = NS_LITERAL_STRING("Places DatabaseShutdown: Blocking profile-before-change");
  }
  return NS_OK;
}


NS_IMETHODIMP DatabaseShutdown::GetState(nsIPropertyBag** aState)
{
  nsresult rv;
  nsCOMPtr<nsIWritablePropertyBag2> bag =
    do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) return rv;

  
  nsCOMPtr<nsIWritableVariant> progress =
    do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) return rv;

  rv = progress->SetAsUint8(mState);
  if (NS_WARN_IF(NS_FAILED(rv))) return rv;

  rv = bag->SetPropertyAsInterface(NS_LITERAL_STRING("progress"), progress);
  if (NS_WARN_IF(NS_FAILED(rv))) return rv;

  
  if (!mBarrier) {
    return NS_OK;
  }
  nsCOMPtr<nsIPropertyBag> barrierState;
  rv = mBarrier->GetState(getter_AddRefs(barrierState));
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  nsCOMPtr<nsIWritableVariant> barrier =
    do_CreateInstance(NS_VARIANT_CONTRACTID, &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) return rv;

  rv = barrier->SetAsInterface(NS_GET_IID(nsIPropertyBag), barrierState);
  if (NS_WARN_IF(NS_FAILED(rv))) return rv;

  rv = bag->SetPropertyAsInterface(NS_LITERAL_STRING("Barrier"), barrier);
  if (NS_WARN_IF(NS_FAILED(rv))) return rv;

  return NS_OK;
}









NS_IMETHODIMP
DatabaseShutdown::BlockShutdown(nsIAsyncShutdownClient* aParentClient)
{
  mParentClient = aParentClient;
  mState = RECEIVED_BLOCK_SHUTDOWN;
  mIsStarted = true;

  if (NS_WARN_IF(!mBarrier)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  DebugOnly<nsresult> rv = mBarrier->Wait(this);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  mState = CALLED_WAIT_CLIENTS;
  return NS_OK;
}







NS_IMETHODIMP
DatabaseShutdown::Done()
{
  mState = RECEIVED_DONE;

  
  nsCOMPtr<nsIObserverService> os = services::GetObserverService();
  MOZ_ASSERT(os);
  if (os) {
    (void)os->NotifyObservers(nullptr, TOPIC_PLACES_WILL_CLOSE_CONNECTION, nullptr);
  }
  mState = NOTIFIED_OBSERVERS_PLACES_WILL_CLOSE_CONNECTION;

  
  
  
  MOZ_ASSERT(Database::gDatabase == nullptr || Database::gDatabase == mDatabase);
  Database::gDatabase = nullptr;

  mDatabase->Shutdown();
  mState = CALLED_STORAGESHUTDOWN;
  return NS_OK;
}









NS_IMETHODIMP
DatabaseShutdown::Complete(nsresult, nsISupports*)
{
  MOZ_ASSERT(NS_IsMainThread());
  mState = RECEIVED_STORAGESHUTDOWN_COMPLETE;
  mDatabase = nullptr;

  nsresult rv;
  if (mParentClient) {
    
    rv = mParentClient->RemoveBlocker(this);
    if (NS_WARN_IF(NS_FAILED(rv))) return rv;
  }

  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  MOZ_ASSERT(os);
  if (os) {
    rv = os->NotifyObservers(nullptr,
                             TOPIC_PLACES_CONNECTION_CLOSED,
                             nullptr);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
  }
  mState = NOTIFIED_OBSERVERS_PLACES_CONNECTION_CLOSED;

  if (NS_WARN_IF(!mBarrier)) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  nsCOMPtr<nsIAsyncShutdownBarrier> barrier = mBarrier.forget();
  nsCOMPtr<nsIAsyncShutdownClient> parentClient = mParentClient.forget();
  nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
  MOZ_ASSERT(mainThread);

  NS_ProxyRelease(mainThread, barrier);
  NS_ProxyRelease(mainThread, parentClient);

  return NS_OK;
}

NS_IMPL_ISUPPORTS(
  DatabaseShutdown
, nsIAsyncShutdownBlocker
, nsIAsyncShutdownCompletionCallback
, mozIStorageCompletionCallback
)




PLACES_FACTORY_SINGLETON_IMPLEMENTATION(Database, gDatabase)

NS_IMPL_ISUPPORTS(Database
, nsIObserver
, nsISupportsWeakReference
)

Database::Database()
  : mMainThreadStatements(mMainConn)
  , mMainThreadAsyncStatements(mMainConn)
  , mAsyncThreadStatements(mMainConn)
  , mDBPageSize(0)
  , mDatabaseStatus(nsINavHistoryService::DATABASE_STATUS_OK)
  , mClosed(false)
  , mConnectionShutdown(new DatabaseShutdown(this))
{
  MOZ_ASSERT(XRE_GetProcessType() != GeckoProcessType_Content,
             "Cannot instantiate Places in the content process");
  
  MOZ_ASSERT(!gDatabase);
  gDatabase = this;

  
  nsCOMPtr<nsIAsyncShutdownClient> shutdownPhase = GetShutdownPhase();
  MOZ_ASSERT(shutdownPhase);

  if (shutdownPhase) {
    DebugOnly<nsresult> rv = shutdownPhase->AddBlocker(
      static_cast<nsIAsyncShutdownBlocker*>(mConnectionShutdown.get()),
      NS_LITERAL_STRING(__FILE__),
      __LINE__,
      NS_LITERAL_STRING(""));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
  }
}

already_AddRefed<nsIAsyncShutdownClient>
Database::GetShutdownPhase()
{
  nsCOMPtr<nsIAsyncShutdownService> asyncShutdownSvc = services::GetAsyncShutdown();
  MOZ_ASSERT(asyncShutdownSvc);
  if (NS_WARN_IF(!asyncShutdownSvc)) {
    return nullptr;
  }

  nsCOMPtr<nsIAsyncShutdownClient> shutdownPhase;
  DebugOnly<nsresult> rv = asyncShutdownSvc->
    GetProfileBeforeChange(getter_AddRefs(shutdownPhase));
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  return shutdownPhase.forget();
}

Database::~Database()
{
}

bool
Database::IsShutdownStarted() const
{
  if (!mConnectionShutdown) {
    
    return true;
  }
  return mConnectionShutdown->IsStarted();
}

already_AddRefed<mozIStorageAsyncStatement>
Database::GetAsyncStatement(const nsACString& aQuery) const
{
  if (IsShutdownStarted()) {
    return nullptr;
  }
  MOZ_ASSERT(NS_IsMainThread());
  return mMainThreadAsyncStatements.GetCachedStatement(aQuery);
}

already_AddRefed<mozIStorageStatement>
Database::GetStatement(const nsACString& aQuery) const
{
  if (IsShutdownStarted()) {
    return nullptr;
  }
  if (NS_IsMainThread()) {
    return mMainThreadStatements.GetCachedStatement(aQuery);
  }
  return mAsyncThreadStatements.GetCachedStatement(aQuery);
}

already_AddRefed<nsIAsyncShutdownClient>
Database::GetConnectionShutdown()
{
  MOZ_ASSERT(mConnectionShutdown);

  return mConnectionShutdown->GetClient();
}

nsresult
Database::Init()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<mozIStorageService> storage =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_STATE(storage);

  
  bool databaseCreated = false;
  nsresult rv = InitDatabaseFile(storage, &databaseCreated);
  if (NS_SUCCEEDED(rv) && databaseCreated) {
    mDatabaseStatus = nsINavHistoryService::DATABASE_STATUS_CREATE;
  }
  else if (rv == NS_ERROR_FILE_CORRUPTED) {
    
    mDatabaseStatus = nsINavHistoryService::DATABASE_STATUS_CORRUPT;
    rv = BackupAndReplaceDatabaseFile(storage);
    
  }

  
  
  if (NS_FAILED(rv)) {
    nsRefPtr<PlacesEvent> lockedEvent = new PlacesEvent(TOPIC_DATABASE_LOCKED);
    (void)NS_DispatchToMainThread(lockedEvent);
    return rv;
  }

  
  
  bool databaseMigrated = false;
  rv = InitSchema(&databaseMigrated);
  if (NS_FAILED(rv)) {
    mDatabaseStatus = nsINavHistoryService::DATABASE_STATUS_CORRUPT;
    rv = BackupAndReplaceDatabaseFile(storage);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = InitSchema(&databaseMigrated);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (databaseMigrated) {
    mDatabaseStatus = nsINavHistoryService::DATABASE_STATUS_UPGRADED;
  }

  if (mDatabaseStatus != nsINavHistoryService::DATABASE_STATUS_OK) {
    rv = updateSQLiteStatistics(MainConn());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  

  rv = InitTempTriggers();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  nsRefPtr<PlacesEvent> completeEvent =
    new PlacesEvent(TOPIC_PLACES_INIT_COMPLETE);
  rv = NS_DispatchToMainThread(completeEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  if (os) {
    (void)os->AddObserver(this, TOPIC_PROFILE_CHANGE_TEARDOWN, true);
  }

  return NS_OK;
}

nsresult
Database::InitDatabaseFile(nsCOMPtr<mozIStorageService>& aStorage,
                           bool* aNewDatabaseCreated)
{
  MOZ_ASSERT(NS_IsMainThread());
  *aNewDatabaseCreated = false;

  nsCOMPtr<nsIFile> databaseFile;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(databaseFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = databaseFile->Append(DATABASE_FILENAME);
  NS_ENSURE_SUCCESS(rv, rv);

  bool databaseFileExists = false;
  rv = databaseFile->Exists(&databaseFileExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (databaseFileExists &&
      Preferences::GetBool(PREF_FORCE_DATABASE_REPLACEMENT, false)) {
    
    
    
    (void)Preferences::ClearUser(PREF_FORCE_DATABASE_REPLACEMENT);

    return NS_ERROR_FILE_CORRUPTED;
  }

  
  
  
  rv = aStorage->OpenUnsharedDatabase(databaseFile, getter_AddRefs(mMainConn));
  NS_ENSURE_SUCCESS(rv, rv);

  *aNewDatabaseCreated = !databaseFileExists;
  return NS_OK;
}

nsresult
Database::BackupAndReplaceDatabaseFile(nsCOMPtr<mozIStorageService>& aStorage)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsCOMPtr<nsIFile> profDir;
  nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                                       getter_AddRefs(profDir));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFile> databaseFile;
  rv = profDir->Clone(getter_AddRefs(databaseFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = databaseFile->Append(DATABASE_FILENAME);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  if (!hasRecentCorruptDB()) {
    nsCOMPtr<nsIFile> backup;
    (void)aStorage->BackupDatabaseFile(databaseFile, DATABASE_CORRUPT_FILENAME,
                                       profDir, getter_AddRefs(backup));
  }

  
  if (mMainConn) {
    
    
    rv = mMainConn->Close();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = databaseFile->Remove(false);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  rv = aStorage->OpenUnsharedDatabase(databaseFile, getter_AddRefs(mMainConn));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::InitSchema(bool* aDatabaseMigrated)
{
  MOZ_ASSERT(NS_IsMainThread());
  *aDatabaseMigrated = false;

  
  
  

  {
    
    
    nsCOMPtr<mozIStorageStatement> statement;
    nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      MOZ_STORAGE_UNIQUIFY_QUERY_STR "PRAGMA page_size"
    ), getter_AddRefs(statement));
    NS_ENSURE_SUCCESS(rv, rv);
    bool hasResult = false;
    rv = statement->ExecuteStep(&hasResult);
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && hasResult, NS_ERROR_FAILURE);
    rv = statement->GetInt32(0, &mDBPageSize);
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && mDBPageSize > 0, NS_ERROR_UNEXPECTED);
  }

  
  nsresult rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      MOZ_STORAGE_UNIQUIFY_QUERY_STR "PRAGMA temp_store = MEMORY"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (JOURNAL_WAL == SetJournalMode(mMainConn, JOURNAL_WAL)) {
    
    
    
    
    int32_t checkpointPages =
      static_cast<int32_t>(DATABASE_MAX_WAL_SIZE_IN_KIBIBYTES * 1024 / mDBPageSize);
    nsAutoCString checkpointPragma("PRAGMA wal_autocheckpoint = ");
    checkpointPragma.AppendInt(checkpointPages);
    rv = mMainConn->ExecuteSimpleSQL(checkpointPragma);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    
    
    
    (void)SetJournalMode(mMainConn, JOURNAL_TRUNCATE);

    
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "PRAGMA synchronous = FULL"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  
  nsAutoCString journalSizePragma("PRAGMA journal_size_limit = ");
  journalSizePragma.AppendInt(DATABASE_MAX_WAL_SIZE_IN_KIBIBYTES * 3);
  (void)mMainConn->ExecuteSimpleSQL(journalSizePragma);

  
  
  int32_t growthIncrementKiB =
    Preferences::GetInt(PREF_GROWTH_INCREMENT_KIB, 10 * BYTES_PER_KIBIBYTE);
  if (growthIncrementKiB > 0) {
    (void)mMainConn->SetGrowthIncrement(growthIncrementKiB * BYTES_PER_KIBIBYTE, EmptyCString());
  }

  nsAutoCString busyTimeoutPragma("PRAGMA busy_timeout = ");
  busyTimeoutPragma.AppendInt(DATABASE_BUSY_TIMEOUT_MS);
  (void)mMainConn->ExecuteSimpleSQL(busyTimeoutPragma);

  
  rv = InitFunctions();
  NS_ENSURE_SUCCESS(rv, rv);

  
  int32_t currentSchemaVersion;
  rv = mMainConn->GetSchemaVersion(&currentSchemaVersion);
  NS_ENSURE_SUCCESS(rv, rv);
  bool databaseInitialized = currentSchemaVersion > 0;

  if (databaseInitialized && currentSchemaVersion == DATABASE_SCHEMA_VERSION) {
    
    return NS_OK;
  }

  
  
  mozStorageTransaction transaction(mMainConn, false);

  if (databaseInitialized) {
    
    
    
    
    
    
    
    
    
    

    if (currentSchemaVersion < DATABASE_SCHEMA_VERSION) {
      *aDatabaseMigrated = true;

      if (currentSchemaVersion < 11) {
        
        
        return NS_ERROR_FILE_CORRUPTED;
      }

      

      

      if (currentSchemaVersion < 13) {
        rv = MigrateV13Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 14) {
        rv = MigrateV14Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 15) {
        rv = MigrateV15Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 16) {
        rv = MigrateV16Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 17) {
        rv = MigrateV17Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 18) {
        rv = MigrateV18Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 19) {
        rv = MigrateV19Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 20) {
        rv = MigrateV20Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 21) {
        rv = MigrateV21Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 22) {
        rv = MigrateV22Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 23) {
        rv = MigrateV23Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 24) {
        rv = MigrateV24Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 25) {
        rv = MigrateV25Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 26) {
        rv = MigrateV26Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 27) {
        rv = MigrateV27Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 28) {
        rv = MigrateV28Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      

      rv = UpdateBookmarkRootTitles();
      
      
      MOZ_ASSERT(NS_SUCCEEDED(rv));
    }
  }
  else {
    

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_PLACES);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_URL);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_FAVICON);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_REVHOST);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_VISITCOUNT);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_FRECENCY);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_LASTVISITDATE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_GUID);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_HISTORYVISITS);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_HISTORYVISITS_PLACEDATE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_HISTORYVISITS_FROMVISIT);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_HISTORYVISITS_VISITDATE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_INPUTHISTORY);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_HOSTS);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_BOOKMARKS);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PLACETYPE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PARENTPOSITION);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PLACELASTMODIFIED);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_GUID);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_BOOKMARKS_ROOTS);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_KEYWORDS);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_KEYWORDS_PLACEPOSTDATA);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_FAVICONS);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_ANNO_ATTRIBUTES);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_ANNOS);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_ANNOS_PLACEATTRIBUTE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_ITEMS_ANNOS);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_ITEMSANNOS_PLACEATTRIBUTE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = CreateBookmarkRoots();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = mMainConn->SetSchemaVersion(DATABASE_SCHEMA_VERSION);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = transaction.Commit();
  NS_ENSURE_SUCCESS(rv, rv);

  ForceWALCheckpoint();

  
  
  
  

  return NS_OK;
}

nsresult
Database::CreateBookmarkRoots()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIStringBundleService> bundleService =
    services::GetStringBundleService();
  NS_ENSURE_STATE(bundleService);
  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = bundleService->CreateBundle(PLACES_BUNDLE, getter_AddRefs(bundle));
  if (NS_FAILED(rv)) return rv;

  nsXPIDLString rootTitle;
  
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("places"),
                  NS_LITERAL_CSTRING("root________"), rootTitle);
  if (NS_FAILED(rv)) return rv;

  
  rv = bundle->GetStringFromName(MOZ_UTF16("BookmarksMenuFolderTitle"),
                                 getter_Copies(rootTitle));
  if (NS_FAILED(rv)) return rv;
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("menu"),
                  NS_LITERAL_CSTRING("menu________"), rootTitle);
  if (NS_FAILED(rv)) return rv;

  rv = bundle->GetStringFromName(MOZ_UTF16("BookmarksToolbarFolderTitle"),
                                 getter_Copies(rootTitle));
  if (NS_FAILED(rv)) return rv;
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("toolbar"),
                  NS_LITERAL_CSTRING("toolbar_____"), rootTitle);
  if (NS_FAILED(rv)) return rv;

  rv = bundle->GetStringFromName(MOZ_UTF16("TagsFolderTitle"),
                                 getter_Copies(rootTitle));
  if (NS_FAILED(rv)) return rv;
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("tags"),
                  NS_LITERAL_CSTRING("tags________"), rootTitle);
  if (NS_FAILED(rv)) return rv;

  rv = bundle->GetStringFromName(MOZ_UTF16("UnsortedBookmarksFolderTitle"),
                                 getter_Copies(rootTitle));
  if (NS_FAILED(rv)) return rv;
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("unfiled"),
                  NS_LITERAL_CSTRING("unfiled_____"), rootTitle);
  if (NS_FAILED(rv)) return rv;

#if DEBUG
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT count(*), sum(position) FROM moz_bookmarks"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) return rv;

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_FAILED(rv)) return rv;
  MOZ_ASSERT(hasResult);
  int32_t bookmarkCount = stmt->AsInt32(0);
  int32_t positionSum = stmt->AsInt32(1);
  MOZ_ASSERT(bookmarkCount == 5 && positionSum == 6);
#endif

  return NS_OK;
}

nsresult
Database::InitFunctions()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv = GetUnreversedHostFunction::create(mMainConn);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = MatchAutoCompleteFunction::create(mMainConn);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = CalculateFrecencyFunction::create(mMainConn);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = GenerateGUIDFunction::create(mMainConn);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = FixupURLFunction::create(mMainConn);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = FrecencyNotificationFunction::create(mMainConn);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::InitTempTriggers()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv = mMainConn->ExecuteSimpleSQL(CREATE_HISTORYVISITS_AFTERINSERT_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(CREATE_HISTORYVISITS_AFTERDELETE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMainConn->ExecuteSimpleSQL(CREATE_PLACES_AFTERINSERT_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(CREATE_PLACES_AFTERDELETE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(CREATE_PLACES_AFTERUPDATE_FRECENCY_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(CREATE_PLACES_AFTERUPDATE_TYPED_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainConn->ExecuteSimpleSQL(CREATE_BOOKMARKS_FOREIGNCOUNT_AFTERDELETE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(CREATE_BOOKMARKS_FOREIGNCOUNT_AFTERINSERT_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(CREATE_BOOKMARKS_FOREIGNCOUNT_AFTERUPDATE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainConn->ExecuteSimpleSQL(CREATE_KEYWORDS_FOREIGNCOUNT_AFTERDELETE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(CREATE_KEYWORDS_FOREIGNCOUNT_AFTERINSERT_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(CREATE_KEYWORDS_FOREIGNCOUNT_AFTERUPDATE_TRIGGER);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::UpdateBookmarkRootTitles()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsCOMPtr<nsIStringBundleService> bundleService =
    services::GetStringBundleService();
  NS_ENSURE_STATE(bundleService);

  nsCOMPtr<nsIStringBundle> bundle;
  nsresult rv = bundleService->CreateBundle(PLACES_BUNDLE, getter_AddRefs(bundle));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<mozIStorageAsyncStatement> stmt;
  rv = mMainConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET title = :new_title WHERE guid = :guid"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  rv = stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  if (NS_FAILED(rv)) return rv;

  const char *rootGuids[] = { "menu________"
                            , "toolbar_____"
                            , "tags________"
                            , "unfiled_____"
                            };
  const char *titleStringIDs[] = { "BookmarksMenuFolderTitle"
                                 , "BookmarksToolbarFolderTitle"
                                 , "TagsFolderTitle"
                                 , "UnsortedBookmarksFolderTitle"
                                 };

  for (uint32_t i = 0; i < ArrayLength(rootGuids); ++i) {
    nsXPIDLString title;
    rv = bundle->GetStringFromName(NS_ConvertASCIItoUTF16(titleStringIDs[i]).get(),
                                   getter_Copies(title));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<mozIStorageBindingParams> params;
    rv = paramsArray->NewBindingParams(getter_AddRefs(params));
    if (NS_FAILED(rv)) return rv;
    rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"),
                                      nsDependentCString(rootGuids[i]));
    if (NS_FAILED(rv)) return rv;
    rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("new_title"),
                                      NS_ConvertUTF16toUTF8(title));
    if (NS_FAILED(rv)) return rv;
    rv = paramsArray->AddParams(params);
    if (NS_FAILED(rv)) return rv;
  }

  rv = stmt->BindParameters(paramsArray);
  if (NS_FAILED(rv)) return rv;
  nsCOMPtr<mozIStoragePendingStatement> pendingStmt;
  rv = stmt->ExecuteAsync(nullptr, getter_AddRefs(pendingStmt));
  if (NS_FAILED(rv)) return rv;

  return NS_OK;
}

nsresult
Database::MigrateV13Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<mozIStorageAsyncStatement> deleteDynContainersStmt;
  nsresult rv = mMainConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
      "DELETE FROM moz_bookmarks WHERE type = :item_type"),
    getter_AddRefs(deleteDynContainersStmt));
  rv = deleteDynContainersStmt->BindInt32ByName(
    NS_LITERAL_CSTRING("item_type"),
    nsINavBookmarksService::TYPE_DYNAMIC_CONTAINER
  );
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = deleteDynContainersStmt->ExecuteAsync(nullptr, getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV14Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  nsCOMPtr<mozIStorageStatement> hasGuidStatement;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT guid FROM moz_favicons"),
    getter_AddRefs(hasGuidStatement));

  if (NS_FAILED(rv)) {
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_favicons "
      "ADD COLUMN guid TEXT"
    ));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_FAVICONS_GUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_favicons "
    "SET guid = GENERATE_GUID() "
    "WHERE guid ISNULL "
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV15Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  nsresult rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP TRIGGER IF EXISTS moz_bookmarks_beforedelete_v1_trigger"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_keywords "
    "WHERE NOT EXISTS ( "
      "SELECT id "
      "FROM moz_bookmarks "
      "WHERE keyword_id = moz_keywords.id "
    ")"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV16Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  nsresult rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_favicons "
    "SET guid = GENERATE_GUID() "
    "WHERE guid ISNULL "
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV17Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  bool tableExists = false;

  nsresult rv = mMainConn->TableExists(NS_LITERAL_CSTRING("moz_hosts"), &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!tableExists) {
    
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP INDEX IF EXISTS moz_hostnames_frecencyindex"
    ));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TABLE IF EXISTS moz_hostnames"
    ));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_HOSTS);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<mozIStorageAsyncStatement> fillHostsStmt;
  rv = mMainConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "INSERT OR IGNORE INTO moz_hosts (host, frecency) "
        "SELECT fixup_url(get_unreversed_host(h.rev_host)) AS host, "
               "(SELECT MAX(frecency) FROM moz_places "
                "WHERE rev_host = h.rev_host "
                   "OR rev_host = h.rev_host || 'www.' "
               ") AS frecency "
        "FROM moz_places h "
        "WHERE LENGTH(h.rev_host) > 1 "
        "GROUP BY h.rev_host"
  ), getter_AddRefs(fillHostsStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = fillHostsStmt->ExecuteAsync(nullptr, getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV18Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT typed FROM moz_hosts"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) {
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_hosts ADD COLUMN typed NOT NULL DEFAULT 0"
    ));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DROP INDEX IF EXISTS moz_hosts_frecencyhostindex"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageAsyncStatement> updateTypedStmt;
  rv = mMainConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_hosts SET typed = 1 WHERE host IN ( "
      "SELECT fixup_url(get_unreversed_host(rev_host)) "
      "FROM moz_places WHERE typed = 1 "
    ") "
  ), getter_AddRefs(updateTypedStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = updateTypedStmt->ExecuteAsync(nullptr, getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV19Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  

  
  nsCOMPtr<mozIStorageStatement> deleteLivemarksChildrenStmt;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_bookmarks WHERE parent IN("
      "SELECT b.id FROM moz_bookmarks b "
      "JOIN moz_items_annos a ON a.item_id = b.id "
      "JOIN moz_anno_attributes n ON n.id = a.anno_attribute_id "
      "WHERE b.type = :item_type AND n.name = :anno_name "
    ")"
  ), getter_AddRefs(deleteLivemarksChildrenStmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksChildrenStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_name"), NS_LITERAL_CSTRING(LMANNO_FEEDURI)
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksChildrenStmt->BindInt32ByName(
    NS_LITERAL_CSTRING("item_type"), nsINavBookmarksService::TYPE_FOLDER
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksChildrenStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  (void)Preferences::ClearUser("browser.bookmarks.livemark_refresh_seconds");
  (void)Preferences::ClearUser("browser.bookmarks.livemark_refresh_limit_count");
  (void)Preferences::ClearUser("browser.bookmarks.livemark_refresh_delay_time");

  
  nsCOMPtr<mozIStorageStatement> deleteLivemarksAnnosStmt;
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_items_annos WHERE anno_attribute_id IN("
      "SELECT id FROM moz_anno_attributes "
      "WHERE name IN (:anno_loading, :anno_loadfailed, :anno_expiration) "
    ")"
  ), getter_AddRefs(deleteLivemarksAnnosStmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksAnnosStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_loading"), NS_LITERAL_CSTRING("livemark/loading")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksAnnosStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_loadfailed"), NS_LITERAL_CSTRING("livemark/loadfailed")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksAnnosStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_expiration"), NS_LITERAL_CSTRING("livemark/expiration")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksAnnosStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_anno_attributes "
      "WHERE name IN (:anno_loading, :anno_loadfailed, :anno_expiration) "
  ), getter_AddRefs(deleteLivemarksAnnosStmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksAnnosStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_loading"), NS_LITERAL_CSTRING("livemark/loading")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksAnnosStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_loadfailed"), NS_LITERAL_CSTRING("livemark/loadfailed")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksAnnosStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_expiration"), NS_LITERAL_CSTRING("livemark/expiration")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteLivemarksAnnosStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV20Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<mozIStorageStatement> deleteOldBookmarkGUIDAnnosStmt;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_items_annos WHERE anno_attribute_id = ("
      "SELECT id FROM moz_anno_attributes "
      "WHERE name = :anno_guid"
    ")"
  ), getter_AddRefs(deleteOldBookmarkGUIDAnnosStmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteOldBookmarkGUIDAnnosStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_guid"), NS_LITERAL_CSTRING("placesInternal/GUID")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteOldBookmarkGUIDAnnosStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM moz_anno_attributes "
      "WHERE name = :anno_guid"
  ), getter_AddRefs(deleteOldBookmarkGUIDAnnosStmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteOldBookmarkGUIDAnnosStmt->BindUTF8StringByName(
    NS_LITERAL_CSTRING("anno_guid"), NS_LITERAL_CSTRING("placesInternal/GUID")
  );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteOldBookmarkGUIDAnnosStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV21Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT prefix FROM moz_hosts"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) {
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_hosts ADD COLUMN prefix"
    ));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
Database::MigrateV22Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  nsresult rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_historyvisits SET session = 0"
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
Database::MigrateV23Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<mozIStorageAsyncStatement> updatePrefixesStmt;
  nsresult rv = mMainConn->CreateAsyncStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_hosts SET prefix = ( " HOSTS_PREFIX_PRIORITY_FRAGMENT ") "
  ), getter_AddRefs(updatePrefixesStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStoragePendingStatement> ps;
  rv = updatePrefixesStmt->ExecuteAsync(nullptr, getter_AddRefs(ps));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV24Up()
{
  MOZ_ASSERT(NS_IsMainThread());

 
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT foreign_count FROM moz_places"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) {
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_places ADD COLUMN foreign_count INTEGER DEFAULT 0 NOT NULL"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<mozIStorageStatement> updateStmt;
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_places SET foreign_count = "
    "(SELECT count(*) FROM moz_bookmarks WHERE fk = moz_places.id) "
  ), getter_AddRefs(updateStmt));
  NS_ENSURE_SUCCESS(rv, rv);
  mozStorageStatementScoper updateScoper(updateStmt);
  rv = updateStmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV25Up()
{
  MOZ_ASSERT(NS_IsMainThread());

  

  
  
  
  {
    nsCOMPtr<mozIStorageStatement> stmt;
    nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT root_name FROM moz_bookmarks_roots"
    ), getter_AddRefs(stmt));
    if (NS_FAILED(rv)) {
      return NS_OK;
    }
  }

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET guid = :guid "
    "WHERE id = (SELECT folder_id FROM moz_bookmarks_roots WHERE root_name = :name) "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  const char *rootNames[] = { "places", "menu", "toolbar", "tags", "unfiled" };
  const char *rootGuids[] = { "root________"
                            , "menu________"
                            , "toolbar_____"
                            , "tags________"
                            , "unfiled_____"
                            };

  for (uint32_t i = 0; i < ArrayLength(rootNames); ++i) {
    
    
    mozStorageStatementScoper scoper(stmt);

    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("name"),
                                      nsDependentCString(rootNames[i]));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("guid"),
                                      nsDependentCString(rootGuids[i]));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = stmt->Execute();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
Database::MigrateV26Up() {
  MOZ_ASSERT(NS_IsMainThread());

  
  nsresult rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET dateAdded = dateAdded - dateAdded % 1000, "
    "                         lastModified = lastModified - lastModified % 1000"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV27Up() {
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT place_id FROM moz_keywords"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) {
    
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_keywords ADD COLUMN place_id INTEGER"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_keywords ADD COLUMN post_data TEXT"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_KEYWORDS_PLACEPOSTDATA);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "INSERT OR REPLACE INTO moz_keywords (id, keyword, place_id, post_data) "
    "SELECT k.id, k.keyword, h.id, MAX(a.content) "
    "FROM moz_places h "
    "JOIN moz_bookmarks b ON b.fk = h.id "
    "JOIN moz_keywords k ON k.id = b.keyword_id "
    "LEFT JOIN moz_items_annos a ON a.item_id = b.id "
                               "AND a.anno_attribute_id = (SELECT id FROM moz_anno_attributes "
                                                          "WHERE name = 'bookmarkProperties/POSTData') "
    "WHERE k.place_id ISNULL "
    "GROUP BY keyword"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "DELETE FROM moz_keywords "
    "WHERE NOT EXISTS (SELECT 1 FROM moz_places WHERE id = moz_keywords.place_id)"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks SET keyword_id = NULL "
    "WHERE NOT EXISTS (SELECT 1 FROM moz_keywords WHERE id = moz_bookmarks.keyword_id)"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_places SET foreign_count = "
    "(SELECT count(*) FROM moz_bookmarks WHERE fk = moz_places.id) + "
    "(SELECT count(*) FROM moz_keywords WHERE place_id = moz_places.id) "
  ));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV28Up() {
  MOZ_ASSERT(NS_IsMainThread());

  
  
  
  
  
  DebugOnly<nsresult> rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "UPDATE moz_keywords "
    "SET post_data = ( "
      "SELECT content FROM moz_items_annos a "
      "JOIN moz_anno_attributes n ON n.id = a.anno_attribute_id "
      "JOIN moz_bookmarks b on b.id = a.item_id "
      "WHERE n.name = 'bookmarkProperties/POSTData' "
      "AND b.keyword_id = moz_keywords.id "
      "ORDER BY b.lastModified DESC "
      "LIMIT 1 "
    ") "
    "WHERE EXISTS(SELECT 1 FROM moz_bookmarks WHERE keyword_id = moz_keywords.id) "
  ));
  
  
  
  
  
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  return NS_OK;
}

void
Database::Shutdown()
{

  
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mClosed);

  
  nsCOMPtr<mozIStorageCompletionCallback> closeListener = mConnectionShutdown.forget();

  if (!mMainConn) {
    
    
    mClosed = true;
    (void)closeListener->Complete(NS_OK, nullptr);
    return;
  }

#ifdef DEBUG
  { 
    bool haveNullGuids = false;
    nsCOMPtr<mozIStorageStatement> stmt;

    nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT 1 "
      "FROM moz_places "
      "WHERE guid IS NULL "
    ), getter_AddRefs(stmt));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    rv = stmt->ExecuteStep(&haveNullGuids);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    MOZ_ASSERT(!haveNullGuids && "Found a page without a GUID!");

    rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT 1 "
      "FROM moz_bookmarks "
      "WHERE guid IS NULL "
    ), getter_AddRefs(stmt));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    rv = stmt->ExecuteStep(&haveNullGuids);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    MOZ_ASSERT(!haveNullGuids && "Found a bookmark without a GUID!");

    rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT 1 "
      "FROM moz_favicons "
      "WHERE guid IS NULL "
    ), getter_AddRefs(stmt));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    rv = stmt->ExecuteStep(&haveNullGuids);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    MOZ_ASSERT(!haveNullGuids && "Found a favicon without a GUID!");
  }

  { 
    
    bool hasUnroundedDates = false;
    nsCOMPtr<mozIStorageStatement> stmt;

    nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT 1 "
        "FROM moz_bookmarks "
        "WHERE dateAdded % 1000 > 0 OR lastModified % 1000 > 0 LIMIT 1"
      ), getter_AddRefs(stmt));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    rv = stmt->ExecuteStep(&hasUnroundedDates);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    MOZ_ASSERT(!hasUnroundedDates && "Found unrounded dates!");
  }
#endif

  mMainThreadStatements.FinalizeStatements();
  mMainThreadAsyncStatements.FinalizeStatements();

  nsRefPtr< FinalizeStatementCacheProxy<mozIStorageStatement> > event =
    new FinalizeStatementCacheProxy<mozIStorageStatement>(
          mAsyncThreadStatements,
          NS_ISUPPORTS_CAST(nsIObserver*, this)
        );
  DispatchToAsyncThread(event);

  mClosed = true;

  (void)mMainConn->AsyncClose(closeListener);
}




NS_IMETHODIMP
Database::Observe(nsISupports *aSubject,
                  const char *aTopic,
                  const char16_t *aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (strcmp(aTopic, TOPIC_PROFILE_CHANGE_TEARDOWN) == 0 ||
      strcmp(aTopic, TOPIC_SIMULATE_PLACES_MUST_CLOSE_1) == 0) {
    
    if (IsShutdownStarted()) {
      return NS_OK;
    }

    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    NS_ENSURE_STATE(os);

    
    
    
    
    
    nsCOMPtr<nsISimpleEnumerator> e;
    if (NS_SUCCEEDED(os->EnumerateObservers(TOPIC_PLACES_INIT_COMPLETE,
                     getter_AddRefs(e))) && e) {
      bool hasMore = false;
      while (NS_SUCCEEDED(e->HasMoreElements(&hasMore)) && hasMore) {
	nsCOMPtr<nsISupports> supports;
        if (NS_SUCCEEDED(e->GetNext(getter_AddRefs(supports)))) {
          nsCOMPtr<nsIObserver> observer = do_QueryInterface(supports);
          (void)observer->Observe(observer, TOPIC_PLACES_INIT_COMPLETE, nullptr);
        }
      }
    }

    
    (void)os->NotifyObservers(nullptr, TOPIC_PLACES_SHUTDOWN, nullptr);
  } else if (strcmp(aTopic, TOPIC_SIMULATE_PLACES_MUST_CLOSE_2) == 0) {
    
    if (IsShutdownStarted()) {
      return NS_OK;
    }

    
    
    nsCOMPtr<nsIAsyncShutdownClient> shutdownPhase = GetShutdownPhase();
    if (shutdownPhase) {
      shutdownPhase->RemoveBlocker(mConnectionShutdown.get());
    }

    return mConnectionShutdown->BlockShutdown(nullptr);
  }
  return NS_OK;
}



} 
} 
