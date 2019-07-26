





#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"

#include "mozStorageService.h"
#include "mozStorageConnection.h"
#include "prinit.h"
#include "pratom.h"
#include "nsAutoPtr.h"
#include "nsCollationCID.h"
#include "nsEmbedCID.h"
#include "nsThreadUtils.h"
#include "mozStoragePrivateHelpers.h"
#include "nsILocale.h"
#include "nsILocaleService.h"
#include "nsIXPConnect.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"

#include "sqlite3.h"

#ifdef SQLITE_OS_WIN

#undef CompareString
#endif

#include "nsIPromptService.h"
#include "nsIMemoryReporter.h"




#define PREF_TS_SYNCHRONOUS "toolkit.storage.synchronous"
#define PREF_TS_SYNCHRONOUS_DEFAULT 1

namespace mozilla {
namespace storage {




static int64_t
GetStorageSQLiteMemoryUsed()
{
  return ::sqlite3_memory_used();
}





NS_MEMORY_REPORTER_IMPLEMENT(StorageSQLite,
    "storage-sqlite",
    KIND_OTHER,
    UNITS_BYTES,
    GetStorageSQLiteMemoryUsed,
    "Memory used by SQLite.")

class StorageSQLiteMultiReporter MOZ_FINAL : public nsIMemoryMultiReporter
{
private:
  Service *mService;    
  nsCString mStmtDesc;
  nsCString mCacheDesc;
  nsCString mSchemaDesc;

public:
  NS_DECL_ISUPPORTS

  StorageSQLiteMultiReporter(Service *aService) 
  : mService(aService)
  {
    mStmtDesc = NS_LITERAL_CSTRING(
      "Memory (approximate) used by all prepared statements used by "
      "connections to this database.");

    mCacheDesc = NS_LITERAL_CSTRING(
      "Memory (approximate) used by all pager caches used by connections "
      "to this database.");

    mSchemaDesc = NS_LITERAL_CSTRING(
      "Memory (approximate) used to store the schema for all databases "
      "associated with connections to this database.");
  }

  NS_IMETHOD GetName(nsACString &aName)
  {
      aName.AssignLiteral("storage-sqlite");
      return NS_OK;
  }

  
  
  
  
  
  
