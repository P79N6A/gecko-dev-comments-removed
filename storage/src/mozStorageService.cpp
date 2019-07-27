





#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "prinit.h"
#include "nsAutoPtr.h"
#include "nsCollationCID.h"
#include "nsEmbedCID.h"
#include "nsThreadUtils.h"
#include "mozStoragePrivateHelpers.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "nsIXPConnect.h"
#include "nsIObserverService.h"
#include "nsIPropertyBag2.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "mozilla/LateWriteChecks.h"
#include "mozIStorageCompletionCallback.h"
#include "mozIStoragePendingStatement.h"

#include "sqlite3.h"

#ifdef SQLITE_OS_WIN

#undef CompareString
#endif

#include "nsIPromptService.h"

#ifdef MOZ_STORAGE_MEMORY
#  include "mozmemory.h"
#  ifdef MOZ_DMD
#    include "DMD.h"
#  endif
#endif




#define PREF_TS_SYNCHRONOUS "toolkit.storage.synchronous"
#define PREF_TS_SYNCHRONOUS_DEFAULT 1

#define PREF_TS_PAGESIZE "toolkit.storage.pageSize"



#define PREF_TS_PAGESIZE_DEFAULT 32768

namespace mozilla {
namespace storage {




#ifdef MOZ_DMD
static mozilla::Atomic<size_t> gSqliteMemoryUsed;
#endif

static int64_t
StorageSQLiteDistinguishedAmount()
{
  return ::sqlite3_memory_used();
}






















nsresult
ReportConn(nsIHandleReportCallback *aHandleReport,
           nsISupports *aData,
           Connection *aConn,
           const nsACString &aPathHead,
           const nsACString &aKind,
           const nsACString &aDesc,
           int32_t aOption,
           size_t *aTotal)
{
  nsCString path(aPathHead);
  path.Append(aKind);
  path.AppendLiteral("-used");

  int32_t val = aConn->getSqliteRuntimeStatus(aOption);
  nsresult rv = aHandleReport->Callback(EmptyCString(), path,
                                        nsIMemoryReporter::KIND_HEAP,
                                        nsIMemoryReporter::UNITS_BYTES,
                                        int64_t(val), aDesc, aData);
  NS_ENSURE_SUCCESS(rv, rv);
  *aTotal += val;

  return NS_OK;
}







NS_IMETHODIMP
Service::CollectReports(nsIHandleReportCallback *aHandleReport,
                        nsISupports *aData, bool aAnonymize)
{
  nsresult rv;
  size_t totalConnSize = 0;
  {
    nsTArray<nsRefPtr<Connection> > connections;
    getConnections(connections);

    for (uint32_t i = 0; i < connections.Length(); i++) {
      nsRefPtr<Connection> &conn = connections[i];

      
      bool isReady;
      (void)conn->GetConnectionReady(&isReady);
      if (!isReady) {
          continue;
      }

      nsCString pathHead("explicit/storage/sqlite/");
      
      pathHead.Append(conn->getFilename());
      pathHead.Append('/');

      SQLiteMutexAutoLock lockedScope(conn->sharedDBMutex);

      NS_NAMED_LITERAL_CSTRING(stmtDesc,
        "Memory (approximate) used by all prepared statements used by "
        "connections to this database.");
      rv = ReportConn(aHandleReport, aData, conn, pathHead,
                      NS_LITERAL_CSTRING("stmt"), stmtDesc,
                      SQLITE_DBSTATUS_STMT_USED, &totalConnSize);
      NS_ENSURE_SUCCESS(rv, rv);

      NS_NAMED_LITERAL_CSTRING(cacheDesc,
        "Memory (approximate) used by all pager caches used by connections "
        "to this database.");
      rv = ReportConn(aHandleReport, aData, conn, pathHead,
                      NS_LITERAL_CSTRING("cache"), cacheDesc,
                      SQLITE_DBSTATUS_CACHE_USED, &totalConnSize);
      NS_ENSURE_SUCCESS(rv, rv);

      NS_NAMED_LITERAL_CSTRING(schemaDesc,
        "Memory (approximate) used to store the schema for all databases "
        "associated with connections to this database.");
      rv = ReportConn(aHandleReport, aData, conn, pathHead,
                      NS_LITERAL_CSTRING("schema"), schemaDesc,
                      SQLITE_DBSTATUS_SCHEMA_USED, &totalConnSize);
      NS_ENSURE_SUCCESS(rv, rv);
    }

#ifdef MOZ_DMD
    if (::sqlite3_memory_used() != int64_t(gSqliteMemoryUsed)) {
      NS_WARNING("memory consumption reported by SQLite doesn't match "
                 "our measurements");
    }
#endif
  }

  int64_t other = ::sqlite3_memory_used() - totalConnSize;

  rv = aHandleReport->Callback(
          EmptyCString(),
          NS_LITERAL_CSTRING("explicit/storage/sqlite/other"),
          KIND_HEAP, UNITS_BYTES, other,
          NS_LITERAL_CSTRING("All unclassified sqlite memory."),
          aData);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}




NS_IMPL_ISUPPORTS(
  Service,
  mozIStorageService,
  nsIObserver,
  nsIMemoryReporter
)

Service *Service::gService = nullptr;

Service *
Service::getSingleton()
{
  if (gService) {
    NS_ADDREF(gService);
    return gService;
  }

  
  
  
  if (SQLITE_VERSION_NUMBER > ::sqlite3_libversion_number()) {
    nsCOMPtr<nsIPromptService> ps(do_GetService(NS_PROMPTSERVICE_CONTRACTID));
    if (ps) {
      nsAutoString title, message;
      title.AppendLiteral("SQLite Version Error");
      message.AppendLiteral("The application has been updated, but your version "
                          "of SQLite is too old and the application cannot "
                          "run.");
      (void)ps->Alert(nullptr, title.get(), message.get());
    }
    ::PR_Abort();
  }

  
  
  NS_ENSURE_TRUE(NS_IsMainThread(), nullptr);
  gService = new Service();
  if (gService) {
    NS_ADDREF(gService);
    if (NS_FAILED(gService->initialize()))
      NS_RELEASE(gService);
  }

  return gService;
}

nsIXPConnect *Service::sXPConnect = nullptr;


already_AddRefed<nsIXPConnect>
Service::getXPConnect()
{
  NS_PRECONDITION(NS_IsMainThread(),
                  "Must only get XPConnect on the main thread!");
  NS_PRECONDITION(gService,
                  "Can not get XPConnect without an instance of our service!");

  
  
  nsCOMPtr<nsIXPConnect> xpc(sXPConnect);
  if (!xpc)
    xpc = do_GetService(nsIXPConnect::GetCID());
  NS_ASSERTION(xpc, "Could not get XPConnect!");
  return xpc.forget();
}

int32_t Service::sSynchronousPref;


int32_t
Service::getSynchronousPref()
{
  return sSynchronousPref;
}

int32_t Service::sDefaultPageSize = PREF_TS_PAGESIZE_DEFAULT;

Service::Service()
: mMutex("Service::mMutex")
, mSqliteVFS(nullptr)
, mRegistrationMutex("Service::mRegistrationMutex")
, mConnections()
{
}

Service::~Service()
{
  mozilla::UnregisterWeakMemoryReporter(this);
  mozilla::UnregisterStorageSQLiteDistinguishedAmount();

  int rc = sqlite3_vfs_unregister(mSqliteVFS);
  if (rc != SQLITE_OK)
    NS_WARNING("Failed to unregister sqlite vfs wrapper.");

  
  
  rc = ::sqlite3_shutdown();
  if (rc != SQLITE_OK)
    NS_WARNING("sqlite3 did not shutdown cleanly.");

  DebugOnly<bool> shutdownObserved = !sXPConnect;
  NS_ASSERTION(shutdownObserved, "Shutdown was not observed!");

  gService = nullptr;
  delete mSqliteVFS;
  mSqliteVFS = nullptr;
}

void
Service::registerConnection(Connection *aConnection)
{
  mRegistrationMutex.AssertNotCurrentThreadOwns();
  MutexAutoLock mutex(mRegistrationMutex);
  (void)mConnections.AppendElement(aConnection);
}

void
Service::unregisterConnection(Connection *aConnection)
{
  
  
  
  nsRefPtr<Service> kungFuDeathGrip(this);
  {
    mRegistrationMutex.AssertNotCurrentThreadOwns();
    MutexAutoLock mutex(mRegistrationMutex);

    for (uint32_t i = 0 ; i < mConnections.Length(); ++i) {
      if (mConnections[i] == aConnection) {
        nsCOMPtr<nsIThread> thread = mConnections[i]->threadOpenedOn;

        
        
        
        NS_ProxyRelease(thread,
          static_cast<mozIStorageConnection*>(mConnections[i].forget().take()));

        mConnections.RemoveElementAt(i);
        return;
      }
    }

    MOZ_ASSERT_UNREACHABLE("Attempt to unregister unknown storage connection!");
  }
}

void
Service::getConnections( nsTArray<nsRefPtr<Connection> >& aConnections)
{
  mRegistrationMutex.AssertNotCurrentThreadOwns();
  MutexAutoLock mutex(mRegistrationMutex);
  aConnections.Clear();
  aConnections.AppendElements(mConnections);
}

void
Service::minimizeMemory()
{
  nsTArray<nsRefPtr<Connection> > connections;
  getConnections(connections);

  for (uint32_t i = 0; i < connections.Length(); i++) {
    nsRefPtr<Connection> conn = connections[i];
    if (!conn->connectionReady())
      continue;

    NS_NAMED_LITERAL_CSTRING(shrinkPragma, "PRAGMA shrink_memory");
    nsCOMPtr<mozIStorageConnection> syncConn = do_QueryInterface(
      NS_ISUPPORTS_CAST(mozIStorageAsyncConnection*, conn));
    bool onOpenedThread = false;

    if (!syncConn) {
      
      
      nsCOMPtr<mozIStoragePendingStatement> ps;
      DebugOnly<nsresult> rv =
        conn->ExecuteSimpleSQLAsync(shrinkPragma, nullptr, getter_AddRefs(ps));
      MOZ_ASSERT(NS_SUCCEEDED(rv), "Should have purged sqlite caches");
    } else if (NS_SUCCEEDED(conn->threadOpenedOn->IsOnCurrentThread(&onOpenedThread)) &&
               onOpenedThread) {
      
      conn->ExecuteSimpleSQL(shrinkPragma);
    } else {
      
      
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethodWithArg<const nsCString>(
          conn, &Connection::ExecuteSimpleSQL, shrinkPragma);
      conn->threadOpenedOn->Dispatch(event, NS_DISPATCH_NORMAL);
    }
  }
}

void
Service::shutdown()
{
  NS_IF_RELEASE(sXPConnect);
}

sqlite3_vfs *ConstructTelemetryVFS();

#ifdef MOZ_STORAGE_MEMORY

namespace {



















#ifdef MOZ_DMD














MOZ_DEFINE_MALLOC_SIZE_OF_ON_ALLOC(SqliteMallocSizeOfOnAlloc)
MOZ_DEFINE_MALLOC_SIZE_OF_ON_FREE(SqliteMallocSizeOfOnFree)

#endif

static void *sqliteMemMalloc(int n)
{
  void* p = ::malloc(n);
#ifdef MOZ_DMD
  gSqliteMemoryUsed += SqliteMallocSizeOfOnAlloc(p);
#endif
  return p;
}

static void sqliteMemFree(void *p)
{
#ifdef MOZ_DMD
  gSqliteMemoryUsed -= SqliteMallocSizeOfOnFree(p);
#endif
  ::free(p);
}

static void *sqliteMemRealloc(void *p, int n)
{
#ifdef MOZ_DMD
  gSqliteMemoryUsed -= SqliteMallocSizeOfOnFree(p);
  void *pnew = ::realloc(p, n);
  if (pnew) {
    gSqliteMemoryUsed += SqliteMallocSizeOfOnAlloc(pnew);
  } else {
    
    gSqliteMemoryUsed += SqliteMallocSizeOfOnAlloc(p);
  }
  return pnew;
#else
  return ::realloc(p, n);
#endif
}

static int sqliteMemSize(void *p)
{
  return ::moz_malloc_usable_size(p);
}

static int sqliteMemRoundup(int n)
{
  n = malloc_good_size(n);

  
  
  
  return n <= 8 ? 8 : n;
}

static int sqliteMemInit(void *p)
{
  return 0;
}

static void sqliteMemShutdown(void *p)
{
}

const sqlite3_mem_methods memMethods = {
  &sqliteMemMalloc,
  &sqliteMemFree,
  &sqliteMemRealloc,
  &sqliteMemSize,
  &sqliteMemRoundup,
  &sqliteMemInit,
  &sqliteMemShutdown,
  nullptr
};

} 

#endif  

static const char* sObserverTopics[] = {
  "memory-pressure",
  "xpcom-shutdown",
  "xpcom-shutdown-threads"
};

nsresult
Service::initialize()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be initialized on the main thread");

