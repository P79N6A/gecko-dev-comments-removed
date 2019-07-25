






































#ifndef mozilla_dom_indexeddb_idbobjectstore_h__
#define mozilla_dom_indexeddb_idbobjectstore_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"
#include "mozilla/dom/indexedDB/IDBTransaction.h"

#include "nsIIDBObjectStore.h"
#include "nsIIDBTransaction.h"

#include "nsCycleCollectionParticipant.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class Key;

struct ObjectStoreInfo;
struct IndexInfo;
struct IndexUpdateInfo;
struct StructuredCloneReadInfo;
struct StructuredCloneWriteInfo;

class IDBObjectStore : public nsIIDBObjectStore
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIIDBOBJECTSTORE

  NS_DECL_CYCLE_COLLECTION_CLASS(IDBObjectStore)

  static already_AddRefed<IDBObjectStore>
  Create(IDBTransaction* aTransaction,
         const ObjectStoreInfo* aInfo);

  static bool
  IsValidKeyPath(JSContext* aCx, const nsAString& aKeyPath);

  static nsresult
  AppendIndexUpdateInfo(PRInt64 aIndexID,
                        const nsAString& aKeyPath,
                        bool aUnique,
                        bool aMultiEntry,
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
  GetStructuredCloneReadInfoFromStatement(mozIStorageStatement* aStatement,
                                          PRUint32 aDataIndex,
                                          PRUint32 aFileIdsIndex,
                                          FileManager* aFileManager,
                                          StructuredCloneReadInfo& aInfo);

  static void
  ClearStructuredCloneBuffer(JSAutoStructuredCloneBuffer& aBuffer);

  static bool
  DeserializeValue(JSContext* aCx,
                   StructuredCloneReadInfo& aCloneReadInfo,
                   jsval* aValue);

  static bool
  SerializeValue(JSContext* aCx,
                 StructuredCloneWriteInfo& aCloneWriteInfo,
                 jsval aValue);

  static JSObject*
  StructuredCloneReadCallback(JSContext* aCx,
                              JSStructuredCloneReader* aReader,
                              uint32_t aTag,
                              uint32_t aData,
                              void* aClosure);
  static JSBool
  StructuredCloneWriteCallback(JSContext* aCx,
                               JSStructuredCloneWriter* aWriter,
                               JSObject* aObj,
                               void* aClosure);

  static nsresult
  ConvertFileIdsToArray(const nsAString& aFileIds,
                        nsTArray<PRInt64>& aResult);

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

  const bool HasKeyPath() const
  {
    return !mKeyPath.IsVoid();
  }

  IDBTransaction* Transaction()
  {
    return mTransaction;
  }

  nsresult ModifyValueForNewKey(StructuredCloneWriteInfo& aCloneWriteInfo,
                                Key& aKey);

protected:
  IDBObjectStore();
  ~IDBObjectStore();

  nsresult GetAddInfo(JSContext* aCx,
                      jsval aValue,
                      jsval aKeyVal,
                      StructuredCloneWriteInfo& aCloneWriteInfo,
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
  bool mAutoIncrement;
  nsCOMPtr<nsIAtom> mDatabaseId;
  PRUint32 mStructuredCloneVersion;

  nsTArray<nsRefPtr<IDBIndex> > mCreatedIndexes;
};

END_INDEXEDDB_NAMESPACE

#endif 
