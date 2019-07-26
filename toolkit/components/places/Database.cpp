



#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Util.h"

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


#define RECENT_BACKUP_TIME_MICROSEC (int64_t)86400 * PR_USEC_PER_SEC // 24H


#define DATABASE_FILENAME NS_LITERAL_STRING("places.sqlite")

#define DATABASE_CORRUPT_FILENAME NS_LITERAL_STRING("places.sqlite.corrupt")


#define PREF_FORCE_DATABASE_REPLACEMENT "places.database.replaceOnStartup"




#define DATABASE_MAX_WAL_SIZE_IN_KIBIBYTES 512

#define BYTES_PER_MEBIBYTE 1048576


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

class BlockingConnectionCloseCallback MOZ_FINAL : public mozIStorageCompletionCallback {
  bool mDone;

public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_MOZISTORAGECOMPLETIONCALLBACK
  BlockingConnectionCloseCallback();
  void Spin();
};

NS_IMETHODIMP
BlockingConnectionCloseCallback::Complete(nsresult, nsISupports*)
{
  mDone = true;
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  MOZ_ASSERT(os);
  if (!os)
    return NS_OK;
  DebugOnly<nsresult> rv = os->NotifyObservers(nullptr,
                                               TOPIC_PLACES_CONNECTION_CLOSED,
                                               nullptr);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  return NS_OK;
}

BlockingConnectionCloseCallback::BlockingConnectionCloseCallback()
  : mDone(false)
{
  MOZ_ASSERT(NS_IsMainThread());
}

void BlockingConnectionCloseCallback::Spin() {
  nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
  while (!mDone) {
    NS_ProcessNextEvent(thread);
  }
}

NS_IMPL_ISUPPORTS1(
  BlockingConnectionCloseCallback
, mozIStorageCompletionCallback
)

nsresult
CreateRoot(nsCOMPtr<mozIStorageConnection>& aDBConn,
           const nsCString& aRootName,
           const nsXPIDLString& titleString)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  static int32_t itemPosition = 0;

  
  
  static PRTime timestamp = 0;
  if (!timestamp)
    timestamp = PR_Now();

  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_bookmarks "
      "(type, position, title, dateAdded, lastModified, guid, parent) "
    "VALUES (:item_type, :item_position, :item_title,"
            ":date_added, :last_modified, GENERATE_GUID(),"
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
  rv = stmt->Execute();
  if (NS_FAILED(rv)) return rv;

  
  nsCOMPtr<mozIStorageStatement> newRootStmt;
  rv = aDBConn->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO moz_bookmarks_roots (root_name, folder_id) "
    "VALUES (:root_name, "
              "(SELECT id from moz_bookmarks WHERE "
              " position = :item_position AND "
              " parent = IFNULL((SELECT MIN(folder_id) FROM moz_bookmarks_roots), 0)))"
  ), getter_AddRefs(newRootStmt));
  if (NS_FAILED(rv)) return rv;

  rv = newRootStmt->BindUTF8StringByName(NS_LITERAL_CSTRING("root_name"),
                                         aRootName);
  if (NS_FAILED(rv)) return rv;
  rv = newRootStmt->BindInt32ByName(NS_LITERAL_CSTRING("item_position"),
                                    itemPosition);
  if (NS_FAILED(rv)) return rv;
  rv = newRootStmt->Execute();
  if (NS_FAILED(rv)) return rv;

  
  
  if (!aRootName.Equals("places"))
    ++itemPosition;

  return NS_OK;
}


} 




PLACES_FACTORY_SINGLETON_IMPLEMENTATION(Database, gDatabase)

NS_IMPL_ISUPPORTS2(Database
, nsIObserver
, nsISupportsWeakReference
)