  NS_IMETHOD CollectReports(nsIMemoryMultiReporterCallback *aCb,
                            nsISupports *aClosure)
  {
    nsresult rv;
    size_t totalConnSize = 0;
    {
      nsTArray<nsRefPtr<Connection> > connections;
      mService->getConnections(connections);

      for (uint32_t i = 0; i < connections.Length(); i++) {
        nsRefPtr<Connection> &conn = connections[i];

        
        bool isReady;
        (void)conn->GetConnectionReady(&isReady);
        if (!isReady) {
            continue;
        }

        nsCString pathHead("explicit/storage/sqlite/");
        pathHead.Append(conn->getFilename());
        pathHead.AppendLiteral("/");

        SQLiteMutexAutoLock lockedScope(conn->sharedDBMutex);

        rv = reportConn(aCb, aClosure, *conn.get(), pathHead,
                        NS_LITERAL_CSTRING("stmt"), mStmtDesc,
                        SQLITE_DBSTATUS_STMT_USED, &totalConnSize);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = reportConn(aCb, aClosure, *conn.get(), pathHead,
                        NS_LITERAL_CSTRING("cache"), mCacheDesc,
                        SQLITE_DBSTATUS_CACHE_USED, &totalConnSize);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = reportConn(aCb, aClosure, *conn.get(), pathHead,
                        NS_LITERAL_CSTRING("schema"), mSchemaDesc,
                        SQLITE_DBSTATUS_SCHEMA_USED, &totalConnSize);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    int64_t other = ::sqlite3_memory_used() - totalConnSize;

    rv = aCb->Callback(NS_LITERAL_CSTRING(""),
                       NS_LITERAL_CSTRING("explicit/storage/sqlite/other"),
                       nsIMemoryReporter::KIND_HEAP,
                       nsIMemoryReporter::UNITS_BYTES, other,
                       NS_LITERAL_CSTRING("All unclassified sqlite memory."),
                       aClosure);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  NS_IMETHOD GetExplicitNonHeap(int64_t *aAmount)
  {
    
    *aAmount = 0;
    return NS_OK;
  }

private:
  





















  nsresult reportConn(nsIMemoryMultiReporterCallback *aCb,
                      nsISupports *aClosure,
                      sqlite3 *aConn,
                      const nsACString &aPathHead,
                      const nsACString &aKind,
                      const nsACString &aDesc,
                      int aOption,
                      size_t *aTotal)
  {
    nsCString path(aPathHead);
    path.Append(aKind);
    path.AppendLiteral("-used");

    int curr = 0, max = 0;
    int rc = ::sqlite3_db_status(aConn, aOption, &curr, &max, 0);
    nsresult rv = convertResultCode(rc);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aCb->Callback(NS_LITERAL_CSTRING(""), path,
                       nsIMemoryReporter::KIND_HEAP,
                       nsIMemoryReporter::UNITS_BYTES, int64_t(curr),
                       aDesc, aClosure);
    NS_ENSURE_SUCCESS(rv, rv);
    *aTotal += curr;

    return NS_OK;
  }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(
  StorageSQLiteMultiReporter,
  nsIMemoryMultiReporter
)




class ServiceMainThreadInitializer : public nsRunnable
{
public:
  ServiceMainThreadInitializer(Service *aService,
                               nsIObserver *aObserver,
                               nsIXPConnect **aXPConnectPtr,
                               int32_t *aSynchronousPrefValPtr)
  : mService(aService)
  , mObserver(aObserver)
  , mXPConnectPtr(aXPConnectPtr)
  , mSynchronousPrefValPtr(aSynchronousPrefValPtr)
  {
  }

  NS_IMETHOD Run()
  {
    NS_PRECONDITION(NS_IsMainThread(), "Must be running on the main thread!");

    
    
    
    
    

    
    
    nsCOMPtr<nsIObserverService> os =
      mozilla::services::GetObserverService();
    NS_ENSURE_TRUE(os, NS_ERROR_FAILURE);
    nsresult rv = os->AddObserver(mObserver, "xpcom-shutdown", false);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = os->AddObserver(mObserver, "xpcom-shutdown-threads", false);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    (void)CallGetService(nsIXPConnect::GetCID(), mXPConnectPtr);

    
    
    
    int32_t synchronous =
      Preferences::GetInt(PREF_TS_SYNCHRONOUS, PREF_TS_SYNCHRONOUS_DEFAULT);
    ::PR_ATOMIC_SET(mSynchronousPrefValPtr, synchronous);

    
    
    mService->mStorageSQLiteReporter = new NS_MEMORY_REPORTER_NAME(StorageSQLite);
    mService->mStorageSQLiteMultiReporter = new StorageSQLiteMultiReporter(mService);
    (void)::NS_RegisterMemoryReporter(mService->mStorageSQLiteReporter);
    (void)::NS_RegisterMemoryMultiReporter(mService->mStorageSQLiteMultiReporter);

    return NS_OK;
  }

private:
  Service *mService;
  nsIObserver *mObserver;
  nsIXPConnect **mXPConnectPtr;
  int32_t *mSynchronousPrefValPtr;
};




NS_IMPL_THREADSAFE_ISUPPORTS2(
  Service,
  mozIStorageService,
  nsIObserver
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
      title.AppendASCII("SQLite Version Error");
      message.AppendASCII("The application has been updated, but your version "
                          "of SQLite is too old and the application cannot "
                          "run.");
      (void)ps->Alert(nullptr, title.get(), message.get());
    }
    ::PR_Abort();
  }

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

Service::Service()
: mMutex("Service::mMutex")
, mSqliteVFS(nullptr)
, mRegistrationMutex("Service::mRegistrationMutex")
, mConnections()
, mStorageSQLiteReporter(nullptr)
, mStorageSQLiteMultiReporter(nullptr)
{
}

Service::~Service()
{
  (void)::NS_UnregisterMemoryReporter(mStorageSQLiteReporter);
  (void)::NS_UnregisterMemoryMultiReporter(mStorageSQLiteMultiReporter);

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
    DebugOnly<bool> removed = mConnections.RemoveElement(aConnection);
    
    MOZ_ASSERT(removed);
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
Service::shutdown()
{
  NS_IF_RELEASE(sXPConnect);
}

sqlite3_vfs *ConstructTelemetryVFS();

#ifdef MOZ_STORAGE_MEMORY
#  include "mozmemory.h"

namespace {



















#ifdef MOZ_DMD

#include "DMD.h"











NS_MEMORY_REPORTER_MALLOC_SIZEOF_ON_ALLOC_FUN(SqliteMallocSizeOfOnAlloc)
NS_MEMORY_REPORTER_MALLOC_SIZEOF_ON_FREE_FUN(SqliteMallocSizeOfOnFree)

#endif

static void *sqliteMemMalloc(int n)
{
  void* p = ::moz_malloc(n);
#ifdef MOZ_DMD
  SqliteMallocSizeOfOnAlloc(p);
#endif
  return p;
}

static void sqliteMemFree(void *p)
{
#ifdef MOZ_DMD
  SqliteMallocSizeOfOnFree(p);
#endif
  ::moz_free(p);
}

static void *sqliteMemRealloc(void *p, int n)
{
#ifdef MOZ_DMD
  SqliteMallocSizeOfOnFree(p);
  void *pnew = ::moz_realloc(p, n);
  if (pnew) {
    SqliteMallocSizeOfOnAlloc(pnew);
  } else {
    
    SqliteMallocSizeOfOnAlloc(p);
  }
  return pnew;
#else
  return ::moz_realloc(p, n);
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
  NULL
};

} 

#endif  

nsresult
Service::initialize()
{
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

  
  
  sSynchronousPref = PREF_TS_SYNCHRONOUS_DEFAULT;

  
  nsCOMPtr<nsIRunnable> event =
    new ServiceMainThreadInitializer(this, this, &sXPConnect, &sSynchronousPref);
  if (event && ::NS_IsMainThread()) {
    (void)event->Run();
  }
  else {
    (void)::NS_DispatchToMainThread(event);
  }

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




#ifndef NS_APP_STORAGE_50_FILE
#define NS_APP_STORAGE_50_FILE "UStor"
#endif

NS_IMETHODIMP
Service::OpenSpecialDatabase(const char *aStorageKey,
                             mozIStorageConnection **_connection)
{
  nsresult rv;

  nsCOMPtr<nsIFile> storageFile;
  if (::strcmp(aStorageKey, "memory") == 0) {
    
    
  }
  else if (::strcmp(aStorageKey, "profile") == 0) {
    rv = NS_GetSpecialDirectory(NS_APP_STORAGE_50_FILE,
                                getter_AddRefs(storageFile));
    NS_ENSURE_SUCCESS(rv, rv);

    
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  nsRefPtr<Connection> msc = new Connection(this, SQLITE_OPEN_READWRITE);

  rv = storageFile ? msc->initialize(storageFile) : msc->initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  msc.forget(_connection);
  return NS_OK;

}

NS_IMETHODIMP
Service::OpenDatabase(nsIFile *aDatabaseFile,
                      mozIStorageConnection **_connection)
{
  NS_ENSURE_ARG(aDatabaseFile);

  
  
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_SHAREDCACHE |
              SQLITE_OPEN_CREATE;
  nsRefPtr<Connection> msc = new Connection(this, flags);

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
  nsRefPtr<Connection> msc = new Connection(this, flags);

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
  nsRefPtr<Connection> msc = new Connection(this, flags);

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
Service::Observe(nsISupports *, const char *aTopic, const PRUnichar *)
{
  if (strcmp(aTopic, "xpcom-shutdown") == 0)
    shutdown();
  if (strcmp(aTopic, "xpcom-shutdown-threads") == 0) {
    nsCOMPtr<nsIObserverService> os =
      mozilla::services::GetObserverService();
    os->RemoveObserver(this, "xpcom-shutdown-threads");
    bool anyOpen = false;
    do {
      nsTArray<nsRefPtr<Connection> > connections;
      getConnections(connections);
      anyOpen = false;
      for (uint32_t i = 0; i < connections.Length(); i++) {
        nsRefPtr<Connection> &conn = connections[i];

        
        
        if (conn->isAsyncClosing()) {
          anyOpen = true;
          break;
        }
      }
      if (anyOpen) {
        nsCOMPtr<nsIThread> thread = do_GetCurrentThread();
        NS_ProcessNextEvent(thread);
      }
    } while (anyOpen);

#ifdef DEBUG
    nsTArray<nsRefPtr<Connection> > connections;
    getConnections(connections);
    for (uint32_t i = 0, n = connections.Length(); i < n; i++) {
      MOZ_ASSERT(!connections[i]->ConnectionReady());
    }
#endif
  }

  return NS_OK;
}

} 
} 
