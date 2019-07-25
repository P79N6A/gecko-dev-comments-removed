






































#ifndef mozilla_dom_indexeddb_idbobjectstore_h__
#define mozilla_dom_indexeddb_idbobjectstore_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBTransaction.h"
#include "mozilla/dom/indexedDB/Key.h"

#include "nsIIDBObjectStore.h"
#include "nsIIDBTransaction.h"

#include "nsCycleCollectionParticipant.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;

struct ObjectStoreInfo;
struct IndexInfo;
struct IndexUpdateInfo;

class IDBObjectStore : public nsIIDBObjectStore
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIIDBOBJECTSTORE

  NS_DECL_CYCLE_COLLECTION_CLASS(IDBObjectStore)

  static already_AddRefed<IDBObjectStore>
  Create(IDBTransaction* aTransaction,
         const ObjectStoreInfo* aInfo);

  static nsresult
  GetKeyFromVariant(nsIVariant* aKeyVariant,
                    Key& aKey);

  static nsresult
  GetKeyFromJSVal(jsval aKeyVal,
                  JSContext* aCx,
                  Key& aKey);

  static nsresult
  GetJSValFromKey(const Key& aKey,
                  JSContext* aCx,
                  jsval* aKeyVal);

  static nsresult
  GetKeyPathValueFromStructuredData(const PRUint8* aData,
                                    PRUint32 aDataLength,
                                    const nsAString& aKeyPath,
                                    JSContext** aCx,
                                    Key& aValue);

  static nsresult
  GetIndexUpdateInfo(ObjectStoreInfo* aObjectStoreInfo,
                     JSContext* aCx,
                     jsval aObject,
                     nsTArray<IndexUpdateInfo>& aUpdateInfoArray);

  static nsresult
  UpdateIndexes(IDBTransaction* aTransaction,
                PRInt64 aObjectStoreId,
                const Key& aObjectStoreKey,
                bool aAutoIncrement,
                bool aOverwrite,
                PRInt64 aObjectDataId,
                const nsTArray<IndexUpdateInfo>& aUpdateInfoArray);

  static nsresult
  GetStructuredCloneDataFromStatement(mozIStorageStatement* aStatement,
                                      PRUint32 aIndex,
                                      JSAutoStructuredCloneBuffer& aBuffer);

  static void
  ClearStructuredCloneBuffer(JSAutoStructuredCloneBuffer& aBuffer);

  static bool
  DeserializeValue(JSContext* aCx,
                   JSAutoStructuredCloneBuffer& aBuffer,
                   jsval* aValue);

  static bool
  SerializeValue(JSContext* aCx,
                 JSAutoStructuredCloneBuffer& aBuffer,
                 jsval aValue);

  const nsString& Name() const
  {
    return mName;
  }

  bool IsAutoIncrement() const
  {
    return mAutoIncrement;
  }

  bool IsWriteAllowed() const
  {
    return mTransaction->IsWriteAllowed();
  }

  PRInt64 Id() const
  {
    NS_ASSERTION(mId != LL_MININT, "Don't ask for this yet!");
    return mId;
  }

  const nsString& KeyPath() const
  {
    return mKeyPath;
  }

  IDBTransaction* Transaction()
  {
    return mTransaction;
  }

protected:
  IDBObjectStore();
  ~IDBObjectStore();

  nsresult GetAddInfo(JSContext* aCx,
                      jsval aValue,
                      jsval aKeyVal,
                      JSAutoStructuredCloneBuffer& aCloneBuffer,
                      Key& aKey,
                      nsTArray<IndexUpdateInfo>& aUpdateInfoArray);

  nsresult AddOrPut(const jsval& aValue,
                    const jsval& aKey,
                    JSContext* aCx,
                    PRUint8 aOptionalArgCount,
                    nsIIDBRequest** _retval,
                    bool aOverwrite);

private:
  nsRefPtr<IDBTransaction> mTransaction;

  nsCOMPtr<nsIScriptContext> mScriptContext;
  nsCOMPtr<nsPIDOMWindow> mOwner;

  PRInt64 mId;
  nsString mName;
  nsString mKeyPath;
  PRBool mAutoIncrement;
  PRUint32 mDatabaseId;
  PRUint32 mStructuredCloneVersion;

  nsTArray<nsRefPtr<IDBIndex> > mCreatedIndexes;

};

END_INDEXEDDB_NAMESPACE

#endif 