Database::Database()
  : mMainThreadStatements(mMainConn)
  , mMainThreadAsyncStatements(mMainConn)
  , mAsyncThreadStatements(mMainConn)
  , mDBPageSize(0)
  , mDatabaseStatus(nsINavHistoryService::DATABASE_STATUS_OK)
  , mShuttingDown(false)
  , mClosed(false)
{
  
  MOZ_ASSERT(!gDatabase);
  gDatabase = this;
}

Database::~Database()
{
  
  
  MOZ_ASSERT(gDatabase == this);

  
  if (gDatabase == this) {
    gDatabase = nullptr;
  }
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
    (void)os->AddObserver(this, TOPIC_PROFILE_BEFORE_CHANGE, true);
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

  
  (void)mMainConn->SetGrowthIncrement(10 * BYTES_PER_MEBIBYTE, EmptyCString());

  
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

      if (currentSchemaVersion < 6) {
        
        
        return NS_ERROR_FILE_CORRUPTED;
      }

      

      if (currentSchemaVersion < 7) {
        rv = MigrateV7Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 8) {
        rv = MigrateV8Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 9) {
        rv = MigrateV9Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (currentSchemaVersion < 10) {
        rv = MigrateV10Up();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      

      if (currentSchemaVersion < 11) {
        rv = MigrateV11Up();
        NS_ENSURE_SUCCESS(rv, rv);
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
  
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("places"), rootTitle);
  if (NS_FAILED(rv)) return rv;

  
  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BookmarksMenuFolderTitle").get(),
                                 getter_Copies(rootTitle));
  if (NS_FAILED(rv)) return rv;
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("menu"), rootTitle);
  if (NS_FAILED(rv)) return rv;

  rv = bundle->GetStringFromName(NS_LITERAL_STRING("BookmarksToolbarFolderTitle").get(),
                                 getter_Copies(rootTitle));
  if (NS_FAILED(rv)) return rv;
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("toolbar"), rootTitle);
  if (NS_FAILED(rv)) return rv;

  rv = bundle->GetStringFromName(NS_LITERAL_STRING("TagsFolderTitle").get(),
                                 getter_Copies(rootTitle));
  if (NS_FAILED(rv)) return rv;
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("tags"), rootTitle);
  if (NS_FAILED(rv)) return rv;

  rv = bundle->GetStringFromName(NS_LITERAL_STRING("UnsortedBookmarksFolderTitle").get(),
                                 getter_Copies(rootTitle));
  if (NS_FAILED(rv)) return rv;
  rv = CreateRoot(mMainConn, NS_LITERAL_CSTRING("unfiled"), rootTitle);
  if (NS_FAILED(rv)) return rv;