  int rc;

#ifdef MOZ_STORAGE_MEMORY
  rc = ::sqlite3_config(SQLITE_CONFIG_MALLOC, &memMethods);
  if (rc != SQLITE_OK)
    return convertResultCode(rc);
#endif

  
  
  
  rc = ::sqlite3_initialize();
  if (rc != SQLITE_OK)
    return convertResultCode(rc);

  mSqliteVFS = ConstructTelemetryVFS();
  if (mSqliteVFS) {
    rc = sqlite3_vfs_register(mSqliteVFS, 1);
    if (rc != SQLITE_OK)
      return convertResultCode(rc);
  } else {
    NS_WARNING("Failed to register telemetry VFS");
  }

  
  
  nsCOMPtr<nsIObserverService> os = mozilla::services::GetObserverService();
  NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);

  for (size_t i = 0; i < ArrayLength(sObserverTopics); ++i) {
    nsresult rv = os->AddObserver(this, sObserverTopics[i], false);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  
  (void)CallGetService(nsIXPConnect::GetCID(), &sXPConnect);

  
  
  
  sSynchronousPref =
    Preferences::GetInt(PREF_TS_SYNCHRONOUS, PREF_TS_SYNCHRONOUS_DEFAULT);

  
  
  
  sDefaultPageSize =
      Preferences::GetInt(PREF_TS_PAGESIZE, PREF_TS_PAGESIZE_DEFAULT);

  mozilla::RegisterWeakMemoryReporter(this);
  mozilla::RegisterStorageSQLiteDistinguishedAmount(StorageSQLiteDistinguishedAmount);

  return NS_OK;
}

int
Service::localeCompareStrings(const nsAString &aStr1,
                              const nsAString &aStr2,
                              int32_t aComparisonStrength)
{
  
  
  
  MutexAutoLock mutex(mMutex);

  nsICollation *coll = getLocaleCollation();
  if (!coll) {
    NS_ERROR("Storage service has no collation");
    return 0;
  }

  int32_t res;
  nsresult rv = coll->CompareString(aComparisonStrength, aStr1, aStr2, &res);
  if (NS_FAILED(rv)) {
    NS_ERROR("Collation compare string failed");
    return 0;
  }

  return res;
}

nsICollation *
Service::getLocaleCollation()
{
  mMutex.AssertCurrentThreadOwns();

  if (mLocaleCollation)
    return mLocaleCollation;

  nsCOMPtr<nsILocaleService> svc(do_GetService(NS_LOCALESERVICE_CONTRACTID));
  if (!svc) {
    NS_WARNING("Could not get locale service");
    return nullptr;
  }

  nsCOMPtr<nsILocale> appLocale;
  nsresult rv = svc->GetApplicationLocale(getter_AddRefs(appLocale));
  if (NS_FAILED(rv)) {
    NS_WARNING("Could not get application locale");
    return nullptr;
  }

  nsCOMPtr<nsICollationFactory> collFact =
    do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID);
  if (!collFact) {
    NS_WARNING("Could not create collation factory");
    return nullptr;
  }

