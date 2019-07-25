






































#include "IDBObjectStoreRequest.h"
#include "AsyncConnectionHelper.h"
#include "nsIIDBDatabaseError.h"

#include "nsDOMClassInfo.h"
#include "nsThreadUtils.h"
#include "mozilla/Storage.h"

USING_INDEXEDDB_NAMESPACE

namespace {

class PutObjectStoreHelper : public AsyncConnectionHelper
{
public:
  PutObjectStoreHelper(IDBDatabaseRequest* aDatabase,
                       nsIDOMEventTarget* aTarget,
                       PRInt64 aObjectStoreID,
                       nsIVariant* aValue,
                       nsIVariant* aKey,
                       bool aAutoIncrement,
                       bool aNoOverwrite)
  : AsyncConnectionHelper(aDatabase, aTarget), mOSID(aObjectStoreID),
    mValue(aValue), mKey(aKey), mAutoIncrement(aAutoIncrement),
    mNoOverwrite(aNoOverwrite)
  { }

  nsresult Init();
  PRUint16 DoDatabaseWork();
  void GetSuccessResult(nsIWritableVariant* aResult);

private:
  const PRInt64 mOSID;
  nsCOMPtr<nsIVariant> mValue;
  nsCOMPtr<nsIVariant> mKey;
  const bool mAutoIncrement;
  const bool mNoOverwrite;
};

} 


already_AddRefed<IDBObjectStoreRequest>
IDBObjectStoreRequest::Create(IDBDatabaseRequest* aDatabase,
                              const nsAString& aName,
                              const nsAString& aKeyPath,
                              PRBool aAutoIncrement,
                              PRUint16 aMode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<IDBObjectStoreRequest> objectStore = new IDBObjectStoreRequest();

  objectStore->mDatabase = aDatabase;
  objectStore->mName.Assign(aName);
  if (aKeyPath.IsVoid()) {
    objectStore->mKeyPath.SetIsVoid(PR_TRUE);
  }
  else {
    objectStore->mKeyPath.Assign(aKeyPath);
  }
  objectStore->mAutoIncrement = aAutoIncrement;
  objectStore->mMode = aMode;

  return objectStore.forget();
}


already_AddRefed<IDBObjectStoreRequest>
IDBObjectStoreRequest::Create(IDBDatabaseRequest* aDatabase,
                              const nsAString& aName,
                              PRUint16 aMode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<IDBObjectStoreRequest> objectStore = new IDBObjectStoreRequest();

  objectStore->mDatabase = aDatabase;
  objectStore->mName.Assign(aName);
  objectStore->mKeyPath.SetIsVoid(PR_TRUE);
  objectStore->mMode = aMode;

  return objectStore.forget();
}

IDBObjectStoreRequest::IDBObjectStoreRequest()
: mAutoIncrement(PR_FALSE),
  mMode(nsIIDBObjectStore::READ_WRITE),
  mId(LL_MININT)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

IDBObjectStoreRequest::~IDBObjectStoreRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

void
IDBObjectStoreRequest::SetId(PRInt64 aId)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mId == LL_MININT, "Overwriting mId!");
  mId = aId;
}

void
IDBObjectStoreRequest::SetKeyPath(const nsAString& aKeyPath)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mKeyPath.IsVoid(), "Overwriting mKeyPath!");
  if (!aKeyPath.IsVoid()) {
    mKeyPath.Assign(aKeyPath);
  }
}

void
IDBObjectStoreRequest::SetAutoIncrement(PRBool aAutoIncrement)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mAutoIncrement = aAutoIncrement;
}

NS_IMPL_ADDREF(IDBObjectStoreRequest)
NS_IMPL_RELEASE(IDBObjectStoreRequest)

NS_INTERFACE_MAP_BEGIN(IDBObjectStoreRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIIDBObjectStoreRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBObjectStoreRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBObjectStore)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBObjectStoreRequest)
NS_INTERFACE_MAP_END

DOMCI_DATA(IDBObjectStoreRequest, IDBObjectStoreRequest)

NS_IMETHODIMP
IDBObjectStoreRequest::GetMode(PRUint16* aMode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  *aMode = mMode;
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::GetName(nsAString& aName)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  aName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::GetKeyPath(nsAString& aKeyPath)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (mKeyPath.IsVoid()) {
    aKeyPath.SetIsVoid(PR_TRUE);
  }
  else {
    aKeyPath.Assign(mKeyPath);
  }
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::GetIndexNames(nsIDOMDOMStringList** aIndexNames)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<nsDOMStringList> list(new nsDOMStringList());
  PRUint32 count = mIndexes.Length();
  for (PRUint32 index = 0; index < count; index++) {
    NS_ENSURE_TRUE(list->Add(mIndexes[index]), NS_ERROR_OUT_OF_MEMORY);
  }

  list.forget(aIndexNames);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Put(nsIVariant* aValue,
                           nsIVariant* aKey,
                           PRBool aNoOverwrite,
                           nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Remove(nsIVariant* aKey,
                              nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Get(nsIVariant* aKey,
                           nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBObjectStoreRequest::OpenCursor(nsIIDBKeyRange* aRange,
                                  PRUint16 aDirection,
                                  nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  return NS_ERROR_NOT_IMPLEMENTED;
}




nsresult
PutObjectStoreHelper::Init()
{
  return NS_OK;
}

PRUint16
PutObjectStoreHelper::DoDatabaseWork()
{
  nsresult rv = mDatabase->EnsureConnection();
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseError::UNKNOWN_ERR);

  nsCOMPtr<mozIStorageConnection> connection = mDatabase->Connection();

  
  mozStorageTransaction transaction(connection, PR_FALSE);

  

  
  nsCOMPtr<mozIStorageStatement> stmt;
  if (mAutoIncrement) {
    rv = connection->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO ai_object_data (object_store_id, data)"
      "VALUES (:osid, :data)"
    ), getter_AddRefs(stmt));
  }
  else {
    rv = connection->CreateStatement(NS_LITERAL_CSTRING(
      "INSERT INTO object_data (object_store_id, key_value, data)"
      "VALUES (:osid, :key_value, :data)"
    ), getter_AddRefs(stmt));
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseError::UNKNOWN_ERR);

    
    
  }
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseError::UNKNOWN_ERR);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mOSID);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseError::UNKNOWN_ERR);

  

  if (NS_FAILED(stmt->Execute())) {
    return nsIIDBDatabaseError::CONSTRAINT_ERR;
  }

  

  return NS_SUCCEEDED(transaction.Commit()) ?
         OK :
         nsIIDBDatabaseError::UNKNOWN_ERR;
}

void
PutObjectStoreHelper::GetSuccessResult(nsIWritableVariant* aResult)
{
  
}
