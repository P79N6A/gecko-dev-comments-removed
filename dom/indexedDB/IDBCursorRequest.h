






































#ifndef mozilla_dom_indexeddb_idbcursorrequest_h__
#define mozilla_dom_indexeddb_idbcursorrequest_h__

#include "mozilla/dom/indexedDB/IDBObjectStoreRequest.h"
#include "nsIIDBCursorRequest.h"

#include "jsapi.h"

class nsIRunnable;

BEGIN_INDEXEDDB_NAMESPACE

class IDBRequest;
class IDBObjectStoreRequest;
class IDBTransactionRequest;

struct KeyValuePair
{
  Key key;
  nsString value;
};

class ContinueRunnable;

class IDBCursorRequest : public IDBRequest::Generator,
                         public nsIIDBCursorRequest
{
  friend class ContinueRunnable;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBCURSOR
  NS_DECL_NSIIDBCURSORREQUEST

  static
  already_AddRefed<IDBCursorRequest>
  Create(IDBRequest* aRequest,
         IDBTransactionRequest* aTransaction,
         IDBObjectStoreRequest* aObjectStore,
         PRUint16 aDirection,
         nsTArray<KeyValuePair>& aData);

  
protected:
  IDBCursorRequest();
  ~IDBCursorRequest();

  nsRefPtr<IDBRequest> mRequest;
  nsRefPtr<IDBTransactionRequest> mTransaction;
  nsRefPtr<IDBObjectStoreRequest> mObjectStore;

  PRUint16 mDirection;

  nsCOMPtr<nsIVariant> mCachedKey;
  jsval mCachedValue;
  bool mHaveCachedValue;
  JSRuntime* mJSRuntime;

  PRUint32 mDataIndex;
  nsTArray<KeyValuePair> mData;
};

END_INDEXEDDB_NAMESPACE

#endif 
