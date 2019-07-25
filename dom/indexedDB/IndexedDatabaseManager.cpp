






































#include "IndexedDatabaseManager.h"

#include "nsIFile.h"
#include "nsIObserverService.h"
#include "nsISimpleEnumerator.h"

#include "mozilla/Services.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "nsXPCOM.h"

#include "IDBDatabase.h"
#include "IDBFactory.h"

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

  nsTArray<nsRefPtr<IDBDatabase> >* array =
    static_cast<nsTArray<nsRefPtr<IDBDatabase> >* >(aUserArg);

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



IndexedDatabaseManager*
IndexedDatabaseManager::GetOrCreateInstance()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (gShutdown) {
    NS_ERROR("Calling GetOrCreateInstance() after shutdown!");
    return nsnull;
  }

  if (!gInstance) {
    nsRefPtr<IndexedDatabaseManager> instance(new IndexedDatabaseManager());

    if (!instance->mLiveDatabases.Init()) {
      NS_WARNING("Out of memory!");
      return nsnull;
    }

    
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    nsresult rv = obs->AddObserver(instance, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                   PR_FALSE);
    NS_ENSURE_SUCCESS(rv, nsnull);

    instance.forget(&gInstance);
  }

  NS_IF_ADDREF(gInstance);
  return gInstance;
}


IndexedDatabaseManager*
IndexedDatabaseManager::GetInstance()
{
  return gInstance;
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

nsresult
IndexedDatabaseManager::WaitForClearAndDispatch(const nsACString& aOrigin,
                                                nsIRunnable* aRunnable)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!aOrigin.IsEmpty(), "Empty origin!");
  NS_ASSERTION(aRunnable, "Null pointer!");

  
  
  PRUint32 count = mOriginClearData.Length();
  for (PRUint32 index = 0; index < count; index++) {
    OriginClearData& data = mOriginClearData[index];
    if (data.origin == aOrigin) {
      nsCOMPtr<nsIRunnable>* newPtr =
        data.delayedRunnables.AppendElement(aRunnable);
      NS_ENSURE_TRUE(newPtr, NS_ERROR_OUT_OF_MEMORY);

      return NS_OK;
    }
  }

  
  
  return NS_DispatchToCurrentThread(aRunnable);
}

NS_IMPL_ISUPPORTS2(IndexedDatabaseManager, nsIIndexedDatabaseManager,
                                           nsIObserver)

NS_IMETHODIMP
IndexedDatabaseManager::GetUsageForURI(nsIURI* aURI,
                                       PRUint64* _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ENSURE_ARG_POINTER(aURI);

  
  nsCString origin;
  nsresult rv = nsContentUtils::GetASCIIOrigin(aURI, origin);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (origin.EqualsLiteral("null")) {
    *_retval = 0;
    return NS_OK;
  }

  
  nsCOMPtr<nsIFile> directory;
  rv = IDBFactory::GetDirectoryForOrigin(origin, getter_AddRefs(directory));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists;
  rv = directory->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint64 usage = 0;

  
  
  if (exists) {
    nsCOMPtr<nsISimpleEnumerator> entries;
    rv = directory->GetDirectoryEntries(getter_AddRefs(entries));
    NS_ENSURE_SUCCESS(rv, rv);

    if (entries) {
      PRBool hasMore;
      while (NS_SUCCEEDED(entries->HasMoreElements(&hasMore)) && hasMore) {
        nsCOMPtr<nsISupports> entry;
        rv = entries->GetNext(getter_AddRefs(entry));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIFile> file(do_QueryInterface(entry));
        NS_ASSERTION(file, "Don't know what this is!");

        PRInt64 fileSize;
        rv = file->GetFileSize(&fileSize);
        NS_ENSURE_SUCCESS(rv, rv);

        NS_ASSERTION(fileSize > 0, "Negative size?!");

        
        if (NS_UNLIKELY((LL_MAXINT - usage) <= PRUint64(fileSize))) {
          NS_WARNING("Database sizes exceed max we can report!");
          usage = LL_MAXINT;
        }
        else {
          usage += fileSize;
        }
      }
    }
  }

  *_retval = usage;
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

  
  
  PRUint32 clearDataCount = mOriginClearData.Length();
  for (PRUint32 index = 0; index < clearDataCount; index++) {
    if (mOriginClearData[index].origin == origin) {
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

  
  
  OriginClearData* data = mOriginClearData.AppendElement();
  NS_ENSURE_TRUE(data, NS_ERROR_OUT_OF_MEMORY);

  data->origin = origin;

  if (!liveDatabases.IsEmpty()) {
    PRUint32 count = liveDatabases.Length();

    
    for (PRUint32 index = 0; index < count; index++) {
      liveDatabases[index]->Invalidate();
    }

    
    for (PRUint32 index = 0; index < count; index++) {
      liveDatabases[index]->WaitForConnectionReleased();
    }
  }

  
  
  
  nsCOMPtr<nsIFile> directory;
  rv = IDBFactory::GetDirectoryForOrigin(origin, getter_AddRefs(directory));
  if (NS_SUCCEEDED(rv)) {
    PRBool exists;
    rv = directory->Exists(&exists);
    if (NS_SUCCEEDED(rv) && exists) {
      rv = directory->Remove(PR_TRUE);
    }
  }

  
  
  clearDataCount = mOriginClearData.Length();
  for (PRUint32 clearDataIndex = 0; clearDataIndex < clearDataCount;
       clearDataIndex++) {
    OriginClearData& data = mOriginClearData[clearDataIndex];
    if (data.origin == origin) {
      nsTArray<nsCOMPtr<nsIRunnable> >& runnables = data.delayedRunnables;
      PRUint32 runnableCount = runnables.Length();
      for (PRUint32 runnableIndex = 0; runnableIndex < runnableCount;
           runnableIndex++) {
        NS_DispatchToCurrentThread(runnables[runnableIndex]);
      }
      mOriginClearData.RemoveElementAt(clearDataIndex);
      break;
    }
  }

  
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
IndexedDatabaseManager::Observe(nsISupports* aSubject,
                                const char* aTopic,
                                const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    
    
    gShutdown = true;

    
    
    nsAutoTArray<nsRefPtr<IDBDatabase>, 50> liveDatabases;
    mLiveDatabases.EnumerateRead(EnumerateToTArray, &liveDatabases);

    
    if (!liveDatabases.IsEmpty()) {
      PRUint32 count = liveDatabases.Length();
      for (PRUint32 index = 0; index < count; index++) {
        liveDatabases[index]->WaitForConnectionReleased();
      }
    }

    mLiveDatabases.Clear();

    
    gInstance->Release();
    return NS_OK;
  }

  NS_NOTREACHED("Unknown topic!");
  return NS_ERROR_UNEXPECTED;
}
