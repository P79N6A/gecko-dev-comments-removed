






































#include "IndexedDatabaseManager.h"

#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsISHEntry.h"
#include "nsISimpleEnumerator.h"
#include "nsITimer.h"

#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/storage.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"

#include "AsyncConnectionHelper.h"
#include "IDBDatabase.h"
#include "IDBEvents.h"
#include "IDBFactory.h"
#include "LazyIdleThread.h"
#include "TransactionThreadPool.h"



#define DEFAULT_THREAD_TIMEOUT_MS 30000



#define DEFAULT_SHUTDOWN_TIMER_MS 30000


#define DEFAULT_QUOTA_MB 50


#define PREF_INDEXEDDB_QUOTA "dom.indexedDB.warningQuota"


#define BAD_TLS_INDEX (PRUintn)-1

USING_INDEXEDDB_NAMESPACE
using namespace mozilla::services;
using mozilla::Preferences;

namespace {

PRInt32 gShutdown = 0;


IndexedDatabaseManager* gInstance = nsnull;

PRUintn gCurrentDatabaseIndex = BAD_TLS_INDEX;

PRInt32 gIndexedDBQuotaMB = DEFAULT_QUOTA_MB;

class QuotaCallback : public mozIStorageQuotaCallback
{
public:
  NS_DECL_ISUPPORTS

  NS_IMETHOD
  QuotaExceeded(const nsACString& aFilename,
                PRInt64 aCurrentSizeLimit,
                PRInt64 aCurrentTotalSize,
                nsISupports* aUserData,
                PRInt64* _retval)
  {
    NS_ASSERTION(gCurrentDatabaseIndex != BAD_TLS_INDEX,
                 "This should be impossible!");

    IDBDatabase* database =
      static_cast<IDBDatabase*>(PR_GetThreadPrivate(gCurrentDatabaseIndex));

    if (database && database->IsQuotaDisabled()) {
      *_retval = 0;
      return NS_OK;
    }

    return NS_ERROR_FAILURE;
  }
};

NS_IMPL_THREADSAFE_ISUPPORTS1(QuotaCallback, mozIStorageQuotaCallback)


PLDHashOperator
EnumerateToTArray(const nsACString& aKey,
                  nsTArray<IDBDatabase*>* aValue,
                  void* aUserArg)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aKey.IsEmpty(), "Empty key!");
  NS_ASSERTION(aValue, "Null pointer!");
  NS_ASSERTION(aUserArg, "Null pointer!");

  nsTArray<IDBDatabase*>* array =
    static_cast<nsTArray<IDBDatabase*>*>(aUserArg);

  if (!array->AppendElements(*aValue)) {
    NS_WARNING("Out of memory!");
    return PL_DHASH_STOP;
  }

  return PL_DHASH_NEXT;
}



class DelayedSetVersion : public nsRunnable
{
public:
  DelayedSetVersion(IDBDatabase* aDatabase,
                    IDBVersionChangeRequest* aRequest,
                    const nsAString& aVersion,
                    AsyncConnectionHelper* aHelper)
  : mDatabase(aDatabase),
    mRequest(aRequest),
    mVersion(aVersion),
    mHelper(aHelper)
  { }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
    NS_ASSERTION(mgr, "This should never be null!");

    nsresult rv = mgr->SetDatabaseVersion(mDatabase, mRequest, mVersion,
                                          mHelper);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  nsRefPtr<IDBDatabase> mDatabase;
  nsRefPtr<IDBVersionChangeRequest> mRequest;
  nsString mVersion;
  nsRefPtr<AsyncConnectionHelper> mHelper;
};




class VersionChangeEventsRunnable : public nsRunnable
{
public:
  VersionChangeEventsRunnable(
                            IDBDatabase* aRequestingDatabase,
                            IDBVersionChangeRequest* aRequest,
                            nsTArray<nsRefPtr<IDBDatabase> >& aWaitingDatabases,
                            const nsAString& aVersion)
  : mRequestingDatabase(aRequestingDatabase),
    mRequest(aRequest),
    mVersion(aVersion)
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    NS_ASSERTION(aRequestingDatabase, "Null pointer!");
    NS_ASSERTION(aRequest, "Null pointer!");

