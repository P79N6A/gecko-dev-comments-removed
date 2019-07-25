






































#ifndef mozilla_dom_indexeddb_idbcursor_h__
#define mozilla_dom_indexeddb_idbcursor_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBObjectStore.h"

#include "nsIIDBCursor.h"

#include "nsCycleCollectionParticipant.h"

class nsIRunnable;
class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class IDBIndex;
class IDBRequest;
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

class IDBCursor : public nsIIDBCursor
{
  friend class ContinueRunnable;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIIDBCURSOR

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IDBCursor)

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

  IDBTransaction* Transaction()
  {
    return mTransaction;
  }

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

  nsCOMPtr<nsIScriptContext> mScriptContext;
  nsCOMPtr<nsPIDOMWindow> mOwner;

  PRUint16 mDirection;

  nsCOMPtr<nsIVariant> mCachedKey;
  jsval mCachedValue;
  bool mHaveCachedValue;
  bool mValueRooted;

  bool mContinueCalled;
  PRUint32 mDataIndex;

  Type mType;
  nsTArray<KeyValuePair> mData;
  nsTArray<KeyKeyPair> mKeyData;
};

END_INDEXEDDB_NAMESPACE

#endif 
