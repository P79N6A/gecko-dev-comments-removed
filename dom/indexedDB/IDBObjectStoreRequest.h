






































#ifndef mozilla_dom_indexeddb_idbobjectstorerequest_h__
#define mozilla_dom_indexeddb_idbobjectstorerequest_h__

#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "mozilla/dom/indexedDB/IDBDatabaseRequest.h"

#include "nsIIDBObjectStoreRequest.h"

BEGIN_INDEXEDDB_NAMESPACE

class IDBObjectStoreRequest : public IDBRequest::Generator,
                              public nsIIDBObjectStoreRequest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBOBJECTSTORE
  NS_DECL_NSIIDBOBJECTSTOREREQUEST

  static already_AddRefed<IDBObjectStoreRequest>
  Create(IDBDatabaseRequest* aDatabase,
         const nsAString& aName,
         const nsAString& aKeyPath,
         PRBool aAutoIncrement,
         PRUint16 aMode);

  void SetId(PRInt64 aId) {
    mId = aId;
  }

protected:
  IDBObjectStoreRequest();
  ~IDBObjectStoreRequest();

private:
  nsRefPtr<IDBDatabaseRequest> mDatabase;

  nsString mName;
  nsString mKeyPath;
  PRBool mAutoIncrement;
  PRUint16 mMode;
  PRInt64 mId;
};

END_INDEXEDDB_NAMESPACE

#endif 