    if (!mWaitingDatabases.SwapElements(aWaitingDatabases)) {
      NS_ERROR("This should never fail!");
    }
  }

  NS_IMETHOD Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

    
    
    for (PRUint32 index = 0; index < mWaitingDatabases.Length(); index++) {
      nsRefPtr<IDBDatabase>& database = mWaitingDatabases[index];

      if (database->IsClosed()) {
        continue;
      }

      
      nsCOMPtr<nsIDocument> ownerDoc = database->GetOwnerDocument();
      nsIBFCacheEntry* bfCacheEntry;
      if (ownerDoc && (bfCacheEntry = ownerDoc->GetBFCacheEntry())) {
        bfCacheEntry->RemoveFromBFCacheSync();
        NS_ASSERTION(database->IsClosed(),
                     "Kicking doc out of bfcache should have closed database");
        continue;
      }

      
      nsCOMPtr<nsIDOMEvent> event(IDBVersionChangeEvent::Create(mVersion));
      NS_ENSURE_TRUE(event, NS_ERROR_FAILURE);

      bool dummy;
      database->DispatchEvent(event, &dummy);
    }

    
    
    for (PRUint32 index = 0; index < mWaitingDatabases.Length(); index++) {
      if (!mWaitingDatabases[index]->IsClosed()) {
        nsCOMPtr<nsIDOMEvent> event =
          IDBVersionChangeEvent::CreateBlocked(mVersion);
        NS_ENSURE_TRUE(event, NS_ERROR_FAILURE);

        bool dummy;
        mRequest->DispatchEvent(event, &dummy);

        break;
      }
    }

    return NS_OK;
  }

private:
  nsRefPtr<IDBDatabase> mRequestingDatabase;
  nsRefPtr<IDBVersionChangeRequest> mRequest;
  nsTArray<nsRefPtr<IDBDatabase> > mWaitingDatabases;
  nsString mVersion;
};

} 

IndexedDatabaseManager::IndexedDatabaseManager()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!gInstance, "More than one instance!");
}

IndexedDatabaseManager::~IndexedDatabaseManager()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(gInstance == this, "Different instances!");
  gInstance = nsnull;
}


already_AddRefed<IndexedDatabaseManager>
IndexedDatabaseManager::GetOrCreate()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (IsShuttingDown()) {
    NS_ERROR("Calling GetOrCreateInstance() after shutdown!");
    return nsnull;
  }

  nsRefPtr<IndexedDatabaseManager> instance(gInstance);

  if (!instance) {
    
    if (gCurrentDatabaseIndex == BAD_TLS_INDEX) {
      if (PR_NewThreadPrivateIndex(&gCurrentDatabaseIndex, nsnull) !=
          PR_SUCCESS) {
        NS_ERROR("PR_NewThreadPrivateIndex failed!");
        gCurrentDatabaseIndex = BAD_TLS_INDEX;
        return nsnull;
      }

      if (NS_FAILED(Preferences::AddIntVarCache(&gIndexedDBQuotaMB,
                                                PREF_INDEXEDDB_QUOTA,
                                                DEFAULT_QUOTA_MB))) {
        NS_WARNING("Unable to respond to quota pref changes!");
        gIndexedDBQuotaMB = DEFAULT_QUOTA_MB;
      }
    }

    instance = new IndexedDatabaseManager();

    if (!instance->mLiveDatabases.Init()) {
      NS_WARNING("Out of memory!");
      return nsnull;
    }

    
    
    instance->mShutdownTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
    NS_ENSURE_TRUE(instance->mShutdownTimer, nsnull);

    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    NS_ENSURE_TRUE(obs, nsnull);

    
    nsresult rv = obs->AddObserver(instance, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                   false);
    NS_ENSURE_SUCCESS(rv, nsnull);

    
    
    
    rv = obs->AddObserver(instance, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID,
                          false);
    NS_ENSURE_SUCCESS(rv, nsnull);

    
    
    instance->mIOThread = new LazyIdleThread(DEFAULT_THREAD_TIMEOUT_MS,
                                             LazyIdleThread::ManualShutdown);

    
    instance->mQuotaCallbackSingleton = new QuotaCallback();

    
    gInstance = instance;
  }

  return instance.forget();
}


