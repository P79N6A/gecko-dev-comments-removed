






































#include "IndexedDatabaseManager.h"

#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsISimpleEnumerator.h"
#include "nsITimer.h"

#include "mozilla/Services.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"
#include "nsXPCOMPrivate.h"

#include "IDBDatabase.h"
#include "IDBFactory.h"
#include "LazyIdleThread.h"
#include "TransactionThreadPool.h"



#define DEFAULT_THREAD_TIMEOUT_MS 30000



#define DEFAULT_SHUTDOWN_TIMER_MS 30000

USING_INDEXEDDB_NAMESPACE
using namespace mozilla::services;

namespace {

bool gShutdown = false;


IndexedDatabaseManager* gInstance = nsnull;


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

  if (gShutdown) {
    NS_ERROR("Calling GetOrCreateInstance() after shutdown!");
    return nsnull;
  }

  nsRefPtr<IndexedDatabaseManager> instance(gInstance);

  if (!instance) {
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
                                   PR_FALSE);
    NS_ENSURE_SUCCESS(rv, nsnull);

    
    
    
    rv = obs->AddObserver(instance, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID,
                          PR_FALSE);
    NS_ENSURE_SUCCESS(rv, nsnull);

    
    
    instance->mIOThread = new LazyIdleThread(DEFAULT_THREAD_TIMEOUT_MS);

    
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

  
  if (gShutdown) {
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
  NS_ASSERTION(aRunnable->mDatabasesWaiting.IsEmpty(), "Databases waiting?!");
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



nsresult
IndexedDatabaseManager::WaitForClearAndDispatch(const nsACString& aOrigin,
                                                nsIRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
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

  
  
  return NS_DispatchToCurrentThread(aRunnable);
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

    PRBool equals;
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
    new OriginClearRunnable(origin, mIOThread, liveDatabases);

  NS_ASSERTION(liveDatabases.IsEmpty(), "Should have swapped!");

  
  nsRefPtr<OriginClearRunnable>* newRunnable =
    mOriginClearRunnables.AppendElement(runnable);
  NS_ENSURE_TRUE(newRunnable, NS_ERROR_OUT_OF_MEMORY);

  if (!runnable->mDatabasesWaiting.IsEmpty()) {
    PRUint32 count = runnable->mDatabasesWaiting.Length();

    
    for (PRUint32 index = 0; index < count; index++) {
      runnable->mDatabasesWaiting[index]->Invalidate();
    }

    
    TransactionThreadPool* pool = TransactionThreadPool::Get();
    for (PRUint32 index = 0; index < count; index++) {
      if (!pool->WaitForAllTransactionsToComplete(
                                  runnable->mDatabasesWaiting[index],
                                  OriginClearRunnable::DatabaseCompleteCallback,
                                  runnable)) {
        NS_WARNING("Out of memory!");
        return NS_ERROR_OUT_OF_MEMORY;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
IndexedDatabaseManager::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID)) {
    
    
    gShutdown = true;

    
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



void
IndexedDatabaseManager::OriginClearRunnable::OnDatabaseComplete(
                                                         IDBDatabase* aDatabase)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aDatabase, "Null pointer!");
  NS_ASSERTION(mThread, "This shouldn't be cleared yet!");

  
  if (!mDatabasesWaiting.RemoveElement(aDatabase)) {
    NS_ERROR("Don't know anything about this database!");
  }

  
  if (mDatabasesWaiting.IsEmpty()) {
    if (NS_FAILED(mThread->Dispatch(this, NS_DISPATCH_NORMAL))) {
      NS_WARNING("Can't dispatch to IO thread!");
    }

    
    mThread = nsnull;
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(IndexedDatabaseManager::OriginClearRunnable,
                              nsIRunnable)







NS_IMETHODIMP
IndexedDatabaseManager::OriginClearRunnable::Run()
{
  if (NS_IsMainThread()) {
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

  
  nsCOMPtr<nsIFile> directory;
  nsresult rv = IDBFactory::GetDirectoryForOrigin(mOrigin,
                                                  getter_AddRefs(directory));
  if (NS_SUCCEEDED(rv)) {
    PRBool exists;
    rv = directory->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists) {
      rv = directory->Remove(PR_TRUE);
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
  if (PR_AtomicSet(&mCanceled, 1)) {
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

  PRBool exists;
  rv = directory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (exists && !mCanceled) {
    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    if (entries) {
      PRBool hasMore;
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
