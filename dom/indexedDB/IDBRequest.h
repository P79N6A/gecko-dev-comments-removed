





#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/IDBRequestBinding.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

#include "mozilla/dom/indexedDB/IDBWrapperCache.h"

class nsIScriptContext;
class nsPIDOMWindow;

namespace mozilla {
class EventChainPostVisitor;
class EventChainPreVisitor;
namespace dom {
class OwningIDBObjectStoreOrIDBIndexOrIDBCursor;
struct ErrorEventInit;
}
}

BEGIN_INDEXEDDB_NAMESPACE

class HelperBase;
class IDBCursor;
class IDBFactory;
class IDBIndex;
class IDBObjectStore;
class IDBTransaction;
class IndexedDBRequestParentBase;

class IDBRequest : public IDBWrapperCache
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(IDBRequest,
                                                         IDBWrapperCache)

  static
  already_AddRefed<IDBRequest> Create(IDBDatabase* aDatabase,
                                      IDBTransaction* aTransaction);

  static
  already_AddRefed<IDBRequest> Create(IDBObjectStore* aSource,
                                      IDBDatabase* aDatabase,
                                      IDBTransaction* aTransaction);

  static
  already_AddRefed<IDBRequest> Create(IDBIndex* aSource,
                                      IDBDatabase* aDatabase,
                                      IDBTransaction* aTransaction);

  
  virtual nsresult PreHandleEvent(EventChainPreVisitor& aVisitor) MOZ_OVERRIDE;

  void GetSource(Nullable<OwningIDBObjectStoreOrIDBIndexOrIDBCursor>& aSource) const;

  void Reset();

  nsresult NotifyHelperCompleted(HelperBase* aHelper);
  void NotifyHelperSentResultsToChildProcess(nsresult aRv);

  void SetError(nsresult aRv);

  nsresult
  GetErrorCode() const
#ifdef DEBUG
  ;
#else
  {
    return mErrorCode;
  }
#endif

  DOMError* GetError(ErrorResult& aRv);

  void
  SetActor(IndexedDBRequestParentBase* aActorParent)
  {
    NS_ASSERTION(!aActorParent || !mActorParent,
                 "Shouldn't have more than one!");
    mActorParent = aActorParent;
  }

  IndexedDBRequestParentBase*
  GetActorParent() const
  {
    return mActorParent;
  }

  void CaptureCaller();

  void FillScriptErrorEvent(ErrorEventInit& aEventInit) const;

  bool
  IsPending() const
  {
    return !mHaveResultOrErrorCode;
  }

#ifdef MOZ_ENABLE_PROFILER_SPS
  uint64_t
  GetSerialNumber() const
  {
    return mSerialNumber;
  }
#endif

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  nsPIDOMWindow*
  GetParentObject() const
  {
    return GetOwner();
  }

  void
  GetResult(JS::MutableHandle<JS::Value> aResult, ErrorResult& aRv) const;

  void
  GetResult(JSContext* aCx, JS::MutableHandle<JS::Value> aResult,
            ErrorResult& aRv) const
  {
    GetResult(aResult, aRv);
  }

  IDBTransaction*
  GetTransaction() const
  {
    NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
    return mTransaction;
  }

  IDBRequestReadyState
  ReadyState() const;

  IMPL_EVENT_HANDLER(success);
  IMPL_EVENT_HANDLER(error);

protected:
  explicit IDBRequest(IDBDatabase* aDatabase);
  explicit IDBRequest(nsPIDOMWindow* aOwner);
  ~IDBRequest();

  
  nsRefPtr<IDBObjectStore> mSourceAsObjectStore;
  nsRefPtr<IDBIndex> mSourceAsIndex;
  nsRefPtr<IDBCursor> mSourceAsCursor;

  
#ifdef DEBUG
  void AssertSourceIsCorrect() const;
#else
  void AssertSourceIsCorrect() const {}
#endif

  nsRefPtr<IDBTransaction> mTransaction;

  JS::Heap<JS::Value> mResultVal;
  nsRefPtr<mozilla::dom::DOMError> mError;
  IndexedDBRequestParentBase* mActorParent;
  nsString mFilename;
#ifdef MOZ_ENABLE_PROFILER_SPS
  uint64_t mSerialNumber;
#endif
  nsresult mErrorCode;
  uint32_t mLineNo;
  bool mHaveResultOrErrorCode;
};

class IDBOpenDBRequest : public IDBRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBOpenDBRequest, IDBRequest)

  static
  already_AddRefed<IDBOpenDBRequest>
  Create(IDBFactory* aFactory,
         nsPIDOMWindow* aOwner,
         JS::Handle<JSObject*> aScriptOwner);

  void SetTransaction(IDBTransaction* aTransaction);

  
  virtual nsresult PostHandleEvent(
                     EventChainPostVisitor& aVisitor) MOZ_OVERRIDE;

  DOMError* GetError(ErrorResult& aRv)
  {
    return IDBRequest::GetError(aRv);
  }

  IDBFactory*
  Factory() const
  {
    return mFactory;
  }

  
  virtual JSObject*
  WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  
  IMPL_EVENT_HANDLER(blocked);
  IMPL_EVENT_HANDLER(upgradeneeded);

protected:
  explicit IDBOpenDBRequest(nsPIDOMWindow* aOwner);
  ~IDBOpenDBRequest();

  
  nsRefPtr<IDBFactory> mFactory;
};

END_INDEXEDDB_NAMESPACE

#endif