IndexedDatabaseManager*
IndexedDatabaseManager::Get()
{
  
  return gInstance;
}


IndexedDatabaseManager*
IndexedDatabaseManager::FactoryCreate()
{
  
  
  return GetOrCreate().get();
}

bool
IndexedDatabaseManager::RegisterDatabase(IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");

  
  if (IsShuttingDown()) {
    return false;
  }

  
  nsTArray<IDBDatabase*>* array;
  if (!mLiveDatabases.Get(aDatabase->Origin(), &array)) {
    nsAutoPtr<nsTArray<IDBDatabase*> > newArray(new nsTArray<IDBDatabase*>());
    if (!mLiveDatabases.Put(aDatabase->Origin(), newArray)) {
      NS_WARNING("Out of memory?");
      return false;
    }
    array = newArray.forget();
  }
  if (!array->AppendElement(aDatabase)) {
    NS_WARNING("Out of memory?");
    return false;
  }

  aDatabase->mRegistered = true;
  return true;
}

void
IndexedDatabaseManager::UnregisterDatabase(IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");

  
  
  nsTArray<IDBDatabase*>* array;
  if (mLiveDatabases.Get(aDatabase->Origin(), &array) &&
      array->RemoveElement(aDatabase)) {
    if (array->IsEmpty()) {
      mLiveDatabases.Remove(aDatabase->Origin());
    }
    return;
  }
  NS_ERROR("Didn't know anything about this database!");
}

void
IndexedDatabaseManager::OnOriginClearComplete(OriginClearRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aRunnable, "Null pointer!");
  NS_ASSERTION(!aRunnable->mThread, "Thread should be null!");
  NS_ASSERTION(aRunnable->mDelayedRunnables.IsEmpty(),
               "Delayed runnables should have been dispatched already!");

  if (!mOriginClearRunnables.RemoveElement(aRunnable)) {
    NS_ERROR("Don't know anything about this runnable!");
  }
}

void
IndexedDatabaseManager::OnUsageCheckComplete(AsyncUsageRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aRunnable, "Null pointer!");
  NS_ASSERTION(!aRunnable->mURI, "Should have been cleared!");
  NS_ASSERTION(!aRunnable->mCallback, "Should have been cleared!");

  if (!mUsageRunnables.RemoveElement(aRunnable)) {
    NS_ERROR("Don't know anything about this runnable!");
  }
}

void
IndexedDatabaseManager::OnSetVersionRunnableComplete(
                                                  SetVersionRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aRunnable, "Null pointer!");
  NS_ASSERTION(aRunnable->mDelayedRunnables.IsEmpty(),
               "Delayed runnables should have been dispatched already!");

  
  
  if (!mSetVersionRunnables.RemoveElement(aRunnable)) {
    NS_ERROR("Don't know anything about this runnable!");
  }
}

nsresult
IndexedDatabaseManager::WaitForOpenAllowed(const nsAString& aName,
                                           const nsACString& aOrigin,
                                           nsIRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aName.IsEmpty(), "Empty name!");
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");
  NS_ASSERTION(aRunnable, "Null pointer!");

  
  
  PRUint32 count = mOriginClearRunnables.Length();
  for (PRUint32 index = 0; index < count; index++) {
    nsRefPtr<OriginClearRunnable>& data = mOriginClearRunnables[index];
    if (data->mOrigin == aOrigin) {
      nsCOMPtr<nsIRunnable>* newPtr =
        data->mDelayedRunnables.AppendElement(aRunnable);
      NS_ENSURE_TRUE(newPtr, NS_ERROR_OUT_OF_MEMORY);

      return NS_OK;
    }
  }

  
  
  for (PRUint32 index = 0; index < mSetVersionRunnables.Length(); index++) {
    nsRefPtr<SetVersionRunnable>& runnable = mSetVersionRunnables[index];
    if (runnable->mRequestingDatabase->Name() == aName &&
        runnable->mRequestingDatabase->Origin() == aOrigin) {
      nsCOMPtr<nsIRunnable>* newPtr =
        runnable->mDelayedRunnables.AppendElement(aRunnable);
      NS_ENSURE_TRUE(newPtr, NS_ERROR_OUT_OF_MEMORY);

      return NS_OK;
    }
  }

  
  
  
  return NS_DispatchToCurrentThread(aRunnable);
}


