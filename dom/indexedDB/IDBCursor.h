






































#ifndef mozilla_dom_indexeddb_idbcursor_h__
#define mozilla_dom_indexeddb_idbcursor_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBObjectStore.h"
#include "mozilla/dom/indexedDB/Key.h"

#include "nsIIDBCursorWithValue.h"

#include "nsCycleCollectionParticipant.h"

class nsIRunnable;
class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class IDBIndex;
class IDBRequest;
class IDBTransaction;

class ContinueHelper;
class ContinueObjectStoreHelper;
class ContinueIndexHelper;
class ContinueIndexObjectHelper;

class IDBCursor : public nsIIDBCursorWithValue
{
  friend class ContinueHelper;
  friend class ContinueObjectStoreHelper;
  friend class ContinueIndexHelper;
  friend class ContinueIndexObjectHelper;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIIDBCURSOR
  NS_DECL_NSIIDBCURSORWITHVALUE

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(IDBCursor)

  
  static
  already_AddRefed<IDBCursor>
  Create(IDBRequest* aRequest,
         IDBTransaction* aTransaction,
         IDBObjectStore* aObjectStore,
         PRUint16 aDirection,
         const Key& aRangeKey,
         const nsACString& aContinueQuery,
         const nsACString& aContinueToQuery,
         const Key& aKey,
         StructuredCloneReadInfo& aCloneReadInfo);

  
  static
  already_AddRefed<IDBCursor>
  Create(IDBRequest* aRequest,
         IDBTransaction* aTransaction,
         IDBIndex* aIndex,
         PRUint16 aDirection,
         const Key& aRangeKey,
         const nsACString& aContinueQuery,
         const nsACString& aContinueToQuery,
         const Key& aKey,
         const Key& aObjectKey);

  
  static
  already_AddRefed<IDBCursor>
  Create(IDBRequest* aRequest,
         IDBTransaction* aTransaction,
         IDBIndex* aIndex,
         PRUint16 aDirection,
         const Key& aRangeKey,
         const nsACString& aContinueQuery,
         const nsACString& aContinueToQuery,
         const Key& aKey,
         const Key& aObjectKey,
         StructuredCloneReadInfo& aCloneReadInfo);

  enum Type
  {
    OBJECTSTORE = 0,
    INDEXKEY,
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
               IDBObjectStore* aObjectStore,
               PRUint16 aDirection,
               const Key& aRangeKey,
               const nsACString& aContinueQuery,
               const nsACString& aContinueToQuery);

  nsresult
  ContinueInternal(const Key& aKey,
                   PRInt32 aCount);

  nsRefPtr<IDBRequest> mRequest;
  nsRefPtr<IDBTransaction> mTransaction;
  nsRefPtr<IDBObjectStore> mObjectStore;
  nsRefPtr<IDBIndex> mIndex;

  nsCOMPtr<nsIScriptContext> mScriptContext;
  nsCOMPtr<nsPIDOMWindow> mOwner;

  Type mType;
  PRUint16 mDirection;
  nsCString mContinueQuery;
  nsCString mContinueToQuery;

  
  jsval mCachedKey;
  jsval mCachedPrimaryKey;
  jsval mCachedValue;

  Key mRangeKey;

  Key mKey;
  Key mObjectKey;
  StructuredCloneReadInfo mCloneReadInfo;
  Key mContinueToKey;

  bool mHaveCachedKey;
  bool mHaveCachedPrimaryKey;
  bool mHaveCachedValue;
  bool mRooted;
  bool mContinueCalled;
  bool mHaveValue;
};

END_INDEXEDDB_NAMESPACE

#endif 