#if DEBUG
  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT "
      "(SELECT COUNT(*) FROM moz_bookmarks), "
      "(SELECT COUNT(*) FROM moz_bookmarks_roots), "
      "(SELECT SUM(position) FROM moz_bookmarks WHERE "
        "id IN (SELECT folder_id FROM moz_bookmarks_roots))"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) return rv;

  bool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  if (NS_FAILED(rv)) return rv;
  MOZ_ASSERT(hasResult);
  int32_t bookmarkCount = 0;
  rv = stmt->GetInt32(0, &bookmarkCount);
  if (NS_FAILED(rv)) return rv;
  int32_t rootCount = 0;
  rv = stmt->GetInt32(1, &rootCount);
  if (NS_FAILED(rv)) return rv;
  int32_t positionSum = 0;
  rv = stmt->GetInt32(2, &positionSum);
  if (NS_FAILED(rv)) return rv;
  MOZ_ASSERT(bookmarkCount == 5 && rootCount == 5 && positionSum == 6);
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
    "UPDATE moz_bookmarks SET title = :new_title WHERE id = "
      "(SELECT folder_id FROM moz_bookmarks_roots WHERE root_name = :root_name)"
  ), getter_AddRefs(stmt));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<mozIStorageBindingParamsArray> paramsArray;
  rv = stmt->NewBindingParamsArray(getter_AddRefs(paramsArray));
  if (NS_FAILED(rv)) return rv;

  const char *rootNames[] = { "menu", "toolbar", "tags", "unfiled" };
  const char *titleStringIDs[] = {
    "BookmarksMenuFolderTitle", "BookmarksToolbarFolderTitle",
    "TagsFolderTitle", "UnsortedBookmarksFolderTitle"
  };

  for (uint32_t i = 0; i < ArrayLength(rootNames); ++i) {
    nsXPIDLString title;
    rv = bundle->GetStringFromName(NS_ConvertASCIItoUTF16(titleStringIDs[i]).get(),
                                   getter_Copies(title));
    if (NS_FAILED(rv)) return rv;

    nsCOMPtr<mozIStorageBindingParams> params;
    rv = paramsArray->NewBindingParams(getter_AddRefs(params));
    if (NS_FAILED(rv)) return rv;
    rv = params->BindUTF8StringByName(NS_LITERAL_CSTRING("root_name"),
                                      nsDependentCString(rootNames[i]));
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
Database::CheckAndUpdateGUIDs()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsCOMPtr<mozIStorageStatement> updateStmt;
  nsresult rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks "
    "SET guid = :guid "
    "WHERE id = :item_id "
  ), getter_AddRefs(updateStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<mozIStorageStatement> stmt;
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT item_id, content "
    "FROM moz_items_annos "
    "JOIN moz_anno_attributes "
    "WHERE name = :anno_name "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = stmt->BindUTF8StringByName(NS_LITERAL_CSTRING("anno_name"),
                                  SYNCGUID_ANNO);
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasResult;
  while (NS_SUCCEEDED(stmt->ExecuteStep(&hasResult)) && hasResult) {
    int64_t itemId;
    rv = stmt->GetInt64(0, &itemId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoCString guid;
    rv = stmt->GetUTF8String(1, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!IsValidGUID(guid)) {
      continue;
    }

    mozStorageStatementScoper updateScoper(updateStmt);
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

  
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
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

  
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_bookmarks "
    "SET guid = GENERATE_GUID() "
    "WHERE guid IS NULL "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_places "
    "SET guid = :guid "
    "WHERE id = :place_id "
  ), getter_AddRefs(updateStmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
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
    int64_t placeId;
    rv = stmt->GetInt64(0, &placeId);
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoCString guid;
    rv = stmt->GetUTF8String(1, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (!IsValidGUID(guid)) {
      continue;
    }

    mozStorageStatementScoper updateScoper(updateStmt);
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

  
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
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

  
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
    "UPDATE moz_places "
    "SET guid = GENERATE_GUID() "
    "WHERE guid IS NULL "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
Database::MigrateV7Up() 
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  bool URLUniqueIndexExists = false;
  nsresult rv = mMainConn->IndexExists(NS_LITERAL_CSTRING(
    "moz_places_url_uniqueindex"
  ), &URLUniqueIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!URLUniqueIndexExists) {
    return NS_ERROR_FILE_CORRUPTED;
  }

  mozStorageTransaction transaction(mMainConn, false);

  
  
  bool lastModIndexExists = false;
  rv = mMainConn->IndexExists(
    NS_LITERAL_CSTRING("moz_bookmarks_itemlastmodifiedindex"),
    &lastModIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!lastModIndexExists) {
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_PLACELASTMODIFIED);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  bool pageIndexExists = false;
  rv = mMainConn->IndexExists(
    NS_LITERAL_CSTRING("moz_historyvisits_pageindex"), &pageIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (pageIndexExists) {
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_historyvisits_pageindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_HISTORYVISITS_PLACEDATE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<mozIStorageStatement> hasFrecencyStatement;
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT frecency FROM moz_places"),
    getter_AddRefs(hasFrecencyStatement));

  if (NS_FAILED(rv)) {
    
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_places ADD frecency INTEGER DEFAULT -1 NOT NULL"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_FRECENCY);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<mozIStorageAsyncStatement> stmt = GetAsyncStatement(
      "UPDATE moz_places SET frecency = ( "
        "CASE "
        "WHEN url BETWEEN 'place:' AND 'place;' "
        "THEN 0 "
        "ELSE -1 "
        "END "
      ") "
    );
    NS_ENSURE_STATE(stmt);
    nsCOMPtr<mozIStoragePendingStatement> ps;
    (void)stmt->ExecuteAsync(nullptr, getter_AddRefs(ps));
  }

  
  nsCOMPtr<mozIStorageStatement> moveUnfiledBookmarks;
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
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
  NS_ENSURE_SUCCESS(rv, rv);
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
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT name "
      "FROM sqlite_master "
      "WHERE type = 'trigger' "
      "AND name = :trigger_name"),
    getter_AddRefs(triggerDetection));
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool triggerExists;
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
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DELETE FROM moz_keywords "
        "WHERE id IN ("
          "SELECT k.id "
          "FROM moz_keywords k "
          "LEFT OUTER JOIN moz_bookmarks b "
          "ON b.keyword_id = k.id "
          "WHERE b.id IS NULL"
        ")"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  bool tableExists = false;
  rv = mMainConn->TableExists(NS_LITERAL_CSTRING("moz_inputhistory"),
                              &tableExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!tableExists) {
    rv = mMainConn->ExecuteSimpleSQL(CREATE_MOZ_INPUTHISTORY);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return transaction.Commit();
}


nsresult
Database::MigrateV8Up()
{
  MOZ_ASSERT(NS_IsMainThread());
  mozStorageTransaction transaction(mMainConn, false);

  nsresult rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TRIGGER IF EXISTS moz_historyvisits_afterinsert_v1_trigger"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP TRIGGER IF EXISTS moz_historyvisits_afterdelete_v1_trigger"));
  NS_ENSURE_SUCCESS(rv, rv);


  
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP INDEX IF EXISTS moz_places_titleindex"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP INDEX IF EXISTS moz_annos_item_idindex"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "DROP INDEX IF EXISTS moz_annos_place_idindex"));
  NS_ENSURE_SUCCESS(rv, rv);

  
  bool oldIndexExists = false;
  rv = mMainConn->IndexExists(NS_LITERAL_CSTRING("moz_annos_attributesindex"), &oldIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (oldIndexExists) {
    
    rv = mMainConn->ExecuteSimpleSQL(
        NS_LITERAL_CSTRING("DROP INDEX moz_annos_attributesindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_ANNOS_PLACEATTRIBUTE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "DROP INDEX IF EXISTS moz_items_annos_attributesindex"));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_ITEMSANNOS_PLACEATTRIBUTE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return transaction.Commit();
}


nsresult
Database::MigrateV9Up()
{
  MOZ_ASSERT(NS_IsMainThread());
  mozStorageTransaction transaction(mMainConn, false);
  
  
  
  
  
  bool oldIndexExists = false;
  nsresult rv = mMainConn->IndexExists(
    NS_LITERAL_CSTRING("moz_places_lastvisitdateindex"), &oldIndexExists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!oldIndexExists) {
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "ALTER TABLE moz_places ADD last_visit_date INTEGER"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_LASTVISITDATE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
        "UPDATE moz_places SET last_visit_date = "
          "(SELECT MAX(visit_date) "
           "FROM moz_historyvisits "
           "WHERE place_id = moz_places.id)"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return transaction.Commit();
}


nsresult
Database::MigrateV10Up()
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  nsresult rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "UPDATE moz_bookmarks SET lastModified = dateAdded "
      "WHERE lastModified IS NULL"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


nsresult
Database::MigrateV11Up()
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  
  nsresult rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
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
  rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
      "SELECT guid FROM moz_bookmarks"),
    getter_AddRefs(hasGuidStatement));

  if (NS_FAILED(rv)) {
    
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_bookmarks "
      "ADD COLUMN guid TEXT"
    ));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_BOOKMARKS_GUID);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    rv = mMainConn->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
      "ALTER TABLE moz_places "
      "ADD COLUMN guid TEXT"
    ));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMainConn->ExecuteSimpleSQL(CREATE_IDX_MOZ_PLACES_GUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = CheckAndUpdateGUIDs();
  NS_ENSURE_SUCCESS(rv, rv);

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

void
Database::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!mShuttingDown);
  MOZ_ASSERT(!mClosed);

  mShuttingDown = true;

  mMainThreadStatements.FinalizeStatements();
  mMainThreadAsyncStatements.FinalizeStatements();

  nsRefPtr< FinalizeStatementCacheProxy<mozIStorageStatement> > event =
    new FinalizeStatementCacheProxy<mozIStorageStatement>(
          mAsyncThreadStatements, NS_ISUPPORTS_CAST(nsIObserver*, this)
        );
  DispatchToAsyncThread(event);

  nsRefPtr<BlockingConnectionCloseCallback> closeListener =
    new BlockingConnectionCloseCallback();
  (void)mMainConn->AsyncClose(closeListener);
  closeListener->Spin();

  mClosed = true;
}




