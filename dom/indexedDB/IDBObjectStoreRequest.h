






































#ifndef mozilla_dom_indexeddb_idbdatabaserequest_h__
#define mozilla_dom_indexeddb_idbdatabaserequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBObjectStoreRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBObjectStoreRequest : public IDBRequest::Generator,
                              public nsIIDBObjectStoreRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBOBJECTSTORE
  NS_DECL_NSIIDBOBJECTSTOREREQUEST

  static already_AddRefed<nsIIDBObjectStoreRequest>
  Create(const nsAString& aName,
         const nsAString& aKeyPath,
         PRBool aAutoIncrement,
         PRUint16 aMode);

protected:
  IDBObjectStoreRequest();
  ~IDBObjectStoreRequest();

private:
  nsAutoPtr<AsyncDatabaseConnection> mDatabase;
};

END_INDEXEDDB_NAMESPACE

#endif 
