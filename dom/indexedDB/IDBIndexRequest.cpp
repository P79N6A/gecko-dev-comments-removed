







































#include "IDBIndexRequest.h"

#include "nsIIDBDatabaseException.h"

#include "jsapi.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfo.h"
#include "nsJSON.h"
#include "nsThreadUtils.h"
#include "mozilla/Storage.h"

#include "AsyncConnectionHelper.h"
#include "IDBEvents.h"
#include "IDBObjectStoreRequest.h"
#include "IDBTransactionRequest.h"
#include "DatabaseInfo.h"

USING_INDEXEDDB_NAMESPACE

namespace {

class GetHelper : public AsyncConnectionHelper
{
public:
  GetHelper(IDBTransactionRequest* aTransaction,
            IDBRequest* aRequest,
            const nsAString& aValue,
            PRInt64 aId,
            bool aUnique,
            bool aAutoIncrement)
  : AsyncConnectionHelper(aTransaction, aRequest), mValue(aValue), mId(aId),
    mUnique(aUnique), mAutoIncrement(aAutoIncrement)
  { }

  PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection);
  PRUint16 GetSuccessResult(nsIWritableVariant* aResult);

protected:
  
  nsString mValue;
  const PRInt64 mId;
  const bool mUnique;
  const bool mAutoIncrement;

private:
  
  Key mKey;
};

class GetObjectHelper : public GetHelper
{
public:
  GetObjectHelper(IDBTransactionRequest* aTransaction,
                  IDBRequest* aRequest,
                  const nsAString& aValue,
                  PRInt64 aId,
                  bool aUnique,
                  bool aAutoIncrement)
  : GetHelper(aTransaction, aRequest, aValue, aId, aUnique, aAutoIncrement)
  { }

  PRUint16 DoDatabaseWork(mozIStorageConnection* aConnection);
  PRUint16 OnSuccess(nsIDOMEventTarget* aTarget);
};


class GetObjectSuccessEvent : public IDBSuccessEvent
{
public:
  GetObjectSuccessEvent(const nsAString& aValue)
  : mValue(aValue),
    mCachedValue(JSVAL_VOID),
    mJSRuntime(nsnull)
  { }

  ~GetObjectSuccessEvent()
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

} 


already_AddRefed<IDBIndexRequest>
IDBIndexRequest::Create(IDBObjectStoreRequest* aObjectStore,
                        const IndexInfo* aIndexInfo)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aObjectStore, "Null pointer!");
  NS_ASSERTION(aIndexInfo, "Null pointer!");

  nsRefPtr<IDBIndexRequest> index = new IDBIndexRequest();

  index->mObjectStore = aObjectStore;
  index->mId = aIndexInfo->id;
  index->mName = aIndexInfo->name;
  index->mKeyPath = aIndexInfo->keyPath;
  index->mUnique = aIndexInfo->unique;

  return index.forget();
}

IDBIndexRequest::IDBIndexRequest()
: mId(LL_MININT),
  mUnique(false),
  mAutoIncrement(false)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
}

IDBIndexRequest::~IDBIndexRequest()
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
}

NS_IMPL_ADDREF(IDBIndexRequest)
NS_IMPL_RELEASE(IDBIndexRequest)

NS_INTERFACE_MAP_BEGIN(IDBIndexRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, IDBRequest::Generator)
  NS_INTERFACE_MAP_ENTRY(nsIIDBIndexRequest)
  NS_INTERFACE_MAP_ENTRY(nsIIDBIndex)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(IDBIndexRequest)
NS_INTERFACE_MAP_END

DOMCI_DATA(IDBIndexRequest, IDBIndexRequest)




NS_IMETHODIMP
IDBIndexRequest::GetName(nsAString& aName)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  aName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
IDBIndexRequest::GetStoreName(nsAString& aStoreName)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  return mObjectStore->GetName(aStoreName);
}

NS_IMETHODIMP
IDBIndexRequest::GetKeyPath(nsAString& aKeyPath)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  aKeyPath.Assign(mKeyPath);
  return NS_OK;
}

NS_IMETHODIMP
IDBIndexRequest::GetUnique(PRBool* aUnique)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  *aUnique = mUnique;
  return NS_OK;
}