  rv = collFact->CreateCollation(appLocale, getter_AddRefs(mLocaleCollation));
  if (NS_FAILED(rv)) {
    NS_WARNING("Could not create collation");
    return nullptr;
  }

  return mLocaleCollation;
}





NS_IMETHODIMP
Service::OpenSpecialDatabase(const char *aStorageKey,
                             mozIStorageConnection **_connection)
{
  nsresult rv;

  nsCOMPtr<nsIFile> storageFile;
  if (::strcmp(aStorageKey, "memory") == 0) {
    
    
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  nsRefPtr<Connection> msc = new Connection(this, SQLITE_OPEN_READWRITE, false);

  rv = storageFile ? msc->initialize(storageFile) : msc->initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  msc.forget(_connection);
  return NS_OK;

}

namespace {

class AsyncInitDatabase final : public nsRunnable
{
public:
  AsyncInitDatabase(Connection* aConnection,
                    nsIFile* aStorageFile,
                    int32_t aGrowthIncrement,
                    mozIStorageCompletionCallback* aCallback)
    : mConnection(aConnection)
    , mStorageFile(aStorageFile)
    , mGrowthIncrement(aGrowthIncrement)
    , mCallback(aCallback)
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    nsresult rv = mStorageFile ? mConnection->initialize(mStorageFile)
                               : mConnection->initialize();
    if (NS_FAILED(rv)) {
      nsCOMPtr<nsIRunnable> closeRunnable =
        NS_NewRunnableMethodWithArg<mozIStorageCompletionCallback*>(
          mConnection.get(),
          &Connection::AsyncClose,
          nullptr);
      MOZ_ASSERT(closeRunnable);
      MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(closeRunnable)));

      return DispatchResult(rv, nullptr);
    }

    if (mGrowthIncrement >= 0) {
      
      (void)mConnection->SetGrowthIncrement(mGrowthIncrement, EmptyCString());
    }

    return DispatchResult(NS_OK, NS_ISUPPORTS_CAST(mozIStorageAsyncConnection*,
                          mConnection));
  }

