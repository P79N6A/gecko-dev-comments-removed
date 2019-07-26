





#include "IDBRequest.h"

#include "nsIScriptContext.h"

#include "mozilla/ContentEvents.h"
#include "mozilla/dom/ErrorEventBinding.h"
#include "mozilla/dom/IDBOpenDBRequestBinding.h"
#include "mozilla/dom/UnionTypes.h"
#include "nsComponentManagerUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsDOMJSUtils.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsEventDispatcher.h"
#include "nsJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "nsWrapperCacheInlines.h"

#include "AsyncConnectionHelper.h"
#include "IDBCursor.h"
#include "IDBEvents.h"
#include "IDBFactory.h"
#include "IDBIndex.h"
#include "IDBObjectStore.h"
#include "IDBTransaction.h"
#include "ReportInternalError.h"

namespace {

#ifdef MOZ_ENABLE_PROFILER_SPS
uint64_t gNextRequestSerialNumber = 1;
#endif

} 

USING_INDEXEDDB_NAMESPACE
using mozilla::dom::OwningIDBObjectStoreOrIDBIndexOrIDBCursor;
using namespace mozilla;

IDBRequest::IDBRequest(IDBDatabase* aDatabase)
: IDBWrapperCache(aDatabase),
  mResultVal(JSVAL_VOID),
  mActorParent(nullptr),
#ifdef MOZ_ENABLE_PROFILER_SPS
  mSerialNumber(gNextRequestSerialNumber++),
#endif
  mErrorCode(NS_OK),
  mLineNo(0),
  mHaveResultOrErrorCode(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

IDBRequest::IDBRequest(nsPIDOMWindow* aOwner)
: IDBWrapperCache(aOwner),
  mResultVal(JSVAL_VOID),
  mActorParent(nullptr),
#ifdef MOZ_ENABLE_PROFILER_SPS
  mSerialNumber(gNextRequestSerialNumber++),
#endif
  mErrorCode(NS_OK),
  mLineNo(0),
  mHaveResultOrErrorCode(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

IDBRequest::~IDBRequest()
{
  mResultVal = JSVAL_VOID;
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}


already_AddRefed<IDBRequest>
IDBRequest::Create(IDBDatabase* aDatabase,
                   IDBTransaction* aTransaction)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  nsRefPtr<IDBRequest> request(new IDBRequest(aDatabase));

  request->mTransaction = aTransaction;
  request->SetScriptOwner(aDatabase->GetScriptOwner());

  if (!aDatabase->Factory()->FromIPC()) {
    request->CaptureCaller();
  }


  return request.forget();
}


already_AddRefed<IDBRequest>
IDBRequest::Create(IDBObjectStore* aSourceAsObjectStore,
                   IDBDatabase* aDatabase,
                   IDBTransaction* aTransaction)
{
  nsRefPtr<IDBRequest> request = Create(aDatabase, aTransaction);

  request->mSourceAsObjectStore = aSourceAsObjectStore;

  return request.forget();
}


already_AddRefed<IDBRequest>
IDBRequest::Create(IDBIndex* aSourceAsIndex,
                   IDBDatabase* aDatabase,
                   IDBTransaction* aTransaction)
{
  nsRefPtr<IDBRequest> request = Create(aDatabase, aTransaction);

  request->mSourceAsIndex = aSourceAsIndex;

  return request.forget();
}

#ifdef DEBUG
void
IDBRequest::AssertSourceIsCorrect() const
{
  
  
  

  MOZ_ASSERT(!!mSourceAsObjectStore + !!mSourceAsIndex + !!mSourceAsCursor <= 1);
}
#endif

void
IDBRequest::GetSource(Nullable<OwningIDBObjectStoreOrIDBIndexOrIDBCursor>& aSource) const
{
  MOZ_ASSERT(NS_IsMainThread());

  AssertSourceIsCorrect();

  if (mSourceAsObjectStore) {
    aSource.SetValue().SetAsIDBObjectStore() = mSourceAsObjectStore;
  } else if (mSourceAsIndex) {
    aSource.SetValue().SetAsIDBIndex() = mSourceAsIndex;
  } else if (mSourceAsCursor) {
    aSource.SetValue().SetAsIDBCursor() = mSourceAsCursor;
  } else {
    aSource.SetNull();
  }
}

void
IDBRequest::Reset()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  mResultVal = JSVAL_VOID;
  mHaveResultOrErrorCode = false;
  mError = nullptr;
}

nsresult
IDBRequest::NotifyHelperCompleted(HelperBase* aHelper)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mHaveResultOrErrorCode, "Already called!");
  NS_ASSERTION(JSVAL_IS_VOID(mResultVal), "Should be undefined!");

  mHaveResultOrErrorCode = true;

  nsresult rv = aHelper->GetResultCode();

  
  if (NS_FAILED(rv)) {
    SetError(rv);
    return NS_OK;
  }

  
  
  if (NS_FAILED(CheckInnerWindowCorrectness())) {
    return NS_OK;
  }

  
  AutoPushJSContext cx(GetJSContext());
  if (!cx) {
    IDB_WARNING("Failed to get safe JSContext!");
    rv = NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR;
    SetError(rv);
    return rv;
  }

  JS::Rooted<JSObject*> global(cx, IDBWrapperCache::GetParentObject());
  NS_ASSERTION(global, "This should never be null!");

  JSAutoCompartment ac(cx, global);
  AssertIsRooted();

  JS::Rooted<JS::Value> value(cx);
  rv = aHelper->GetSuccessResult(cx, &value);
  if (NS_FAILED(rv)) {
    NS_WARNING("GetSuccessResult failed!");
  }

  if (NS_SUCCEEDED(rv)) {
    mError = nullptr;
    mResultVal = value;
  }
  else {
    SetError(rv);
    mResultVal = JSVAL_VOID;
  }

  return rv;
}

