






































#ifndef mozilla_dom_indexeddb_idbfactory_h__
#define mozilla_dom_indexeddb_idbfactory_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"

#include "mozIStorageConnection.h"
#include "nsIIDBFactory.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBFactory : public IDBRequest::Generator,
                   public nsIIDBFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBFACTORY

  static
  already_AddRefed<nsIIDBFactory>
  Create();

  static
  already_AddRefed<mozIStorageConnection>
  GetConnection(const nsAString& aDatabaseFilePath);

protected:
  
  IDBFactory() { }
};

END_INDEXEDDB_NAMESPACE

#endif 
