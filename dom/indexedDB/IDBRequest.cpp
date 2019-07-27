





#include "IDBRequest.h"

#include "BackgroundChildImpl.h"
#include "IDBCursor.h"
#include "IDBDatabase.h"
#include "IDBEvents.h"
#include "IDBFactory.h"
#include "IDBIndex.h"
#include "IDBObjectStore.h"
#include "IDBTransaction.h"
#include "mozilla/ContentEvents.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/ErrorEventBinding.h"
#include "mozilla/dom/IDBOpenDBRequestBinding.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/UnionTypes.h"
#include "nsCOMPtr.h"
#include "nsContentUtils.h"
#include "nsIScriptContext.h"
#include "nsJSUtils.h"
#include "nsPIDOMWindow.h"
#include "nsString.h"
#include "ReportInternalError.h"

namespace mozilla {
namespace dom {
namespace indexedDB {

using namespace mozilla::ipc;

IDBRequest::IDBRequest(IDBDatabase* aDatabase)
  : IDBWrapperCache(aDatabase)
{
  MOZ_ASSERT(aDatabase);
  aDatabase->AssertIsOnOwningThread();

  InitMembers();
}

IDBRequest::IDBRequest(nsPIDOMWindow* aOwner)
  : IDBWrapperCache(aOwner)
{
  InitMembers();
}

IDBRequest::~IDBRequest()
{
  AssertIsOnOwningThread();
}

#ifdef DEBUG

void
IDBRequest::AssertIsOnOwningThread() const
{
  MOZ_ASSERT(mOwningThread);
  MOZ_ASSERT(PR_GetCurrentThread() == mOwningThread);
}

#endif 

void
IDBRequest::InitMembers()
{
#ifdef DEBUG
  mOwningThread = PR_GetCurrentThread();
#endif
  AssertIsOnOwningThread();

  mResultVal.setUndefined();
  mErrorCode = NS_OK;
  mLineNo = 0;
  mHaveResultOrErrorCode = false;

#ifdef MOZ_ENABLE_PROFILER_SPS
  {
    BackgroundChildImpl::ThreadLocal* threadLocal =
      BackgroundChildImpl::GetThreadLocalForCurrentThread();
    MOZ_ASSERT(threadLocal);

    mSerialNumber = threadLocal->mNextRequestSerialNumber++;
  }
#endif
}


already_AddRefed<IDBRequest>
IDBRequest::Create(IDBDatabase* aDatabase,
                   IDBTransaction* aTransaction)
{
  MOZ_ASSERT(aDatabase);
  aDatabase->AssertIsOnOwningThread();

  nsRefPtr<IDBRequest> request = new IDBRequest(aDatabase);

  request->mTransaction = aTransaction;
  request->SetScriptOwner(aDatabase->GetScriptOwner());

  request->CaptureCaller();

  return request.forget();
}


already_AddRefed<IDBRequest>
IDBRequest::Create(IDBObjectStore* aSourceAsObjectStore,
                   IDBDatabase* aDatabase,
                   IDBTransaction* aTransaction)
{
  MOZ_ASSERT(aSourceAsObjectStore);
  aSourceAsObjectStore->AssertIsOnOwningThread();

  nsRefPtr<IDBRequest> request = Create(aDatabase, aTransaction);

  request->mSourceAsObjectStore = aSourceAsObjectStore;

  return request.forget();
}


already_AddRefed<IDBRequest>
IDBRequest::Create(IDBIndex* aSourceAsIndex,
                   IDBDatabase* aDatabase,
                   IDBTransaction* aTransaction)
{
  MOZ_ASSERT(aSourceAsIndex);
  aSourceAsIndex->AssertIsOnOwningThread();

  nsRefPtr<IDBRequest> request = Create(aDatabase, aTransaction);

  request->mSourceAsIndex = aSourceAsIndex;

  return request.forget();
}

void
IDBRequest::GetSource(
             Nullable<OwningIDBObjectStoreOrIDBIndexOrIDBCursor>& aSource) const
{
  AssertIsOnOwningThread();

  MOZ_ASSERT_IF(mSourceAsObjectStore, !mSourceAsIndex);
  MOZ_ASSERT_IF(mSourceAsIndex, !mSourceAsObjectStore);
  MOZ_ASSERT_IF(mSourceAsCursor, mSourceAsObjectStore || mSourceAsIndex);

  
  
  if (mSourceAsCursor) {
    aSource.SetValue().SetAsIDBCursor() = mSourceAsCursor;
  } else if (mSourceAsObjectStore) {
    aSource.SetValue().SetAsIDBObjectStore() = mSourceAsObjectStore;
  } else if (mSourceAsIndex) {
    aSource.SetValue().SetAsIDBIndex() = mSourceAsIndex;
  } else {
    aSource.SetNull();
  }
}

void
IDBRequest::Reset()
{
  AssertIsOnOwningThread();

  mResultVal.setUndefined();
  mHaveResultOrErrorCode = false;
  mError = nullptr;
}

void
IDBRequest::DispatchNonTransactionError(nsresult aErrorCode)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(NS_FAILED(aErrorCode));
  MOZ_ASSERT(NS_ERROR_GET_MODULE(aErrorCode) == NS_ERROR_MODULE_DOM_INDEXEDDB);

