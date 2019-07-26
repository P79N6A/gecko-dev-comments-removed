





#include "IDBRequest.h"

#include "nsIScriptContext.h"

#include "mozilla/dom/IDBOpenDBRequestBinding.h"
#include "nsComponentManagerUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsDOMJSUtils.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsEventDispatcher.h"
#include "nsJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsStringGlue.h"
#include "nsThreadUtils.h"
#include "nsWrapperCacheInlines.h"

#include "AsyncConnectionHelper.h"
#include "IDBEvents.h"
#include "IDBFactory.h"
#include "IDBTransaction.h"

namespace {

#ifdef MOZ_ENABLE_PROFILER_SPS
uint64_t gNextSerialNumber = 1;
#endif

} 

USING_INDEXEDDB_NAMESPACE

IDBRequest::IDBRequest()
: mResultVal(JSVAL_VOID),
  mActorParent(nullptr),
#ifdef MOZ_ENABLE_PROFILER_SPS
  mSerialNumber(gNextSerialNumber++),
#endif
  mErrorCode(NS_OK),
  mLineNo(0),
  mHaveResultOrErrorCode(false)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  SetIsDOMBinding();
}

IDBRequest::~IDBRequest()
{
  mResultVal = JSVAL_VOID;
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}


already_AddRefed<IDBRequest>
IDBRequest::Create(nsISupports* aSource,
                   IDBWrapperCache* aOwnerCache,
                   IDBTransaction* aTransaction)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  nsRefPtr<IDBRequest> request(new IDBRequest());

  request->mSource = aSource;
  request->mTransaction = aTransaction;
  request->BindToOwner(aOwnerCache);
  request->SetScriptOwner(aOwnerCache->GetScriptOwner());
  request->CaptureCaller();

  return request.forget();
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
    NS_WARNING("Failed to get safe JSContext!");
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
IDBRequest::FillScriptErrorEvent(nsScriptErrorEvent* aEvent) const
{
  aEvent->lineNr = mLineNo;
  aEvent->fileName = mFilename.get();
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

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(IDBRequest, IDBWrapperCache)
  
  
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mSource)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTransaction)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mError)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(IDBRequest, IDBWrapperCache)
  tmp->mResultVal = JSVAL_VOID;
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mSource)
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

IDBOpenDBRequest::IDBOpenDBRequest()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  SetIsDOMBinding();
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

  nsRefPtr<IDBOpenDBRequest> request = new IDBOpenDBRequest();

  request->BindToOwner(aOwner);
  request->SetScriptOwner(aScriptOwner);
  request->CaptureCaller();
  request->mFactory = aFactory;

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