NS_IMETHODIMP
IDBIndexRequest::OpenObjectCursor(nsIIDBKeyRange* aRange,
                                  PRUint16 aDirection,
                                  PRBool aPreload,
                                  nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBIndexRequest::OpenCursor(nsIIDBKeyRange* aRange,
                            PRUint16 aDirection,
                            PRBool aPreload,
                            nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");
  NS_NOTYETIMPLEMENTED("Implement me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
IDBIndexRequest::GetObject(nsIVariant* ,
                           nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  NS_WARNING("Using a slow path for Get! Fix this now!");

  nsString jsonValue;
  nsresult rv = IDBObjectStoreRequest::GetJSONFromArg0(jsonValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<IDBRequest> request = GenerateRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<GetObjectHelper> helper =
    new GetObjectHelper(mObjectStore->Transaction(), request, jsonValue, mId,
                        mUnique, mAutoIncrement);
  rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

NS_IMETHODIMP
IDBIndexRequest::Get(nsIVariant* ,
                     nsIIDBRequest** _retval)
{
  NS_PRECONDITION(NS_IsMainThread(), "Wrong thread!");

  NS_WARNING("Using a slow path for Get! Fix this now!");

  nsString jsonValue;
  nsresult rv = IDBObjectStoreRequest::GetJSONFromArg0(jsonValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<IDBRequest> request = GenerateRequest();
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsRefPtr<GetHelper> helper =
    new GetHelper(mObjectStore->Transaction(), request, jsonValue, mId, mUnique,
                  mAutoIncrement);
  rv = helper->DispatchToTransactionPool();
  NS_ENSURE_SUCCESS(rv, rv);

  request.forget(_retval);
  return NS_OK;
}

PRUint16
GetHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  NS_ASSERTION(aConnection, "Passed a null connection!");

  nsCOMPtr<mozIStorageStatement> stmt =
    mTransaction->IndexGetStatement(mUnique, mAutoIncrement);
  NS_ENSURE_TRUE(stmt, nsIIDBDatabaseException::UNKNOWN_ERR);

  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("index_id"), mId);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("value"), mValue);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  if (hasResult) {
    PRInt32 keyType;
    rv = stmt->GetTypeOfIndex(0, &keyType);
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

    NS_ASSERTION(keyType == mozIStorageStatement::VALUE_TYPE_INTEGER ||
                 keyType == mozIStorageStatement::VALUE_TYPE_TEXT,
                 "Bad key type!");

    if (keyType == mozIStorageStatement::VALUE_TYPE_INTEGER) {
      mKey = stmt->AsInt64(0);
    }
    else if (keyType == mozIStorageStatement::VALUE_TYPE_TEXT) {
      rv = stmt->GetString(0, mKey.ToString());
      NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);
    }
  }

  return OK;
}

PRUint16
GetHelper::GetSuccessResult(nsIWritableVariant* aResult)
{
  NS_ASSERTION(!mKey.IsNull(), "Badness!");

  if (mKey.IsUnset()) {
    aResult->SetAsEmpty();
  }
  else if (mKey.IsString()) {
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
GetObjectHelper::DoDatabaseWork(mozIStorageConnection* aConnection)
{
  NS_ASSERTION(aConnection, "Passed a null connection!");

  nsCOMPtr<mozIStorageStatement> stmt =
    mTransaction->IndexGetObjectStatement(mUnique, mAutoIncrement);
  NS_ENSURE_TRUE(stmt, nsIIDBDatabaseException::UNKNOWN_ERR);

  mozStorageStatementScoper scoper(stmt);

  nsresult rv = stmt->BindInt64ByName(NS_LITERAL_CSTRING("index_id"), mId);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  rv = stmt->BindStringByName(NS_LITERAL_CSTRING("value"), mValue);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  PRBool hasResult;
  rv = stmt->ExecuteStep(&hasResult);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  if (hasResult) {
    rv = stmt->GetString(0, mValue);
    NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);
  }
  else {
    mValue.SetIsVoid(PR_TRUE);
  }

  return OK;
}

PRUint16
GetObjectHelper::OnSuccess(nsIDOMEventTarget* aTarget)
{
  nsRefPtr<GetObjectSuccessEvent> event(new GetObjectSuccessEvent(mValue));
  nsresult rv = event->Init(mRequest, mTransaction);
  NS_ENSURE_SUCCESS(rv, nsIIDBDatabaseException::UNKNOWN_ERR);

  PRBool dummy;
  aTarget->DispatchEvent(static_cast<nsDOMEvent*>(event), &dummy);
  return OK;
}


NS_IMETHODIMP
GetObjectSuccessEvent::GetResult(nsIVariant** )
{
  
  
  NS_WARNING("Using a slow path for GetObject! Fix this now!");

  nsIXPConnect* xpc = nsContentUtils::XPConnect();
  NS_ENSURE_TRUE(xpc, NS_ERROR_UNEXPECTED);

  nsAXPCNativeCallContext* cc;
  nsresult rv = xpc->GetCurrentNativeCallContext(&cc);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(cc, NS_ERROR_UNEXPECTED);

  jsval* retval;
  rv = cc->GetRetValPtr(&retval);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mValue.IsVoid()) {
    *retval = JSVAL_VOID;
    return NS_OK;
  }

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