private:
  nsresult DispatchResult(nsresult aStatus, nsISupports* aValue) {
    nsRefPtr<CallbackComplete> event =
      new CallbackComplete(aStatus,
                           aValue,
                           mCallback.forget());
    return NS_DispatchToMainThread(event);
  }

  ~AsyncInitDatabase()
  {
    nsCOMPtr<nsIThread> thread;
    DebugOnly<nsresult> rv = NS_GetMainThread(getter_AddRefs(thread));
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    (void)NS_ProxyRelease(thread, mStorageFile);

    
    Connection *rawConnection = nullptr;
    mConnection.swap(rawConnection);
    (void)NS_ProxyRelease(thread, NS_ISUPPORTS_CAST(mozIStorageConnection *,
                                                    rawConnection));

    
    
    
    mozIStorageCompletionCallback *rawCallback = nullptr;
    mCallback.swap(rawCallback);
    (void)NS_ProxyRelease(thread, rawCallback);
  }

  nsRefPtr<Connection> mConnection;
  nsCOMPtr<nsIFile> mStorageFile;
  int32_t mGrowthIncrement;
  nsRefPtr<mozIStorageCompletionCallback> mCallback;
};

} 

NS_IMETHODIMP
Service::OpenAsyncDatabase(nsIVariant *aDatabaseStore,
                           nsIPropertyBag2 *aOptions,
                           mozIStorageCompletionCallback *aCallback)
{
  if (!NS_IsMainThread()) {
    return NS_ERROR_NOT_SAME_THREAD;
  }
  NS_ENSURE_ARG(aDatabaseStore);
  NS_ENSURE_ARG(aCallback);

  nsCOMPtr<nsIFile> storageFile;
  int flags = SQLITE_OPEN_READWRITE;

  nsCOMPtr<nsISupports> dbStore;
  nsresult rv = aDatabaseStore->GetAsISupports(getter_AddRefs(dbStore));
  if (NS_SUCCEEDED(rv)) {
    
    storageFile = do_QueryInterface(dbStore, &rv);
    if (NS_FAILED(rv)) {
      return NS_ERROR_INVALID_ARG;
    }

    rv = storageFile->Clone(getter_AddRefs(storageFile));
    MOZ_ASSERT(NS_SUCCEEDED(rv));

    
    flags |= SQLITE_OPEN_CREATE;

    
    bool shared = false;
    if (aOptions) {
      rv = aOptions->GetPropertyAsBool(NS_LITERAL_STRING("shared"), &shared);
      if (NS_FAILED(rv) && rv != NS_ERROR_NOT_AVAILABLE) {
        return NS_ERROR_INVALID_ARG;
      }
    }
    flags |= shared ? SQLITE_OPEN_SHAREDCACHE : SQLITE_OPEN_PRIVATECACHE;
  } else {
    
    nsAutoCString keyString;
    rv = aDatabaseStore->GetAsACString(keyString);
    if (NS_FAILED(rv) || !keyString.EqualsLiteral("memory")) {
      return NS_ERROR_INVALID_ARG;
    }

    
    
  }

  int32_t growthIncrement = -1;
  if (aOptions && storageFile) {
    rv = aOptions->GetPropertyAsInt32(NS_LITERAL_STRING("growthIncrement"),
                                      &growthIncrement);
    if (NS_FAILED(rv) && rv != NS_ERROR_NOT_AVAILABLE) {
      return NS_ERROR_INVALID_ARG;
    }
  }

  
  nsRefPtr<Connection> msc = new Connection(this, flags, true);
  nsCOMPtr<nsIEventTarget> target = msc->getAsyncExecutionTarget();
  MOZ_ASSERT(target, "Cannot initialize a connection that has been closed already");

  nsRefPtr<AsyncInitDatabase> asyncInit =
    new AsyncInitDatabase(msc,
                          storageFile,
                          growthIncrement,
                          aCallback);
  return target->Dispatch(asyncInit, nsIEventTarget::DISPATCH_NORMAL);
}

