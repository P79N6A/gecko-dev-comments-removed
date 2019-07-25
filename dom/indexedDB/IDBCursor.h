






































#ifndef mozilla_dom_indexeddb_idbcursor_h__
#define mozilla_dom_indexeddb_idbcursor_h__

#include "mozilla/dom/indexedDB/IDBObjectStore.h"
#include "nsIIDBCursor.h"

class nsIRunnable;

BEGIN_INDEXEDDB_NAMESPACE

class IDBIndex;
class IDBRequest;
class IDBObjectStore;
class IDBTransaction;

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

class IDBCursor : public IDBRequest::Generator,
                  public nsIIDBCursor
{
  friend class ContinueRunnable;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBCURSOR

  static
  already_AddRefed<IDBCursor>
  Create(IDBRequest* aRequest,
         IDBTransaction* aTransaction,
         IDBObjectStore* aObjectStore,
         PRUint16 aDirection,
         nsTArray<KeyValuePair>& aData);

  static
  already_AddRefed<IDBCursor>
  Create(IDBRequest* aRequest,
         IDBTransaction* aTransaction,
         IDBIndex* aIndex,
         PRUint16 aDirection,
         nsTArray<KeyKeyPair>& aData);

  static
  already_AddRefed<IDBCursor>
  Create(IDBRequest* aRequest,
         IDBTransaction* aTransaction,
         IDBIndex* aIndex,
         PRUint16 aDirection,
         nsTArray<KeyValuePair>& aData);

  enum Type
  {
    OBJECTSTORE = 0,
    INDEX,
    INDEXOBJECT
  };

protected:
  IDBCursor();
  ~IDBCursor();

  static
  already_AddRefed<IDBCursor>
  CreateCommon(IDBRequest* aRequest,
               IDBTransaction* aTransaction,
               PRUint16 aDirection);

  nsRefPtr<IDBRequest> mRequest;
  nsRefPtr<IDBTransaction> mTransaction;
  nsRefPtr<IDBObjectStore> mObjectStore;
  nsRefPtr<IDBIndex> mIndex;

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
