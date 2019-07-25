









































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
#include "test_quota.c"

#include "nsIPromptService.h"
#include "nsIMemoryReporter.h"

#include "mozilla/FunctionTimer.h"
#include "mozilla/Util.h"

namespace {

class QuotaCallbackData
{
public:
  QuotaCallbackData(mozIStorageQuotaCallback *aCallback,
                    nsISupports *aUserData)
  : callback(aCallback), userData(aUserData)
  {
    MOZ_COUNT_CTOR(QuotaCallbackData);
  }

  ~QuotaCallbackData()
  {
    MOZ_COUNT_DTOR(QuotaCallbackData);
  }

  static void Callback(const char *zFilename,
                       sqlite3_int64 *piLimit,
                       sqlite3_int64 iSize,
                       void *pArg)
  {
    NS_ASSERTION(zFilename && strlen(zFilename), "Null or empty filename!");
    NS_ASSERTION(piLimit, "Null pointer!");

    QuotaCallbackData *data = static_cast<QuotaCallbackData*>(pArg);
    if (!data) {
      
      return;
    }

    NS_ASSERTION(data->callback, "Should never have a null callback!");

    nsDependentCString filename(zFilename);

    PRInt64 newLimit;
    if (NS_SUCCEEDED(data->callback->QuotaExceeded(filename, *piLimit,
                                                   iSize, data->userData,
                                                   &newLimit))) {
      *piLimit = newLimit;
    }
  }

  static void Destroy(void *aUserData)
  {
    delete static_cast<QuotaCallbackData*>(aUserData);
  }

private:
  nsCOMPtr<mozIStorageQuotaCallback> callback;
  nsCOMPtr<nsISupports> userData;
};

} 




#define PREF_TS_SYNCHRONOUS "toolkit.storage.synchronous"
#define PREF_TS_SYNCHRONOUS_DEFAULT 1

namespace mozilla {
namespace storage {




static PRInt64
GetStorageSQLiteMemoryUsed(void *)
{
  return ::sqlite3_memory_used();
}

NS_MEMORY_REPORTER_IMPLEMENT(StorageSQLiteMemoryUsed,
    "explicit/storage/sqlite",
    MR_HEAP,
    "Memory used by SQLite.",
    GetStorageSQLiteMemoryUsed,
    nsnull)




class ServiceMainThreadInitializer : public nsRunnable
{
public:
  ServiceMainThreadInitializer(nsIObserver *aObserver,
                               nsIXPConnect **aXPConnectPtr,
                               PRInt32 *aSynchronousPrefValPtr)
  : mObserver(aObserver)
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
    nsresult rv = os->AddObserver(mObserver, "xpcom-shutdown", PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    (void)CallGetService(nsIXPConnect::GetCID(), mXPConnectPtr);

    
    
    
    PRInt32 synchronous =
      Preferences::GetInt(PREF_TS_SYNCHRONOUS, PREF_TS_SYNCHRONOUS_DEFAULT);
    ::PR_ATOMIC_SET(mSynchronousPrefValPtr, synchronous);

    
    
    NS_RegisterMemoryReporter(new NS_MEMORY_REPORTER_NAME(StorageSQLiteMemoryUsed));

    return NS_OK;
  }

private:
  nsIObserver *mObserver;
  nsIXPConnect **mXPConnectPtr;
  PRInt32 *mSynchronousPrefValPtr;
};




NS_IMPL_THREADSAFE_ISUPPORTS3(
  Service,
  mozIStorageService,
  nsIObserver,
  mozIStorageServiceQuotaManagement
)

Service *Service::gService = nsnull;

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
      (void)ps->Alert(nsnull, title.get(), message.get());
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

nsIXPConnect *Service::sXPConnect = nsnull;


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

PRInt32 Service::sSynchronousPref;


PRInt32
Service::getSynchronousPref()
{
  return sSynchronousPref;
}

Service::Service()
: mMutex("Service::mMutex")
{
}

Service::~Service()
{
  
  
  int rc = ::sqlite3_quota_shutdown();
  if (rc != SQLITE_OK)
    NS_WARNING("sqlite3 did not shutdown cleanly.");

  rc = ::sqlite3_shutdown();
  if (rc != SQLITE_OK)
    NS_WARNING("sqlite3 did not shutdown cleanly.");

  DebugOnly<bool> shutdownObserved = !sXPConnect;
  NS_ASSERTION(shutdownObserved, "Shutdown was not observed!");

  gService = nsnull;
}

void
Service::shutdown()
{
  NS_IF_RELEASE(sXPConnect);
}

nsresult
Service::initialize()
{
  NS_TIME_FUNCTION;

  int rc;

  
  
  
  rc = ::sqlite3_initialize();
  if (rc != SQLITE_OK)
    return convertResultCode(rc);

  rc = ::sqlite3_quota_initialize(NULL, 0);
  if (rc != SQLITE_OK)
    return convertResultCode(rc);

  
  
  sSynchronousPref = PREF_TS_SYNCHRONOUS_DEFAULT;

  
  nsCOMPtr<nsIRunnable> event =
    new ServiceMainThreadInitializer(this, &sXPConnect, &sSynchronousPref);
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
                              PRInt32 aComparisonStrength)
{
  
  
  
  MutexAutoLock mutex(mMutex);

  nsICollation *coll = getLocaleCollation();
  if (!coll) {
    NS_ERROR("Storage service has no collation");
    return 0;
  }

  PRInt32 res;
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
    return nsnull;
  }