void
IDBRequest::NotifyHelperSentResultsToChildProcess(nsresult aRv)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mHaveResultOrErrorCode, "Already called!");
  NS_ASSERTION(JSVAL_IS_VOID(mResultVal), "Should be undefined!");

  
  
  if (NS_FAILED(CheckInnerWindowCorrectness())) {
    return;
  }

  mHaveResultOrErrorCode = true;

  if (NS_FAILED(aRv)) {
    SetError(aRv);
  }
}

void
IDBRequest::SetError(nsresult aRv)
{
  NS_ASSERTION(NS_FAILED(aRv), "Er, what?");
  NS_ASSERTION(!mError, "Already have an error?");

  mHaveResultOrErrorCode = true;
  mError = new mozilla::dom::DOMError(GetOwner(), aRv);
  mErrorCode = aRv;

  mResultVal = JSVAL_VOID;
}

#ifdef DEBUG
nsresult
IDBRequest::GetErrorCode() const
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mHaveResultOrErrorCode, "Don't call me yet!");
  return mErrorCode;
}
#endif

JSContext*
IDBRequest::GetJSContext()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  JSContext* cx;

  if (GetScriptOwner()) {
    return nsContentUtils::GetSafeJSContext();
  }

  nsresult rv;
  nsIScriptContext* sc = GetContextForEventHandlers(&rv);
  NS_ENSURE_SUCCESS(rv, nullptr);
  NS_ENSURE_TRUE(sc, nullptr);

  cx = sc->GetNativeContext();
  NS_ASSERTION(cx, "Failed to get a context!");

  return cx;
}

void
IDBRequest::CaptureCaller()
{
  AutoJSContext cx;

  const char* filename = nullptr;
  uint32_t lineNo = 0;
  if (!nsJSUtils::GetCallingLocation(cx, &filename, &lineNo)) {
    NS_WARNING("Failed to get caller.");
    return;
  }

  mFilename.Assign(NS_ConvertUTF8toUTF16(filename));
  mLineNo = lineNo;
}

void
IDBRequest::FillScriptErrorEvent(ErrorEventInit& aEventInit) const
{
  aEventInit.mLineno = mLineNo;
  aEventInit.mFilename = mFilename;
}

mozilla::dom::IDBRequestReadyState
IDBRequest::ReadyState() const
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (IsPending()) {
    return IDBRequestReadyState::Pending;
  }

  return IDBRequestReadyState::Done;
}

JSObject*
IDBRequest::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return IDBRequestBinding::Wrap(aCx, aScope, this);
}

JS::Value
IDBRequest::GetResult(JSContext* aCx, mozilla::ErrorResult& aRv) const
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mHaveResultOrErrorCode) {
    
    aRv.Throw(NS_ERROR_DOM_INDEXEDDB_NOT_ALLOWED_ERR);
  }

  return mResultVal;
}

mozilla::dom::DOMError*
IDBRequest::GetError(mozilla::ErrorResult& aRv)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  if (!mHaveResultOrErrorCode) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }

  return mError;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBRequest, IDBWrapperCache)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSourceAsObjectStore)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSourceAsIndex)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSourceAsCursor)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTransaction)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mError)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBRequest, IDBWrapperCache)
  tmp->mResultVal = JSVAL_VOID;
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSourceAsObjectStore)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSourceAsIndex)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSourceAsCursor)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mTransaction)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mError)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(IDBRequest, IDBWrapperCache)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRACE_JSVAL_MEMBER_CALLBACK(mResultVal)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBRequest)
NS_INTERFACE_MAP_END_INHERITING(IDBWrapperCache)

NS_IMPL_ADDREF_INHERITED(IDBRequest, IDBWrapperCache)
NS_IMPL_RELEASE_INHERITED(IDBRequest, IDBWrapperCache)

nsresult
IDBRequest::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  aVisitor.mCanHandle = true;
  aVisitor.mParentTarget = mTransaction;
  return NS_OK;
}

IDBOpenDBRequest::IDBOpenDBRequest(nsPIDOMWindow* aOwner)
  : IDBRequest(aOwner)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

IDBOpenDBRequest::~IDBOpenDBRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}


already_AddRefed<IDBOpenDBRequest>
IDBOpenDBRequest::Create(IDBFactory* aFactory,
                         nsPIDOMWindow* aOwner,
                         JS::Handle<JSObject*> aScriptOwner)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aFactory, "Null pointer!");

  nsRefPtr<IDBOpenDBRequest> request = new IDBOpenDBRequest(aOwner);

  request->SetScriptOwner(aScriptOwner);
  request->mFactory = aFactory;

  if (!aFactory->FromIPC()) {
    request->CaptureCaller();
  }

  return request.forget();
}

void
IDBOpenDBRequest::SetTransaction(IDBTransaction* aTransaction)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  NS_ASSERTION(!aTransaction || !mTransaction,
               "Shouldn't have a transaction here!");

  mTransaction = aTransaction;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(IDBOpenDBRequest)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBOpenDBRequest,
                                                  IDBRequest)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFactory)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBOpenDBRequest,
                                                IDBRequest)
  
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(IDBOpenDBRequest)
NS_INTERFACE_MAP_END_INHERITING(IDBRequest)

NS_IMPL_ADDREF_INHERITED(IDBOpenDBRequest, IDBRequest)
NS_IMPL_RELEASE_INHERITED(IDBOpenDBRequest, IDBRequest)

nsresult
IDBOpenDBRequest::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return IndexedDatabaseManager::FireWindowOnError(GetOwner(), aVisitor);
}

JSObject*
IDBOpenDBRequest::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return IDBOpenDBRequestBinding::Wrap(aCx, aScope, this);
}