NS_IMETHODIMP
Database::Observe(nsISupports *aSubject,
                  const char *aTopic,
                  const PRUnichar *aData)
{
  MOZ_ASSERT(NS_IsMainThread());
 
  if (strcmp(aTopic, TOPIC_PROFILE_CHANGE_TEARDOWN) == 0) {
    
    if (mShuttingDown) {
      return NS_OK;
    }

    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    NS_ENSURE_STATE(os);

    
    
    
    
    
    nsCOMPtr<nsISimpleEnumerator> e;
    if (NS_SUCCEEDED(os->EnumerateObservers(TOPIC_PLACES_INIT_COMPLETE,
                     getter_AddRefs(e))) && e) {
      bool hasMore = false;
      while (NS_SUCCEEDED(e->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsIObserver> observer;
        if (NS_SUCCEEDED(e->GetNext(getter_AddRefs(observer)))) {
          (void)observer->Observe(observer, TOPIC_PLACES_INIT_COMPLETE, nullptr);
        }
      }
    }

    
    (void)os->NotifyObservers(nullptr, TOPIC_PLACES_SHUTDOWN, nullptr);
  }

  else if (strcmp(aTopic, TOPIC_PROFILE_BEFORE_CHANGE) == 0) {
    
    if (mShuttingDown) {
      return NS_OK;
    }

    
    nsCOMPtr<nsIObserverService> os = services::GetObserverService();
    if (os) {
      (void)os->NotifyObservers(nullptr, TOPIC_PLACES_WILL_CLOSE_CONNECTION, nullptr);
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
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->ExecuteStep(&haveNullGuids);
      NS_ENSURE_SUCCESS(rv, rv);
      MOZ_ASSERT(!haveNullGuids && "Found a page without a GUID!");

      rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT 1 "
        "FROM moz_bookmarks "
        "WHERE guid IS NULL "
      ), getter_AddRefs(stmt));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->ExecuteStep(&haveNullGuids);
      NS_ENSURE_SUCCESS(rv, rv);
      MOZ_ASSERT(!haveNullGuids && "Found a bookmark without a GUID!");

      rv = mMainConn->CreateStatement(NS_LITERAL_CSTRING(
        "SELECT 1 "
        "FROM moz_favicons "
        "WHERE guid IS NULL "
      ), getter_AddRefs(stmt));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = stmt->ExecuteStep(&haveNullGuids);
      NS_ENSURE_SUCCESS(rv, rv);
      MOZ_ASSERT(!haveNullGuids && "Found a favicon without a GUID!");
    }
#endif

    
    Shutdown();
  }

  return NS_OK;
}

} 
} 