NS_IMETHODIMP
Service::OpenDatabase(nsIFile *aDatabaseFile,
                      mozIStorageConnection **_connection)
{
  NS_ENSURE_ARG(aDatabaseFile);

  
  
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_SHAREDCACHE |
              SQLITE_OPEN_CREATE;
  nsRefPtr<Connection> msc = new Connection(this, flags, false);

  nsresult rv = msc->initialize(aDatabaseFile);
  NS_ENSURE_SUCCESS(rv, rv);

  msc.forget(_connection);
  return NS_OK;
}

NS_IMETHODIMP
Service::OpenUnsharedDatabase(nsIFile *aDatabaseFile,
                              mozIStorageConnection **_connection)
{
  NS_ENSURE_ARG(aDatabaseFile);

  
  
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_PRIVATECACHE |
              SQLITE_OPEN_CREATE;
  nsRefPtr<Connection> msc = new Connection(this, flags, false);

  nsresult rv = msc->initialize(aDatabaseFile);
  NS_ENSURE_SUCCESS(rv, rv);

  msc.forget(_connection);
  return NS_OK;
}

NS_IMETHODIMP
Service::OpenDatabaseWithFileURL(nsIFileURL *aFileURL,
                                 mozIStorageConnection **_connection)
{
  NS_ENSURE_ARG(aFileURL);

  
  
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_SHAREDCACHE |
              SQLITE_OPEN_CREATE | SQLITE_OPEN_URI;
  nsRefPtr<Connection> msc = new Connection(this, flags, false);

  nsresult rv = msc->initialize(aFileURL);
  NS_ENSURE_SUCCESS(rv, rv);

  msc.forget(_connection);
  return NS_OK;
}

