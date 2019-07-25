







































#include "jscntxt.h"
#include "jsapi.h"
#include "nsContentUtils.h"
#include "nsJSON.h"
#include "IDBEvents.h"

#include "IDBObjectStoreRequest.h"
#include "IDBIndexRequest.h"

#include "nsIIDBDatabaseException.h"
#include "nsIJSContextStack.h"
#include "nsIUUIDGenerator.h"
#include "nsIVariant.h"

#include "nsDOMClassInfo.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "mozilla/Storage.h"

#include "AsyncConnectionHelper.h"
#include "IDBCursorRequest.h"
#include "IDBKeyRange.h"
#include "IDBTransactionRequest.h"
#include "DatabaseInfo.h"
#include "Savepoint.h"

USING_INDEXEDDB_NAMESPACE

namespace {

class AddHelper : public AsyncConnectionHelper
{
public:
  AddHelper(IDBTransactionRequest* aTransaction,
            IDBRequest* aRequest,
            PRInt64 aObjectStoreID,
            const nsAString& aKeyPath,
            const nsAString& aValue,
            const Key& aKey,
            bool aAutoIncrement,
            bool aCreate,
            bool aOverwrite)
  : AsyncConnectionHelper(aTransaction, aRequest), mOSID(aObjectStoreID),
    mKeyPath(aKeyPath), mValue(aValue), mKey(aKey),
    mAutoIncrement(aAutoIncrement), mCreate(aCreate), mOverwrite(aOverwrite)
  { }

  PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection);
  PRUint16 GetSuccessResult(nsIWritableVariant* aResult);

  nsresult ModifyValueForNewKey();

private:
  
  const PRInt64 mOSID;
  const nsString mKeyPath;
  
  nsString mValue;
  Key mKey;
  const bool mAutoIncrement;
  const bool mCreate;
  const bool mOverwrite;
};

class GetHelper : public AsyncConnectionHelper
{
public:
  GetHelper(IDBTransactionRequest* aTransaction,
            IDBRequest* aRequest,
            PRInt64 aObjectStoreID,
            const Key& aKey,
            bool aAutoIncrement)
  : AsyncConnectionHelper(aTransaction, aRequest), mOSID(aObjectStoreID),
    mKey(aKey), mAutoIncrement(aAutoIncrement)
  { }

  PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection);
  PRUint16 OnSuccess(nsIDOMEventTarget* aTarget);

protected:
  
  const PRInt64 mOSID;
  const Key mKey;
  const bool mAutoIncrement;

private:
  
  nsString mValue;
};

class RemoveHelper : public GetHelper
{
public:
  RemoveHelper(IDBTransactionRequest* aTransaction,
               IDBRequest* aRequest,
               PRInt64 aObjectStoreID,
               const Key& aKey,
               bool aAutoIncrement)
  : GetHelper(aTransaction, aRequest, aObjectStoreID, aKey, aAutoIncrement)
  { }

  PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection);
  PRUint16 OnSuccess(nsIDOMEventTarget* aTarget);
  PRUint16 GetSuccessResult(nsIWritableVariant* aResult);
};

class OpenCursorHelper : public AsyncConnectionHelper
{
public:
  OpenCursorHelper(IDBTransactionRequest* aTransaction,
                   IDBRequest* aRequest,
                   IDBObjectStoreRequest* aObjectStore,
                   const Key& aLeftKey,
                   const Key& aRightKey,
                   PRUint16 aKeyRangeFlags,
                   PRUint16 aDirection,
                   PRBool aPreload)
  : AsyncConnectionHelper(aTransaction, aRequest), mObjectStore(aObjectStore),
    mLeftKey(aLeftKey), mRightKey(aRightKey), mKeyRangeFlags(aKeyRangeFlags),
    mDirection(aDirection), mPreload(aPreload)
  { }

  PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection);
  PRUint16 GetSuccessResult(nsIWritableVariant* aResult);

private:
  
  const nsRefPtr<IDBObjectStoreRequest> mObjectStore;
  const Key mLeftKey;
  const Key mRightKey;
  const PRUint16 mKeyRangeFlags;
  const PRUint16 mDirection;
  const PRBool mPreload;

  
  nsTArray<KeyValuePair> mData;
};

class CreateIndexHelper : public AsyncConnectionHelper
{
public:
  CreateIndexHelper(IDBTransactionRequest* aDatabase,
                    IDBRequest* aRequest,
                    const nsAString& aName,
                    const nsAString& aKeyPath,
                    bool aUnique,
                    bool aAutoIncrement,
                    IDBObjectStoreRequest* aObjectStore)
  : AsyncConnectionHelper(aDatabase, aRequest), mName(aName),
    mKeyPath(aKeyPath), mUnique(aUnique), mAutoIncrement(aAutoIncrement),
    mObjectStore(aObjectStore), mId(LL_MININT)
  { }

  PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection);
  PRUint16 GetSuccessResult(nsIWritableVariant* aResult);

private:
  
  nsString mName;
  nsString mKeyPath;
  const bool mUnique;
  const bool mAutoIncrement;
  nsRefPtr<IDBObjectStoreRequest> mObjectStore;

  
  PRInt64 mId;
};

class RemoveIndexHelper : public AsyncConnectionHelper
{
public:
  RemoveIndexHelper(IDBTransactionRequest* aDatabase,
                    IDBRequest* aRequest,
                    const nsAString& aName,
                    IDBObjectStoreRequest* aObjectStore)
  : AsyncConnectionHelper(aDatabase, aRequest), mName(aName),
    mObjectStore(aObjectStore)
  { }

  PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection);
  PRUint16 GetSuccessResult(nsIWritableVariant* aResult);

private:
  
  nsString mName;
  nsRefPtr<IDBObjectStoreRequest> mObjectStore;
};


class GetSuccessEvent : public IDBSuccessEvent
{
public:
  GetSuccessEvent(const nsAString& aValue)
  : mValue(aValue),
    mCachedValue(JSVAL_VOID),
    mJSRuntime(nsnull)
  { }

  ~GetSuccessEvent()
  {
    if (mJSRuntime) {
      JS_RemoveRootRT(mJSRuntime, &mCachedValue);
    }
  }

  NS_IMETHOD GetResult(nsIVariant** aResult);

  nsresult Init(IDBRequest* aRequest,
                IDBTransactionRequest* aTransaction)
  {
    mSource = aRequest->GetGenerator();
    mTransaction = aTransaction;

    nsresult rv = InitEvent(NS_LITERAL_STRING(SUCCESS_EVT_STR), PR_FALSE,
                            PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetTrusted(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  nsString mValue;
  jsval mCachedValue;
  JSRuntime* mJSRuntime;
};

inline
nsresult
GetKeyFromObject(JSContext* aCx,
                 JSObject* aObj,
                 const nsString& aKeyPath,
                 Key& aKey)
{
  NS_PRECONDITION(aCx && aObj, "Null pointers!");
  NS_ASSERTION(!aKeyPath.IsVoid(), "This will explode!");

  const jschar* keyPathChars = reinterpret_cast<const jschar*>(aKeyPath.get());
  const size_t keyPathLen = aKeyPath.Length();

  jsval key;
  JSBool ok = JS_GetUCProperty(aCx, aObj, keyPathChars, keyPathLen, &key);
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  if (JSVAL_IS_VOID(key)) {
    aKey = Key::UNSETKEY;
    return NS_OK;
  }

  if (JSVAL_IS_NULL(key)) {
    aKey = Key::NULLKEY;
    return NS_OK;
  }

  if (JSVAL_IS_INT(key)) {
    aKey = JSVAL_TO_INT(key);
    return NS_OK;
  }

  if (JSVAL_IS_DOUBLE(key)) {
    aKey = *JSVAL_TO_DOUBLE(key);
    return NS_OK;
  }

  if (JSVAL_IS_STRING(key)) {
    JSString* str = JSVAL_TO_STRING(key);
    size_t len = JS_GetStringLength(str);
    if (!len) {
      return NS_ERROR_INVALID_ARG;
    }
    const PRUnichar* chars =
      reinterpret_cast<const PRUnichar*>(JS_GetStringChars(str));
    aKey = nsDependentString(chars, len);
    return NS_OK;
  }

  
  return NS_ERROR_INVALID_ARG;
}

} 


already_AddRefed<IDBObjectStoreRequest>
IDBObjectStoreRequest::Create(IDBDatabaseRequest* aDatabase,
                              IDBTransactionRequest* aTransaction,
                              const ObjectStoreInfo* aStoreInfo,
                              PRUint16 aMode)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<IDBObjectStoreRequest> objectStore = new IDBObjectStoreRequest();

  objectStore->mDatabase = aDatabase;
  objectStore->mTransaction = aTransaction;
  objectStore->mName = aStoreInfo->name;
  objectStore->mId = aStoreInfo->id;
  objectStore->mKeyPath = aStoreInfo->keyPath;
  objectStore->mAutoIncrement = aStoreInfo->autoIncrement;
  objectStore->mDatabaseId = aStoreInfo->databaseId;
  objectStore->mMode = aMode;

  return objectStore.forget();
}


nsresult
IDBObjectStoreRequest::GetKeyFromVariant(nsIVariant* aKeyVariant,
                                         Key& aKey)
{
  NS_ASSERTION(aKeyVariant, "Null pointer!");

  PRUint16 type;
  nsresult rv = aKeyVariant->GetDataType(&type);
  NS_ENSURE_SUCCESS(rv, rv);

  
  switch (type) {
    case nsIDataType::VTYPE_VOID:
      aKey = Key::UNSETKEY;
      break;

    case nsIDataType::VTYPE_EMPTY:
      aKey = Key::NULLKEY;
      break;

    case nsIDataType::VTYPE_WSTRING_SIZE_IS:
      rv = aKeyVariant->GetAsAString(aKey.ToString());
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case nsIDataType::VTYPE_INT32:
    case nsIDataType::VTYPE_DOUBLE:
      rv = aKeyVariant->GetAsInt64(aKey.ToIntPtr());
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    default:
      return NS_ERROR_INVALID_ARG;
  }

  return NS_OK;
}

ObjectStoreInfo*
IDBObjectStoreRequest::GetObjectStoreInfo()
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  ObjectStoreInfo* info;
  if (!ObjectStoreInfo::Get(mDatabaseId, mName, &info)) {
    NS_ERROR("This should never fail!");
    return nsnull;
  }
  return info;
}