  SetError(aErrorCode);

  
  nsCOMPtr<nsIDOMEvent> event =
    CreateGenericEvent(this,
                       nsDependentString(kErrorEventType),
                       eDoesBubble,
                       eCancelable);
  if (NS_WARN_IF(!event)) {
    return;
  }

  bool ignored;
  NS_WARN_IF(NS_FAILED(DispatchEvent(event, &ignored)));
}

void
IDBRequest::SetError(nsresult aRv)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(NS_FAILED(aRv));
  MOZ_ASSERT(NS_ERROR_GET_MODULE(aRv) == NS_ERROR_MODULE_DOM_INDEXEDDB);
  MOZ_ASSERT(!mError);

  mHaveResultOrErrorCode = true;
  mError = new DOMError(GetOwner(), aRv);
  mErrorCode = aRv;

  mResultVal.setUndefined();
}

#ifdef DEBUG

nsresult
IDBRequest::GetErrorCode() const
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(mHaveResultOrErrorCode);

  return mErrorCode;
}

#endif 

void
IDBRequest::CaptureCaller()
{
  AutoJSContext cx;

  const char* filename = nullptr;
  uint32_t lineNo = 0;
  if (!nsJSUtils::GetCallingLocation(cx, &filename, &lineNo)) {
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

IDBRequestReadyState
IDBRequest::ReadyState() const
{
  AssertIsOnOwningThread();

  return IsPending() ?
    IDBRequestReadyState::Pending :
    IDBRequestReadyState::Done;
}

void
IDBRequest::SetSource(IDBCursor* aSource)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aSource);
  MOZ_ASSERT(mSourceAsObjectStore || mSourceAsIndex);
  MOZ_ASSERT(!mSourceAsCursor);

  mSourceAsCursor = aSource;
}

JSObject*
IDBRequest::WrapObject(JSContext* aCx)
{
  return IDBRequestBinding::Wrap(aCx, this);
}

void
IDBRequest::GetResult(JS::MutableHandle<JS::Value> aResult,
                      ErrorResult& aRv) const
{
  AssertIsOnOwningThread();

  if (!mHaveResultOrErrorCode) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  JS::ExposeValueToActiveJS(mResultVal);
  aResult.set(mResultVal);
}

