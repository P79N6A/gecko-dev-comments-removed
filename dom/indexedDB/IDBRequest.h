





#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBRequest.h"
#include "nsIIDBOpenDBRequest.h"
#include "nsDOMEventTargetHelper.h"
#include "mozilla/dom/indexedDB/IDBWrapperCache.h"
#include "mozilla/dom/DOMError.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class HelperBase;
class IDBFactory;
class IDBTransaction;
class IndexedDBRequestParentBase;

class IDBRequest : public IDBWrapperCache,
                   public nsIIDBRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBREQUEST
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(IDBRequest,
                                                         IDBWrapperCache)

  static
  already_AddRefed<IDBRequest> Create(nsISupports* aSource,
                                      IDBWrapperCache* aOwnerCache,
                                      IDBTransaction* aTransaction,
                                      JSContext* aCallingCx);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  nsISupports* Source()
  {
    return mSource;
  }

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

  JSContext* GetJSContext();

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

  void CaptureCaller(JSContext* aCx);

  void FillScriptErrorEvent(nsScriptErrorEvent* aEvent) const;

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

protected:
  IDBRequest();
  ~IDBRequest();

  nsCOMPtr<nsISupports> mSource;
  nsRefPtr<IDBTransaction> mTransaction;

  jsval mResultVal;
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

class IDBOpenDBRequest : public IDBRequest,
                         public nsIIDBOpenDBRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIIDBREQUEST(IDBRequest::)
  NS_DECL_NSIIDBOPENDBREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBOpenDBRequest, IDBRequest)

  static
  already_AddRefed<IDBOpenDBRequest>
  Create(IDBFactory* aFactory,
         nsPIDOMWindow* aOwner,
         JS::Handle<JSObject*> aScriptOwner,
         JSContext* aCallingCx);

  void SetTransaction(IDBTransaction* aTransaction);

  
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  DOMError* GetError(ErrorResult& aRv)
  {
    return IDBRequest::GetError(aRv);
  }

  IDBFactory*
  Factory() const
  {
    return mFactory;
  }

protected:
  ~IDBOpenDBRequest();

  
  nsRefPtr<IDBFactory> mFactory;
};

END_INDEXEDDB_NAMESPACE

#endif
