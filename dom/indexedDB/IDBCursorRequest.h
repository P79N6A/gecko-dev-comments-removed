






































#ifndef mozilla_dom_indexeddb_idbcursorrequest_h__
#define mozilla_dom_indexeddb_idbcursorrequest_h__

#include "mozilla/dom/indexedDB/IDBObjectStoreRequest.h"
#include "nsIIDBCursorRequest.h"

#include "jsapi.h"

class nsIRunnable;

BEGIN_INDEXEDDB_NAMESPACE

class IDBIndexRequest;
class IDBRequest;
class IDBObjectStoreRequest;
class IDBTransactionRequest;

struct KeyValuePair
{
  Key key;
  nsString value;
};

struct KeyKeyPair
{
  Key key;
  Key value;
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

  static
  already_AddRefed<IDBCursorRequest>
  Create(IDBRequest* aRequest,
         IDBTransactionRequest* aTransaction,
         IDBIndexRequest* aIndex,
         PRUint16 aDirection,
         nsTArray<KeyKeyPair>& aData);

  static
  already_AddRefed<IDBCursorRequest>
  Create(IDBRequest* aRequest,
         IDBTransactionRequest* aTransaction,
         IDBIndexRequest* aIndex,
         PRUint16 aDirection,
         nsTArray<KeyValuePair>& aData);

  enum Type
  {
    OBJECTSTORE = 0,
    INDEX,
    INDEXOBJECT
  };

protected:
  IDBCursorRequest();
  ~IDBCursorRequest();

  static
  already_AddRefed<IDBCursorRequest>
  CreateCommon(IDBRequest* aRequest,
               IDBTransactionRequest* aTransaction,
               PRUint16 aDirection);

  nsRefPtr<IDBRequest> mRequest;
  nsRefPtr<IDBTransactionRequest> mTransaction;
  nsRefPtr<IDBObjectStoreRequest> mObjectStore;
  nsRefPtr<IDBIndexRequest> mIndex;

  PRUint16 mDirection;

  nsCOMPtr<nsIVariant> mCachedKey;
  jsval mCachedValue;
  bool mHaveCachedValue;
  JSRuntime* mJSRuntime;

  bool mContinueCalled;
  PRUint32 mDataIndex;

  Type mType;
  nsTArray<KeyValuePair> mData;
  nsTArray<KeyKeyPair> mKeyData;
};

END_INDEXEDDB_NAMESPACE

#endif 