bool
IndexedDatabaseManager::IsShuttingDown()
{
  return !!gShutdown;
}

nsresult
IndexedDatabaseManager::SetDatabaseVersion(IDBDatabase* aDatabase,
                                           IDBVersionChangeRequest* aRequest,
                                           const nsAString& aVersion,
                                           AsyncConnectionHelper* aHelper)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");
  NS_ASSERTION(aHelper, "Null pointer!");

  nsresult rv;

  
  for (PRUint32 index = 0; index < mSetVersionRunnables.Length(); index++) {
    nsRefPtr<SetVersionRunnable>& runnable = mSetVersionRunnables[index];
    if (runnable->mRequestingDatabase->Id() == aDatabase->Id()) {
      if (runnable->mRequestingDatabase == aDatabase) {
        
        
        nsRefPtr<DelayedSetVersion> delayed =
          new DelayedSetVersion(aDatabase, aRequest, aVersion, aHelper);
        if (!runnable->mDelayedRunnables.AppendElement(delayed)) {
          NS_WARNING("Out of memory!");
          return NS_ERROR_OUT_OF_MEMORY;
        }
        return NS_OK;
      }

      
      aHelper->SetError(NS_ERROR_DOM_INDEXEDDB_DEADLOCK_ERR);

      rv = NS_DispatchToCurrentThread(aHelper);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_OK;
    }
  }

  
  nsTArray<IDBDatabase*>* array;
  if (!mLiveDatabases.Get(aDatabase->Origin(), &array)) {
    NS_ERROR("Must have some alive if we've got a live argument!");
  }

  
  
  nsTArray<nsRefPtr<IDBDatabase> > liveDatabases;

  for (PRUint32 index = 0; index < array->Length(); index++) {
    IDBDatabase*& database = array->ElementAt(index);
    if (database != aDatabase &&
        database->Id() == aDatabase->Id() &&
        !database->IsClosed() &&
        !liveDatabases.AppendElement(database)) {
      NS_WARNING("Out of memory?");
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  nsRefPtr<SetVersionRunnable> runnable =
    new SetVersionRunnable(aDatabase, liveDatabases);
  if (!mSetVersionRunnables.AppendElement(runnable)) {
    NS_WARNING("Out of memory!");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ASSERTION(liveDatabases.IsEmpty(), "Should have swapped!");

  
  
  runnable->mHelper = aHelper;

  if (runnable->mDatabases.IsEmpty()) {
    
    
    RunSetVersionTransaction(aDatabase);
  }
  else {
    
    
    nsTArray<nsRefPtr<IDBDatabase> > waitingDatabases;
    if (!waitingDatabases.AppendElements(runnable->mDatabases)) {
      NS_WARNING("Out of memory!");
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsRefPtr<VersionChangeEventsRunnable> eventsRunnable =
      new VersionChangeEventsRunnable(aDatabase, aRequest, waitingDatabases,
                                      aVersion);

    rv = NS_DispatchToCurrentThread(eventsRunnable);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
IndexedDatabaseManager::AbortCloseDatabasesForWindow(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWindow, "Null pointer!");

  nsAutoTArray<IDBDatabase*, 50> liveDatabases;
  mLiveDatabases.EnumerateRead(EnumerateToTArray, &liveDatabases);

  TransactionThreadPool* pool = TransactionThreadPool::Get();

  for (PRUint32 index = 0; index < liveDatabases.Length(); index++) {
    IDBDatabase*& database = liveDatabases[index];
    if (database->Owner() == aWindow) {
      if (NS_FAILED(database->Close())) {
        NS_WARNING("Failed to close database for dying window!");
      }

      if (pool) {
        pool->AbortTransactionsForDatabase(database);
      }
    }
  }
}

bool
IndexedDatabaseManager::HasOpenTransactions(nsPIDOMWindow* aWindow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aWindow, "Null pointer!");

  nsAutoTArray<IDBDatabase*, 50> liveDatabases;
  mLiveDatabases.EnumerateRead(EnumerateToTArray, &liveDatabases);

  TransactionThreadPool* pool = TransactionThreadPool::Get();
  if (!pool) {
    return false;
  }

  for (PRUint32 index = 0; index < liveDatabases.Length(); index++) {
    IDBDatabase*& database = liveDatabases[index];
    if (database->Owner() == aWindow &&
        pool->HasTransactionsForDatabase(database)) {
      return true;
    }
  }
  
  return false;
}

void
IndexedDatabaseManager::OnDatabaseClosed(IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");

  
  
  for (PRUint32 index = 0; index < mSetVersionRunnables.Length(); index++) {
    nsRefPtr<SetVersionRunnable>& runnable = mSetVersionRunnables[index];

    if (runnable->mRequestingDatabase->Id() == aDatabase->Id()) {
      
      
      
      
      if (!runnable->mDatabases.IsEmpty() &&
          !runnable->mDatabases.RemoveElement(aDatabase)) {
        NS_ERROR("Didn't have this database in our list!");
      }

      
      if (runnable->mHelper && runnable->mDatabases.IsEmpty()) {
        
        nsRefPtr<AsyncConnectionHelper> helper;
        helper.swap(runnable->mHelper);

        if (NS_FAILED(helper->DispatchToTransactionPool())) {
          NS_WARNING("Failed to dispatch to thread pool!");
        }

        
        
        
        TransactionThreadPool* pool = TransactionThreadPool::Get();
        NS_ASSERTION(pool, "This should never be null!");

        
        
        nsAutoTArray<nsRefPtr<IDBDatabase>, 1> array;
        if (!array.AppendElement(aDatabase)) {
          NS_ERROR("This should never fail!");
        }

        
        if (!pool->WaitForAllDatabasesToComplete(array, runnable)) {
          NS_WARNING("Failed to wait for transaction to complete!");
        }
      }
      break;
    }
  }
}


bool
IndexedDatabaseManager::SetCurrentDatabase(IDBDatabase* aDatabase)
{
  NS_ASSERTION(gCurrentDatabaseIndex != BAD_TLS_INDEX,
               "This should have been set already!");

#ifdef DEBUG
  if (aDatabase) {
    NS_ASSERTION(!PR_GetThreadPrivate(gCurrentDatabaseIndex),
                 "Someone forgot to unset gCurrentDatabaseIndex!");
  }
  else {
    NS_ASSERTION(PR_GetThreadPrivate(gCurrentDatabaseIndex),
                 "Someone forgot to set gCurrentDatabaseIndex!");
  }
#endif

  if (PR_SetThreadPrivate(gCurrentDatabaseIndex, aDatabase) != PR_SUCCESS) {
    NS_WARNING("Failed to set gCurrentDatabaseIndex!");
    return false;
  }

  return true;
}


PRUint32
IndexedDatabaseManager::GetIndexedDBQuotaMB()
{
  return PRUint32(NS_MAX(gIndexedDBQuotaMB, 0));
}

nsresult
IndexedDatabaseManager::EnsureQuotaManagementForDirectory(nsIFile* aDirectory)
{
#ifdef DEBUG
  {
    bool correctThread;
    NS_ASSERTION(NS_SUCCEEDED(mIOThread->IsOnCurrentThread(&correctThread)) &&
                 correctThread,
                 "Running on the wrong thread!");
  }
#endif
  NS_ASSERTION(aDirectory, "Null pointer!");

  nsCString path;
  nsresult rv = aDirectory->GetNativePath(path);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mTrackedQuotaPaths.Contains(path)) {
    return true;
  }

  
  nsCOMPtr<nsIFile> patternFile;
  rv = aDirectory->Clone(getter_AddRefs(patternFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = patternFile->Append(NS_LITERAL_STRING("*"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString pattern;
  rv = patternFile->GetNativePath(pattern);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<mozIStorageServiceQuotaManagement> ss =
    do_GetService(MOZ_STORAGE_SERVICE_CONTRACTID);
  NS_ENSURE_TRUE(ss, NS_ERROR_FAILURE);

  rv = ss->SetQuotaForFilenamePattern(pattern,
                                      GetIndexedDBQuotaMB() * 1024 * 1024,
                                      mQuotaCallbackSingleton, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  bool exists;
  rv = aDirectory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (exists) {
    
    bool isDirectory;
    rv = aDirectory->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(isDirectory, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = aDirectory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    bool hasMore;
    while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) && hasMore) {
      nsCOMPtr<nsISupports> entry;
      rv = entries->GetNext(getter_AddRefs(entry));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIFile> file = do_QueryInterface(entry);
      NS_ENSURE_TRUE(file, NS_NOINTERFACE);

      rv = ss->UpdateQutoaInformationForFile(file);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(!mTrackedQuotaPaths.Contains(path), "What?!");

  mTrackedQuotaPaths.AppendElement(path);
  return rv;
}

NS_IMPL_ISUPPORTS2(IndexedDatabaseManager, nsIIndexedDatabaseManager,
                                           nsIObserver)

NS_IMETHODIMP
IndexedDatabaseManager::GetUsageForURI(
                                     nsIURI* aURI,
                                     nsIIndexedDatabaseUsageCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aCallback);

  
  nsCString origin;
  nsresult rv = nsContentUtils::GetASCIIOrigin(aURI, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<AsyncUsageRunnable> runnable =
    new AsyncUsageRunnable(aURI, origin, aCallback);

  nsRefPtr<AsyncUsageRunnable>* newRunnable =
    mUsageRunnables.AppendElement(runnable);
  NS_ENSURE_TRUE(newRunnable, NS_ERROR_OUT_OF_MEMORY);

  
  
  if (origin.EqualsLiteral("null")) {
    rv = NS_DispatchToCurrentThread(runnable);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  
  
  for (PRUint32 index = 0; index < mOriginClearRunnables.Length(); index++) {
    if (mOriginClearRunnables[index]->mOrigin == origin) {
      rv = NS_DispatchToCurrentThread(runnable);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }

  
  rv = mIOThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
IndexedDatabaseManager::CancelGetUsageForURI(
                                     nsIURI* aURI,
                                     nsIIndexedDatabaseUsageCallback* aCallback)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(aCallback);

  
  
  for (PRUint32 index = 0; index < mUsageRunnables.Length(); index++) {
    nsRefPtr<AsyncUsageRunnable>& runnable = mUsageRunnables[index];

    bool equals;
    nsresult rv = runnable->mURI->Equals(aURI, &equals);
    NS_ENSURE_SUCCESS(rv, rv);

    if (equals && SameCOMIdentity(aCallback, runnable->mCallback)) {
      runnable->Cancel();
      break;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
IndexedDatabaseManager::ClearDatabasesForURI(nsIURI* aURI)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);

  
  nsCString origin;
  nsresult rv = nsContentUtils::GetASCIIOrigin(aURI, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (origin.EqualsLiteral("null")) {
    return NS_OK;
  }

  
  
  PRUint32 clearDataCount = mOriginClearRunnables.Length();
  for (PRUint32 index = 0; index < clearDataCount; index++) {
    if (mOriginClearRunnables[index]->mOrigin == origin) {
      return NS_OK;
    }
  }

  
  
  nsTArray<nsRefPtr<IDBDatabase> > liveDatabases;

  
  nsTArray<IDBDatabase*>* array;
  if (mLiveDatabases.Get(origin, &array)) {
    if (!liveDatabases.AppendElements(*array)) {
      NS_WARNING("Out of memory?");
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  nsRefPtr<OriginClearRunnable> runnable =
    new OriginClearRunnable(origin, mIOThread);

  
  nsRefPtr<OriginClearRunnable>* newRunnable =
    mOriginClearRunnables.AppendElement(runnable);
  NS_ENSURE_TRUE(newRunnable, NS_ERROR_OUT_OF_MEMORY);

  if (liveDatabases.IsEmpty()) {
    rv = runnable->Run();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  
  for (PRUint32 index = 0; index < liveDatabases.Length(); index++) {
    liveDatabases[index]->Invalidate();
  }

  
  TransactionThreadPool* pool = TransactionThreadPool::GetOrCreate();
  NS_ENSURE_TRUE(pool, NS_ERROR_FAILURE);

  if (!pool->WaitForAllDatabasesToComplete(liveDatabases, runnable)) {
    NS_WARNING("Can't wait on databases!");
    return NS_ERROR_FAILURE;
  }

  NS_ASSERTION(liveDatabases.IsEmpty(), "Should have swapped!");

  return NS_OK;
}

NS_IMETHODIMP
IndexedDatabaseManager::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID)) {
    
    
    if (PR_ATOMIC_SET(&gShutdown, 1)) {
      NS_ERROR("Shutdown more than once?!");
    }

    
    if (NS_FAILED(mIOThread->Shutdown())) {
      NS_WARNING("Failed to shutdown IO thread!");
    }

    
    if (NS_FAILED(mShutdownTimer->Init(this, DEFAULT_SHUTDOWN_TIMER_MS,
                                       nsITimer::TYPE_ONE_SHOT))) {
      NS_WARNING("Failed to initialize shutdown timer!");
    }

    
    
    TransactionThreadPool::Shutdown();

    
    if (NS_FAILED(mShutdownTimer->Cancel())) {
      NS_WARNING("Failed to cancel shutdown timer!");
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, NS_TIMER_CALLBACK_TOPIC)) {
    NS_WARNING("Some database operations are taking longer than expected "
               "during shutdown and will be aborted!");

    
    nsAutoTArray<IDBDatabase*, 50> liveDatabases;
    mLiveDatabases.EnumerateRead(EnumerateToTArray, &liveDatabases);

    
    if (!liveDatabases.IsEmpty()) {
      PRUint32 count = liveDatabases.Length();
      for (PRUint32 index = 0; index < count; index++) {
        liveDatabases[index]->Invalidate();
      }
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    
    return NS_OK;
  }

  NS_NOTREACHED("Unknown topic!");
  return NS_ERROR_UNEXPECTED;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(IndexedDatabaseManager::OriginClearRunnable,
                              nsIRunnable)

NS_IMETHODIMP
IndexedDatabaseManager::OriginClearRunnable::Run()
{
  if (NS_IsMainThread()) {
    
    if (mFirstCallback) {
      NS_ASSERTION(mThread, "Should have a thread here!");

      mFirstCallback = false;

      nsCOMPtr<nsIThread> thread;
      mThread.swap(thread);

      
      if (NS_FAILED(thread->Dispatch(this, NS_DISPATCH_NORMAL))) {
        NS_WARNING("Failed to dispatch to IO thread!");
        return NS_ERROR_FAILURE;
      }

      return NS_OK;
    }

    NS_ASSERTION(!mThread, "Should have been cleared already!");

    
    for (PRUint32 index = 0; index < mDelayedRunnables.Length(); index++) {
      if (NS_FAILED(NS_DispatchToCurrentThread(mDelayedRunnables[index]))) {
        NS_WARNING("Failed to dispatch delayed runnable!");
      }
    }
    mDelayedRunnables.Clear();

    
    IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
    if (mgr) {
      mgr->OnOriginClearComplete(this);
    }

    return NS_OK;
  }

  NS_ASSERTION(!mThread, "Should have been cleared already!");

  
  nsCOMPtr<nsIFile> directory;
  nsresult rv = IDBFactory::GetDirectoryForOrigin(mOrigin,
                                                  getter_AddRefs(directory));
  if (NS_SUCCEEDED(rv)) {
    bool exists;
    rv = directory->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists) {
      rv = directory->Remove(true);
    }
  }
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to remove directory!");

  
  rv = NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

IndexedDatabaseManager::AsyncUsageRunnable::AsyncUsageRunnable(
                                     nsIURI* aURI,
                                     const nsACString& aOrigin,
                                     nsIIndexedDatabaseUsageCallback* aCallback)
: mURI(aURI),
  mOrigin(aOrigin),
  mCallback(aCallback),
  mUsage(0),
  mCanceled(0)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aURI, "Null pointer!");
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");
  NS_ASSERTION(aCallback, "Null pointer!");
}

void
IndexedDatabaseManager::AsyncUsageRunnable::Cancel()
{
  if (PR_ATOMIC_SET(&mCanceled, 1)) {
    NS_ERROR("Canceled more than once?!");
  }
}

nsresult
IndexedDatabaseManager::AsyncUsageRunnable::RunInternal()
{
  if (NS_IsMainThread()) {
    
    if (!mCanceled) {
      mCallback->OnUsageResult(mURI, mUsage);
    }

    
    mURI = nsnull;
    mCallback = nsnull;

    
    IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
    if (mgr) {
      mgr->OnUsageCheckComplete(this);
    }

    return NS_OK;
  }

  if (mCanceled) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIFile> directory;
  nsresult rv = IDBFactory::GetDirectoryForOrigin(mOrigin,
                                                  getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists;
  rv = directory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (exists && !mCanceled) {
    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    if (entries) {
      bool hasMore;
      while (NS_SUCCEEDED((rv = entries->HasMoreElements(&hasMore))) &&
             hasMore && !mCanceled) {
        nsCOMPtr<nsISupports> entry;
        rv = entries->GetNext(getter_AddRefs(entry));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIFile> file(do_QueryInterface(entry));
        NS_ASSERTION(file, "Don't know what this is!");

        PRInt64 fileSize;
        rv = file->GetFileSize(&fileSize);
        NS_ENSURE_SUCCESS(rv, rv);

        NS_ASSERTION(fileSize > 0, "Negative size?!");

        
        if (NS_UNLIKELY((LL_MAXINT - mUsage) <= PRUint64(fileSize))) {
          NS_WARNING("Database sizes exceed max we can report!");
          mUsage = LL_MAXINT;
        }
        else {
          mUsage += fileSize;
        }
      }
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(IndexedDatabaseManager::AsyncUsageRunnable,
                              nsIRunnable)

NS_IMETHODIMP
IndexedDatabaseManager::AsyncUsageRunnable::Run()
{
  nsresult rv = RunInternal();

  if (!NS_IsMainThread()) {
    if (NS_FAILED(rv)) {
      mUsage = 0;
    }

    if (NS_FAILED(NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Failed to dispatch to main thread!");
    }
  }

  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

IndexedDatabaseManager::SetVersionRunnable::SetVersionRunnable(
                                   IDBDatabase* aDatabase,
                                   nsTArray<nsRefPtr<IDBDatabase> >& aDatabases)
: mRequestingDatabase(aDatabase)
{
  NS_ASSERTION(aDatabase, "Null database!");
  if (!mDatabases.SwapElements(aDatabases)) {
    NS_ERROR("This should never fail!");
  }
}

IndexedDatabaseManager::SetVersionRunnable::~SetVersionRunnable()
{
}

NS_IMPL_ISUPPORTS1(IndexedDatabaseManager::SetVersionRunnable, nsIRunnable)

NS_IMETHODIMP
IndexedDatabaseManager::SetVersionRunnable::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mHelper, "Should have been cleared already!");

  
  
  for (PRUint32 index = 0; index < mDelayedRunnables.Length(); index++) {
    if (NS_FAILED(NS_DispatchToCurrentThread(mDelayedRunnables[index]))) {
      NS_WARNING("Failed to dispatch delayed runnable!");
    }
  }

  
  mDelayedRunnables.Clear();

  IndexedDatabaseManager* mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "This should never be null!");

  
  
  mgr->OnSetVersionRunnableComplete(this);

  return NS_OK;
}