NS_IMETHODIMP
Service::BackupDatabaseFile(nsIFile *aDBFile,
                            const nsAString &aBackupFileName,
                            nsIFile *aBackupParentDirectory,
                            nsIFile **backup)
{
  nsresult rv;
  nsCOMPtr<nsIFile> parentDir = aBackupParentDirectory;
  if (!parentDir) {
    
    
    rv = aDBFile->GetParent(getter_AddRefs(parentDir));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIFile> backupDB;
  rv = parentDir->Clone(getter_AddRefs(backupDB));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = backupDB->Append(aBackupFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = backupDB->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString fileName;
  rv = backupDB->GetLeafName(fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = backupDB->Remove(false);
  NS_ENSURE_SUCCESS(rv, rv);

  backupDB.forget(backup);

  return aDBFile->CopyTo(parentDir, fileName);
}




NS_IMETHODIMP
Service::Observe(nsISupports *, const char *aTopic, const char16_t *)
{
  if (strcmp(aTopic, "memory-pressure") == 0) {
    minimizeMemory();
  } else if (strcmp(aTopic, "xpcom-shutdown") == 0) {
    shutdown();
  } else if (strcmp(aTopic, "xpcom-shutdown-threads") == 0) {
    nsCOMPtr<nsIObserverService> os =
      mozilla::services::GetObserverService();

    for (size_t i = 0; i < ArrayLength(sObserverTopics); ++i) {
      (void)os->RemoveObserver(this, sObserverTopics[i]);
    }

    bool anyOpen = false;
    do {
      nsTArray<nsRefPtr<Connection> > connections;
      getConnections(connections);
      anyOpen = false;
      for (uint32_t i = 0; i < connections.Length(); i++) {
        nsRefPtr<Connection> &conn = connections[i];
        if (conn->isClosing()) {
          anyOpen = true;
          break;
        }
      }
      if (anyOpen) {
        nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
        NS_ProcessNextEvent(thread);
      }
    } while (anyOpen);

    if (gShutdownChecks == SCM_CRASH) {
      nsTArray<nsRefPtr<Connection> > connections;
      getConnections(connections);
      for (uint32_t i = 0, n = connections.Length(); i < n; i++) {
        if (!connections[i]->isClosed()) {
          MOZ_CRASH();
        }
      }
    }
  }

  return NS_OK;
}

} 
} 