  nsCOMPtr<nsILocale> appLocale;
  nsresult rv = svc->GetApplicationLocale(getter_AddRefs(appLocale));
  if (NS_FAILED(rv)) {
    NS_WARNING("Could not get application locale");
    return nsnull;
  }

  nsCOMPtr<nsICollationFactory> collFact =
    do_CreateInstance(NS_COLLATIONFACTORY_CONTRACTID);
  if (!collFact) {
    NS_WARNING("Could not create collation factory");
    return nsnull;
  }

  rv = collFact->CreateCollation(appLocale, getter_AddRefs(mLocaleCollation));
  if (NS_FAILED(rv)) {
    NS_WARNING("Could not create collation");
    return nsnull;
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

    nsString filename;
    storageFile->GetPath(filename);
    nsCString filename8 = NS_ConvertUTF16toUTF8(filename.get());
    
  }
  else {
    return NS_ERROR_INVALID_ARG;
  }

  Connection *msc = new Connection(this, SQLITE_OPEN_READWRITE);
  NS_ENSURE_TRUE(msc, NS_ERROR_OUT_OF_MEMORY);

  rv = msc->initialize(storageFile);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_connection = msc);
  return NS_OK;
}

NS_IMETHODIMP
Service::OpenDatabase(nsIFile *aDatabaseFile,
                      mozIStorageConnection **_connection)
{
  NS_ENSURE_ARG(aDatabaseFile);

#ifdef NS_FUNCTION_TIMER
  nsCString leafname;
  (void)aDatabaseFile->GetNativeLeafName(leafname);
  NS_TIME_FUNCTION_FMT("mozIStorageService::OpenDatabase(%s)", leafname.get());
#endif

  
  
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_SHAREDCACHE |
              SQLITE_OPEN_CREATE;
  nsRefPtr<Connection> msc = new Connection(this, flags);
  NS_ENSURE_TRUE(msc, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = msc->initialize(aDatabaseFile);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_connection = msc);
  return NS_OK;
}

NS_IMETHODIMP
Service::OpenUnsharedDatabase(nsIFile *aDatabaseFile,
                              mozIStorageConnection **_connection)
{
#ifdef NS_FUNCTION_TIMER
  nsCString leafname;
  (void)aDatabaseFile->GetNativeLeafName(leafname);
  NS_TIME_FUNCTION_FMT("mozIStorageService::OpenUnsharedDatabase(%s)",
                       leafname.get());
#endif

  
  
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_PRIVATECACHE |
              SQLITE_OPEN_CREATE;
  nsRefPtr<Connection> msc = new Connection(this, flags);
  NS_ENSURE_TRUE(msc, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = msc->initialize(aDatabaseFile);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_connection = msc);
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

  rv = backupDB->Remove(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  backupDB.forget(backup);

  return aDBFile->CopyTo(parentDir, fileName);
}




NS_IMETHODIMP
Service::Observe(nsISupports *, const char *aTopic, const PRUnichar *)
{
  if (strcmp(aTopic, "xpcom-shutdown") == 0)
    shutdown();
  return NS_OK;
}




NS_IMETHODIMP
Service::OpenDatabaseWithVFS(nsIFile *aDatabaseFile,
                             const nsACString &aVFSName,
                             mozIStorageConnection **_connection)
{
  NS_ENSURE_ARG(aDatabaseFile);

#ifdef NS_FUNCTION_TIMER
  nsCString leafname;
  (void)aDatabaseFile->GetNativeLeafName(leafname);
  NS_TIME_FUNCTION_FMT("mozIStorageService::OpenDatabaseWithVFS(%s)",
                       leafname.get());
#endif

  
  
  int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_SHAREDCACHE |
              SQLITE_OPEN_CREATE;
  nsRefPtr<Connection> msc = new Connection(this, flags);
  NS_ENSURE_TRUE(msc, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = msc->initialize(aDatabaseFile,
                                PromiseFlatCString(aVFSName).get());
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_connection = msc);
  return NS_OK;
}

NS_IMETHODIMP
Service::SetQuotaForFilenamePattern(const nsACString &aPattern,
                                    PRInt64 aSizeLimit,
                                    mozIStorageQuotaCallback *aCallback,
                                    nsISupports *aUserData)
{
  NS_ENSURE_FALSE(aPattern.IsEmpty(), NS_ERROR_INVALID_ARG);

  nsAutoPtr<QuotaCallbackData> data;
  if (aSizeLimit && aCallback) {
    data = new QuotaCallbackData(aCallback, aUserData);
  }

  int rc = ::sqlite3_quota_set(PromiseFlatCString(aPattern).get(),
                               aSizeLimit, QuotaCallbackData::Callback,
                               data, QuotaCallbackData::Destroy);
  NS_ENSURE_TRUE(rc == SQLITE_OK, convertResultCode(rc));

  data.forget();
  return NS_OK;
}

} 
} 
