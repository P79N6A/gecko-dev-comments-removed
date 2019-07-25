






































#ifndef mozilla_dom_indexeddb_idbfactory_h__
#define mozilla_dom_indexeddb_idbfactory_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "mozIStorageConnection.h"
#include "nsIIDBFactory.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBDatabase;

class IDBFactory : public nsIIDBFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBFACTORY

  static already_AddRefed<nsIIDBFactory> Create();

  static already_AddRefed<mozIStorageConnection>
  GetConnection(const nsAString& aDatabaseFilePath);

  static bool
  SetCurrentDatabase(IDBDatabase* aDatabase);

  static PRUint32
  GetIndexedDBQuota();

private:
  IDBFactory() { }
  ~IDBFactory() { }
};

END_INDEXEDDB_NAMESPACE

#endif 