void
IDBRequest::SetResultCallback(ResultCallback* aCallback)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aCallback);
  MOZ_ASSERT(!mHaveResultOrErrorCode);
  MOZ_ASSERT(mResultVal.isUndefined());
  MOZ_ASSERT(!mError);

  
  if (NS_WARN_IF(NS_FAILED(CheckInnerWindowCorrectness()))) {
    IDB_REPORT_INTERNAL_ERR();
    SetError(NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
    return;
  }

  AutoJSAPI autoJS;
  Maybe<JSAutoCompartment> ac;

  if (GetScriptOwner()) {
    
    
    autoJS.Init();
    ac.emplace(autoJS.cx(), GetScriptOwner());
  } else {
    
    MOZ_ASSERT(GetOwner());
    if (!autoJS.InitWithLegacyErrorReporting(GetOwner())) {
      IDB_WARNING("Failed to initialize AutoJSAPI!");
      SetError(NS_ERROR_DOM_INDEXEDDB_UNKNOWN_ERR);
      return;
    }
  }

  JSContext* cx = autoJS.cx();

  AssertIsRooted();

  JS::Rooted<JS::Value> result(cx);
  nsresult rv = aCallback->GetResult(cx, &result);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    SetError(rv);
    mResultVal.setUndefined();
  } else {
    mError = nullptr;
    mResultVal = result;
  }

  mHaveResultOrErrorCode = true;
}

DOMError*
IDBRequest::GetError(ErrorResult& aRv)
{
  AssertIsOnOwningThread();

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
  tmp->mResultVal.setUndefined();
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
IDBRequest::PreHandleEvent(EventChainPreVisitor& aVisitor)
{
  AssertIsOnOwningThread();

  aVisitor.mCanHandle = true;
  aVisitor.mParentTarget = mTransaction;
  return NS_OK;
}

IDBOpenDBRequest::IDBOpenDBRequest(IDBFactory* aFactory, nsPIDOMWindow* aOwner)
  : IDBRequest(aOwner)
  , mFactory(aFactory)
{
  AssertIsOnOwningThread();
  MOZ_ASSERT(aFactory);

  
}

IDBOpenDBRequest::~IDBOpenDBRequest()
{
  AssertIsOnOwningThread();
}


already_AddRefed<IDBOpenDBRequest>
IDBOpenDBRequest::CreateForWindow(IDBFactory* aFactory,
                                  nsPIDOMWindow* aOwner,
                                  JS::Handle<JSObject*> aScriptOwner)
{
  MOZ_ASSERT(aFactory);
  aFactory->AssertIsOnOwningThread();
  MOZ_ASSERT(aOwner);
  MOZ_ASSERT(aScriptOwner);

  nsRefPtr<IDBOpenDBRequest> request = new IDBOpenDBRequest(aFactory, aOwner);
  request->CaptureCaller();

  request->SetScriptOwner(aScriptOwner);

  return request.forget();
}


already_AddRefed<IDBOpenDBRequest>
IDBOpenDBRequest::CreateForJS(IDBFactory* aFactory,
                              JS::Handle<JSObject*> aScriptOwner)
{
  MOZ_ASSERT(aFactory);
  aFactory->AssertIsOnOwningThread();
  MOZ_ASSERT(aScriptOwner);

  nsRefPtr<IDBOpenDBRequest> request = new IDBOpenDBRequest(aFactory, nullptr);
  request->CaptureCaller();

  request->SetScriptOwner(aScriptOwner);

  return request.forget();
}

void
IDBOpenDBRequest::SetTransaction(IDBTransaction* aTransaction)
{
  AssertIsOnOwningThread();

  MOZ_ASSERT(!aTransaction || !mTransaction);

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
IDBOpenDBRequest::PostHandleEvent(EventChainPostVisitor& aVisitor)
{
  
  MOZ_ASSERT(NS_IsMainThread());

  return IndexedDatabaseManager::FireWindowOnError(GetOwner(), aVisitor);
}

JSObject*
IDBOpenDBRequest::WrapObject(JSContext* aCx)
{
  AssertIsOnOwningThread();

  return IDBOpenDBRequestBinding::Wrap(aCx, this);
}

} 
} 
} 
