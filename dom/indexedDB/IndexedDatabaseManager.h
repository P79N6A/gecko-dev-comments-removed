






































#ifndef mozilla_dom_indexeddb_indexeddatabasemanager_h__
#define mozilla_dom_indexeddb_indexeddatabasemanager_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIndexedDatabaseManager.h"
#include "nsIObserver.h"

#include "nsClassHashtable.h"
#include "nsHashKeys.h"

#define INDEXEDDB_MANAGER_CONTRACTID \
  "@mozilla.org/dom/indexeddb/manager;1"

class nsIRunnable;

BEGIN_INDEXEDDB_NAMESPACE

class IDBDatabase;

class IndexedDatabaseManager : public nsIIndexedDatabaseManager,
                               public nsIObserver
{
  friend class IDBDatabase;

public:
  
  static IndexedDatabaseManager* GetOrCreateInstance();

  
  static IndexedDatabaseManager* GetInstance();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINDEXEDDATABASEMANAGER
  NS_DECL_NSIOBSERVER

  nsresult WaitForClearAndDispatch(const nsACString& aOrigin,
                                   nsIRunnable* aRunnable);

private:
  IndexedDatabaseManager();
  ~IndexedDatabaseManager();

  bool RegisterDatabase(IDBDatabase* aDatabase);
  void UnregisterDatabase(IDBDatabase* aDatabase);

  struct OriginClearData
  {
    nsCString origin;
    nsTArray<nsCOMPtr<nsIRunnable> > delayedRunnables;
  };

  
  nsClassHashtable<nsCStringHashKey, nsTArray<IDBDatabase*> > mLiveDatabases;

  
  nsAutoTArray<OriginClearData, 1> mOriginClearData;
};

END_INDEXEDDB_NAMESPACE

#endif 