IDBObjectStoreRequest::IDBObjectStoreRequest()
: mId(LL_MININT),
  mAutoIncrement(PR_FALSE),
  mMode(nsIIDBTransaction::READ_WRITE)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

IDBObjectStoreRequest::~IDBObjectStoreRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

nsresult
IDBObjectStoreRequest::GetJSONAndKeyForAdd(
                                           nsIVariant* aKeyVariant,
                                           nsString& aJSON,
                                           Key& aKey)
{
  
  
  NS_WARNING("Using a slow path for Add! Fix this now!");

  nsIXPConnect* xpc = nsContentUtils::XPConnect();
  NS_ENSURE_TRUE(xpc, NS_ERROR_UNEXPECTED);

  nsAXPCNativeCallContext* cc;
  nsresult rv = xpc->GetCurrentNativeCallContext(&cc);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(cc, NS_ERROR_UNEXPECTED);

  PRUint32 argc;
  rv = cc->GetArgc(&argc);
  NS_ENSURE_SUCCESS(rv, rv);

  if (argc < 1) {
    return NS_ERROR_XPC_NOT_ENOUGH_ARGS;
  }

  jsval* argv;
  rv = cc->GetArgvPtr(&argv);
  NS_ENSURE_SUCCESS(rv, rv);

  JSContext* cx;
  rv = cc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(cx);

  js::AutoValueRooter clone(cx);
  rv = nsContentUtils::CreateStructuredClone(cx, argv[0], clone.addr());
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (mKeyPath.IsEmpty()) {
    
    if (argc < 2) {
      
      aKey = Key::UNSETKEY;
    }
    else {
      rv = GetKeyFromVariant(aKeyVariant, aKey);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    
    rv = GetKeyFromObject(cx, JSVAL_TO_OBJECT(clone.value()), mKeyPath, aKey);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (aKey.IsUnset() && argc >= 2 && JSVAL_IS_NULL(argv[1])) {
      aKey = Key::NULLKEY;
    }
  }

  if (aKey.IsUnset() && !mAutoIncrement) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIJSON> json(new nsJSON());

  rv = json->EncodeFromJSVal(clone.addr(), cx, aJSON);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMPL_ADDREF(IDBObjectStoreRequest)
NS_IMPL_RELEASE(IDBObjectStoreRequest)

NS_INTERFACE_MAP_BEGIN(IDBObjectStoreRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, IDBRequest::Generator)
  NS_INTERFACE_MAP_ENTRY(nsIIDBObjectStoreRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBObjectStore)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBObjectStoreRequest)
NS_INTERFACE_MAP_END

DOMCI_DATA(IDBObjectStoreRequest, IDBObjectStoreRequest)

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

  aKeyPath.Assign(mKeyPath);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::GetIndexNames(nsIDOMDOMStringList** aIndexNames)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  ObjectStoreInfo* info = GetObjectStoreInfo();
  NS_ENSURE_TRUE(info, NS_ERROR_UNEXPECTED);

  nsRefPtr<nsDOMStringList> list(new nsDOMStringList());
  PRUint32 count = info->indexNames.Length();
  for (PRUint32 index = 0; index < count; index++) {
    NS_ENSURE_TRUE(list->Add(info->indexNames[index]), NS_ERROR_OUT_OF_MEMORY);
  }
  list.forget(aIndexNames);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Get(nsIVariant* aKey,
                           nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  Key key;
  nsresult rv = GetKeyFromVariant(aKey, key);
  NS_ENSURE_SUCCESS(rv, rv);

  if (key.IsUnset()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsRefPtr<IDBRequest> request = GenerateRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<GetHelper> helper =
    new GetHelper(mTransaction, request, mId, key, !!mAutoIncrement);
  rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::GetAll(nsIIDBKeyRange* aKeyRange,
                              PRUint32 aLimit,
                              PRUint8 aOptionalArgCount,
                              nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Add(nsIVariant* ,
                           nsIVariant* aKey,
                           nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  if (mMode != nsIIDBTransaction::READ_WRITE) {
    return NS_ERROR_OBJECT_IS_IMMUTABLE;
  }

  nsString jsonValue;
  Key key;

  nsresult rv = GetJSONAndKeyForAdd(aKey, jsonValue, key);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (key.IsUnset() && !mAutoIncrement) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsRefPtr<IDBRequest> request = GenerateWriteRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<AddHelper> helper =
    new AddHelper(mTransaction, request, mId, mKeyPath, jsonValue, key,
                  !!mAutoIncrement, true, false);
  rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Modify(nsIVariant* ,
                              nsIVariant* aKey,
                              nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  if (mMode != nsIIDBTransaction::READ_WRITE) {
    return NS_ERROR_OBJECT_IS_IMMUTABLE;
  }

  nsString jsonValue;
  Key key;

  nsresult rv = GetJSONAndKeyForAdd(aKey, jsonValue, key);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (key.IsUnset() || key.IsNull()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsRefPtr<IDBRequest> request = GenerateWriteRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<AddHelper> helper =
    new AddHelper(mTransaction, request, mId, mKeyPath, jsonValue, key,
                  !!mAutoIncrement, false, true);
  rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::AddOrModify(nsIVariant* ,
                                   nsIVariant* aKey,
                                   nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  if (mMode != nsIIDBTransaction::READ_WRITE) {
    return NS_ERROR_OBJECT_IS_IMMUTABLE;
  }

  nsString jsonValue;
  Key key;

  nsresult rv = GetJSONAndKeyForAdd(aKey, jsonValue, key);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (key.IsUnset() || key.IsNull()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsRefPtr<IDBRequest> request = GenerateWriteRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<AddHelper> helper =
    new AddHelper(mTransaction, request, mId, mKeyPath, jsonValue, key,
                  !!mAutoIncrement, true, true);
  rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Remove(nsIVariant* aKey,
                              nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  if (mMode != nsIIDBTransaction::READ_WRITE) {
    return NS_ERROR_OBJECT_IS_IMMUTABLE;
  }

  Key key;
  nsresult rv = GetKeyFromVariant(aKey, key);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (key.IsUnset() || key.IsNull()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsRefPtr<IDBRequest> request = GenerateWriteRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<RemoveHelper> helper =
    new RemoveHelper(mTransaction, request, mId, key, !!mAutoIncrement);
  rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::OpenCursor(nsIIDBKeyRange* aKeyRange,
                                  PRUint16 aDirection,
                                  PRBool aPreload,
                                  PRUint8 aOptionalArgCount,
                                  nsIIDBRequest** _retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsresult rv;

  Key leftKey, rightKey;
  PRUint16 keyRangeFlags = 0;

  if (aKeyRange) {
    rv = aKeyRange->GetFlags(&keyRangeFlags);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIVariant> variant;
    rv = aKeyRange->GetLeft(getter_AddRefs(variant));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetKeyFromVariant(variant, leftKey);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aKeyRange->GetRight(getter_AddRefs(variant));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetKeyFromVariant(variant, rightKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aOptionalArgCount >= 2) {
    if (aDirection != nsIIDBCursor::NEXT &&
        aDirection != nsIIDBCursor::NEXT_NO_DUPLICATE &&
        aDirection != nsIIDBCursor::PREV &&
        aDirection != nsIIDBCursor::PREV_NO_DUPLICATE) {
      return NS_ERROR_INVALID_ARG;
    }
  }
  else {
    aDirection = nsIIDBCursor::NEXT;
  }

  if (aPreload) {
    NS_NOTYETIMPLEMENTED("Implement me!");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsRefPtr<IDBRequest> request = GenerateRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<OpenCursorHelper> helper =
    new OpenCursorHelper(mTransaction, request, this, leftKey, rightKey,
                         keyRangeFlags, aDirection, aPreload);

  rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::CreateIndex(const nsAString& aName,
                                   const nsAString& aKeyPath,
                                   PRBool aUnique,
                                   nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  if (aName.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  ObjectStoreInfo* info = GetObjectStoreInfo();
  NS_ENSURE_TRUE(info, NS_ERROR_UNEXPECTED);

  if (info->indexNames.Contains(aName)) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  
  nsString keyPath(aKeyPath);
  if (keyPath.IsVoid()) {
    keyPath.Truncate();
  }

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<IDBRequest> request = GenerateWriteRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<CreateIndexHelper> helper =
    new CreateIndexHelper(mTransaction, request, aName, keyPath, !!aUnique,
                          mAutoIncrement, this);
  nsresult rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::Index(const nsAString& aName,
                             nsIIDBIndexRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  if (aName.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  ObjectStoreInfo* info = GetObjectStoreInfo();
  NS_ENSURE_TRUE(info, NS_ERROR_UNEXPECTED);

  if (!info->indexNames.Contains(aName)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<IDBIndexRequest> request =
    IDBIndexRequest::Create(mDatabase, this, mTransaction);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBObjectStoreRequest::RemoveIndex(const nsAString& aName,
                                   nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  if (!mTransaction->TransactionIsOpen()) {
    return NS_ERROR_UNEXPECTED;
  }

  if (aName.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }

  ObjectStoreInfo* info = GetObjectStoreInfo();
  NS_ENSURE_TRUE(info, NS_ERROR_UNEXPECTED);

  if (!info->indexNames.Contains(aName)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<IDBRequest> request = GenerateWriteRequest();

  nsRefPtr<RemoveIndexHelper> helper =
    new RemoveIndexHelper(mTransaction, request, aName, this);
  nsresult rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

PRUint16
AddHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  NS_PRECONDITION(aConnection, "Passed a null connection!");

  nsresult rv;
  if (mKey.IsNull()) {
    NS_WARNING("Using a UUID for null keys, probably can do something faster!");

    nsCOMPtr<nsIUUIDGenerator> uuidGen =
      do_GetService("@mozilla.org/uuid-generator;1", &rv);
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

    nsID id;
    rv = uuidGen->GenerateUUIDInPlace(&id);
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

    char idString[NSID_LENGTH] = { 0 };
    id.ToProvidedString(idString);

    mKey = NS_ConvertASCIItoUTF16(idString);
  }

  bool mayOverwrite = mOverwrite;
  bool unsetKey = mKey.IsUnset();

  if (unsetKey) {
    NS_ASSERTION(mAutoIncrement, "Must have a key for non-autoIncrement!");

    
    mayOverwrite = false;
  }

  if (mAutoIncrement && !unsetKey) {
    mayOverwrite = true;
  }

  nsCOMPtr<mozIStorageStatement> stmt =
    mTransaction->AddStatement(mCreate, mayOverwrite, mAutoIncrement);
  NS_ENSURE_TRUE(stmt, nsIIDBDatabaseException::UNKNOWN_ERR);

  mozStorageStatementScoper scoper(stmt);

  Savepoint savepoint(mTransaction);

  NS_NAMED_LITERAL_CSTRING(keyValue, "key_value");

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mOSID);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  if (!mAutoIncrement || mayOverwrite) {
    NS_ASSERTION(!mKey.IsUnset(), "This shouldn't happen!");

    if (mKey.IsInt()) {
      rv = stmt->BindInt64ByName(keyValue, mKey.IntValue());
    }
    else if (mKey.IsString()) {
      rv = stmt->BindStringByName(keyValue, mKey.StringValue());
    }
    else {
      NS_NOTREACHED("Unknown key type!");
    }
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);
  }

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("data"), mValue);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  if (NS_FAILED(stmt->Execute())) {
    return nsIIDBDatabaseException::CONSTRAINT_ERR;
  }

  if (mAutoIncrement && mCreate && !mOverwrite) {
#ifdef DEBUG
    PRInt64 oldKey = unsetKey ? 0 : mKey.IntValue();
#endif

    rv = aConnection->GetLastInsertRowID(mKey.ToIntPtr());
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

#ifdef DEBUG
    NS_ASSERTION(mKey.IsInt(), "Bad key value!");
    if (!unsetKey) {
      NS_ASSERTION(mKey.IntValue() == oldKey, "Something went haywire!");
    }
#endif

    if (!mKeyPath.IsEmpty() && unsetKey) {
      
      
      
      rv = ModifyValueForNewKey();
      NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

      scoper.Abandon();
      rv = stmt->Reset();
      NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

      stmt = mTransaction->AddStatement(false, true, true);
      NS_ENSURE_TRUE(stmt, nsIIDBDatabaseException::UNKNOWN_ERR);

      mozStorageStatementScoper scoper2(stmt);

      rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mOSID);
      NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

      rv = stmt->BindInt64ByName(keyValue, mKey.IntValue());
      NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

      rv = stmt->BindStringByName(NS_LITERAL_CSTRING("data"), mValue);
      NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

      rv = stmt->Execute();
      NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);
    }
  }

  

  rv = savepoint.Release();
  return NS_SUCCEEDED(rv) ? OK : nsIIDBDatabaseException::UNKNOWN_ERR;
}

PRUint16
AddHelper::GetSuccessResult(nsIWritableVariant* aResult)
{
  NS_ASSERTION(!mKey.IsUnset() && !mKey.IsNull(), "Badness!");

  if (mKey.IsString()) {
    aResult->SetAsAString(mKey.StringValue());
  }
  else if (mKey.IsInt()) {
    aResult->SetAsInt64(mKey.IntValue());
  }
  else {
    NS_NOTREACHED("Unknown key type!");
  }
  return OK;
}

nsresult
AddHelper::ModifyValueForNewKey()
{
  NS_ASSERTION(mAutoIncrement && !mKeyPath.IsEmpty() && mKey.IsInt(),
               "Don't call me!");

  JSContext* cx;
  nsresult rv = nsContentUtils::ThreadJSContextStack()->GetSafeJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(cx);

  js::AutoValueRooter clone(cx);

  nsCOMPtr<nsIJSON> json(new nsJSON());
  rv = json->DecodeToJSVal(mValue, cx, clone.addr());
  NS_ENSURE_SUCCESS(rv, rv);

  JSObject* obj = JSVAL_TO_OBJECT(clone.value());
  JSBool ok;
  js::AutoValueRooter key(cx);

  const jschar* keyPathChars = reinterpret_cast<const jschar*>(mKeyPath.get());
  const size_t keyPathLen = mKeyPath.Length();

#ifdef DEBUG
  ok = JS_GetUCProperty(cx, obj, keyPathChars, keyPathLen, key.addr());
  NS_ASSERTION(ok && JSVAL_IS_VOID(key.value()), "Already has a key prop!");
#endif

  ok = JS_NewNumberValue(cx, mKey.IntValue(), key.addr());
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  ok = JS_DefineUCProperty(cx, obj, keyPathChars, keyPathLen, key.value(),
                           nsnull, nsnull, JSPROP_ENUMERATE);
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  rv = json->EncodeFromJSVal(clone.addr(), cx, mValue);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRUint16
GetHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  NS_PRECONDITION(aConnection, "Passed a null connection!");

  nsCOMPtr<mozIStorageStatement> stmt =
    mTransaction->GetStatement(mAutoIncrement);
  NS_ENSURE_TRUE(stmt, nsIIDBDatabaseException::UNKNOWN_ERR);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mOSID);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  NS_ASSERTION(!mKey.IsUnset() && !mKey.IsNull(), "Must have a key here!");

  NS_NAMED_LITERAL_CSTRING(id, "id");

  if (mKey.IsInt()) {
    rv = stmt->BindInt64ByName(id, mKey.IntValue());
  }
  else if (mKey.IsString()) {
    rv = stmt->BindStringByName(id, mKey.StringValue());
  }
  else {
    NS_NOTREACHED("Unknown key type!");
  }
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  
  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  if (!hasResult) {
    return nsIIDBDatabaseException::NOT_FOUND_ERR;
  }

  
  (void)stmt->GetString(0, mValue);

  return OK;
}

PRUint16
GetHelper::OnSuccess(nsIDOMEventTarget* aTarget)
{
  nsRefPtr<GetSuccessEvent> event(new GetSuccessEvent(mValue));
  nsresult rv = event->Init(mRequest, mTransaction);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  PRBool dummy;
  aTarget->DispatchEvent(static_cast<nsDOMEvent*>(event), &dummy);
  return OK;
}

PRUint16
RemoveHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  NS_PRECONDITION(aConnection, "Passed a null connection!");

  nsCOMPtr<mozIStorageStatement> stmt =
    mTransaction->RemoveStatement(mAutoIncrement);
  NS_ENSURE_TRUE(stmt, nsIIDBDatabaseException::UNKNOWN_ERR);
  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mOSID);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  NS_ASSERTION(!mKey.IsUnset() && !mKey.IsNull(), "Must have a key here!");

  NS_NAMED_LITERAL_CSTRING(key_value, "key_value");

  if (mKey.IsInt()) {
    rv = stmt->BindInt64ByName(key_value, mKey.IntValue());
  }
  else if (mKey.IsString()) {
    rv = stmt->BindStringByName(key_value, mKey.StringValue());
  }
  else {
    NS_NOTREACHED("Unknown key type!");
  }
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  
  rv = stmt->Execute();
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  return OK;
}

PRUint16
RemoveHelper::OnSuccess(nsIDOMEventTarget* aTarget)
{
  return AsyncConnectionHelper::OnSuccess(aTarget);
}

PRUint16
RemoveHelper::GetSuccessResult(nsIWritableVariant* aResult)
{
  NS_ASSERTION(!mKey.IsUnset() && !mKey.IsNull(), "Badness!");

  if (mKey.IsString()) {
    aResult->SetAsAString(mKey.StringValue());
  }
  else if (mKey.IsInt()) {
    aResult->SetAsInt64(mKey.IntValue());
  }
  else {
    NS_NOTREACHED("Unknown key type!");
  }
  return OK;
}

PRUint16
OpenCursorHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  nsCString table;
  nsCString keyColumn;

  if (mObjectStore->IsAutoIncrement()) {
    table.AssignLiteral("ai_object_data");
    keyColumn.AssignLiteral("id");
  }
  else {
    table.AssignLiteral("object_data");
    keyColumn.AssignLiteral("key_value");
  }

  NS_NAMED_LITERAL_CSTRING(osid, "osid");
  NS_NAMED_LITERAL_CSTRING(leftKeyName, "left_key");
  NS_NAMED_LITERAL_CSTRING(rightKeyName, "right_key");

  nsCAutoString keyRangeClause;
  if (!mLeftKey.IsUnset()) {
    keyRangeClause.AppendLiteral(" AND ");
    keyRangeClause.Append(keyColumn);
    if (mKeyRangeFlags & nsIIDBKeyRange::LEFT_OPEN) {
      keyRangeClause.AppendLiteral(" > :");
    }
    else {
      NS_ASSERTION(mKeyRangeFlags & nsIIDBKeyRange::LEFT_BOUND, "Bad flags!");
      keyRangeClause.AppendLiteral(" >= :");
    }
    keyRangeClause.Append(leftKeyName);
  }

  if (!mRightKey.IsUnset()) {
    keyRangeClause.AppendLiteral(" AND ");
    keyRangeClause.Append(keyColumn);
    if (mKeyRangeFlags & nsIIDBKeyRange::RIGHT_OPEN) {
      keyRangeClause.AppendLiteral(" < :");
    }
    else {
      NS_ASSERTION(mKeyRangeFlags & nsIIDBKeyRange::RIGHT_BOUND, "Bad flags!");
      keyRangeClause.AppendLiteral(" <= :");
    }
    keyRangeClause.Append(rightKeyName);
  }

  nsCAutoString query("SELECT ");
  query.Append(keyColumn);
  query.AppendLiteral(", data FROM ");
  query.Append(table);
  query.AppendLiteral(" WHERE object_store_id = :");
  query.Append(osid);
  query.Append(keyRangeClause);
  query.AppendLiteral(" ORDER BY ");
  query.Append(keyColumn);
  query.AppendLiteral(" DESC");

  if (!mData.SetCapacity(50)) {
    NS_ERROR("Out of memory!");
    return nsIIDBDatabaseException::UNKNOWN_ERR;
  }

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aConnection->CreateStatement(query, getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindInt64ByName(osid, mObjectStore->Id());
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  if (!mLeftKey.IsUnset()) {
    if (mLeftKey.IsString()) {
      rv = stmt->BindStringByName(leftKeyName, mLeftKey.StringValue());
    }
    else if (mLeftKey.IsInt()) {
      rv = stmt->BindInt64ByName(leftKeyName, mLeftKey.IntValue());
    }
    else {
      NS_NOTREACHED("Bad key!");
    }
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);
  }

  if (!mRightKey.IsUnset()) {
    if (mRightKey.IsString()) {
      rv = stmt->BindStringByName(rightKeyName, mRightKey.StringValue());
    }
    else if (mRightKey.IsInt()) {
      rv = stmt->BindInt64ByName(rightKeyName, mRightKey.IntValue());
    }
    else {
      NS_NOTREACHED("Bad key!");
    }
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);
  }

  NS_WARNING("Copying all results for cursor snapshot, do something smarter!");

  PRBool hasResult;
  while (NS_SUCCEEDED((rv = stmt->ExecuteStep(&hasResult))) && hasResult) {
    if (mData.Capacity() == mData.Length()) {
      if (!mData.SetCapacity(mData.Capacity() * 2)) {
        NS_ERROR("Out of memory!");
        return nsIIDBDatabaseException::UNKNOWN_ERR;
      }
    }

    KeyValuePair* pair = mData.AppendElement();
    NS_ASSERTION(pair, "Shouldn't fail if SetCapacity succeeded!");

    PRInt32 keyType;
    rv = stmt->GetTypeOfIndex(0, &keyType);
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

    NS_ASSERTION(keyType == mozIStorageStatement::VALUE_TYPE_INTEGER ||
                 keyType == mozIStorageStatement::VALUE_TYPE_TEXT,
                 "Bad key type!");

    if (keyType == mozIStorageStatement::VALUE_TYPE_INTEGER) {
      pair->key = stmt->AsInt64(0);
    }
    else if (keyType == mozIStorageStatement::VALUE_TYPE_TEXT) {
      rv = stmt->GetString(0, pair->key.ToString());
      NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);
    }

#ifdef DEBUG
    {
      PRInt32 valueType;
      NS_ASSERTION(NS_SUCCEEDED(stmt->GetTypeOfIndex(1, &valueType)) &&
                   valueType == mozIStorageStatement::VALUE_TYPE_TEXT,
                   "Bad value type!");
    }
#endif

    rv = stmt->GetString(1, pair->value);
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);
  }
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  return OK;
}

PRUint16
OpenCursorHelper::GetSuccessResult(nsIWritableVariant* aResult)
{
  if (mData.IsEmpty()) {
    aResult->SetAsEmpty();
    return NS_OK;
  }

  nsRefPtr<IDBCursorRequest> cursor =
    IDBCursorRequest::Create(mRequest, mTransaction, mObjectStore, mDirection,
                             mData);
  NS_ENSURE_TRUE(cursor, nsIIDBDatabaseException::UNKNOWN_ERR);

  aResult->SetAsISupports(static_cast<IDBRequest::Generator*>(cursor));

  return OK;
}

PRUint16
CreateIndexHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  
  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO object_store_index (name, key_path, unique_index, "
      "object_store_id, object_store_autoincrement) "
    "VALUES (:name, :key_path, :unique, :osid, :os_auto_increment)"
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("name"), mName);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("key_path"), mKeyPath);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("unique"),
                             mUnique ? 1 : 0);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("osid"), mObjectStore->Id());
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindInt32ByName(NS_LITERAL_CSTRING("os_auto_increment"),
                             mAutoIncrement ? 1 : 0);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  if (NS_FAILED(stmt->Execute())) {
    return nsIIDBDatabaseException::CONSTRAINT_ERR;
  }

  
  (void)aConnection->GetLastInsertRowID(&mId);

  return OK;
}

PRUint16
CreateIndexHelper::GetSuccessResult(nsIWritableVariant* aResult)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  ObjectStoreInfo* info = mObjectStore->GetObjectStoreInfo();
  if (!info) {
    NS_ERROR("Couldn't get info!");
    return nsIIDBDatabaseException::UNKNOWN_ERR;
  }

  NS_ASSERTION(!info->indexNames.Contains(mName), "Alreayd have this index!");

  if (!info->indexNames.AppendElement(mName)) {
    NS_ERROR("Couldn't add index name!  Out of memory?");
    return nsIIDBDatabaseException::UNKNOWN_ERR;
  }

  nsCOMPtr<nsIIDBIndexRequest> result;
  nsresult rv = mObjectStore->Index(mName, getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  aResult->SetAsISupports(result);
  return OK;
}

PRUint16
RemoveIndexHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  NS_PRECONDITION(!NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<mozIStorageStatement> stmt;
  nsresult rv = aConnection->CreateStatement(NS_LITERAL_CSTRING(
    "DELETE FROM object_store_index "
    "WHERE name = :name "
  ), getter_AddRefs(stmt));
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("name"), mName);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  if (NS_FAILED(stmt->Execute())) {
    return nsIIDBDatabaseException::NOT_FOUND_ERR;
  }

  return OK;
}

PRUint16
RemoveIndexHelper::GetSuccessResult(nsIWritableVariant* )
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  ObjectStoreInfo* info = mObjectStore->GetObjectStoreInfo();
  if (!info) {
    NS_ERROR("Unable to get object store info!");
    return nsIIDBDatabaseException::UNKNOWN_ERR;
  }
  info->indexNames.RemoveElement(mName);
  return OK;
}


NS_IMETHODIMP
GetSuccessEvent::GetResult(nsIVariant** )
{
  
  
  NS_WARNING("Using a slow path for Get! Fix this now!");

  nsIXPConnect* xpc = nsContentUtils::XPConnect();
  NS_ENSURE_TRUE(xpc, NS_ERROR_UNEXPECTED);

  nsAXPCNativeCallContext* cc;
  nsresult rv = xpc->GetCurrentNativeCallContext(&cc);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(cc, NS_ERROR_UNEXPECTED);

  jsval* retval;
  rv = cc->GetRetValPtr(&retval);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mJSRuntime) {
    JSContext* cx;
    rv = cc->GetJSContext(&cx);
    NS_ENSURE_SUCCESS(rv, rv);

    JSAutoRequest ar(cx);

    JSRuntime* rt = JS_GetRuntime(cx);

    JSBool ok = JS_AddNamedRootRT(rt, &mCachedValue,
                                  "GetSuccessEvent::mCachedValue");
    NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

    nsCOMPtr<nsIJSON> json(new nsJSON());
    rv = json->DecodeToJSVal(mValue, cx, &mCachedValue);
    NS_ENSURE_SUCCESS(rv, rv);

    mJSRuntime = rt;
  }

  *retval = mCachedValue;
  cc->SetReturnValueWasSet(PR_TRUE);
  return NS_OK;
}
