







































#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBRequest.h"
#include "nsIIDBOpenDBRequest.h"
#include "nsDOMEventTargetHelper.h"
#include "mozilla/dom/indexedDB/IDBWrapperCache.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class HelperBase;
class IDBTransaction;

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
                                      IDBTransaction* aTransaction);

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  nsISupports* Source()
  {
    return mSource;
  }

  void Reset();

  nsresult NotifyHelperCompleted(HelperBase* aHelper);

  void SetError(nsresult rv)
  {
    NS_ASSERTION(NS_FAILED(rv), "Er, what?");
    NS_ASSERTION(mErrorCode == NS_OK, "Already have an error?");

    mErrorCode = rv;
  }

protected:
  IDBRequest();
  ~IDBRequest();

  virtual void RootResultValInternal();
  virtual void UnrootResultValInternal();

  void RootResultVal()
  {
    if (!mRooted) {
      RootResultValInternal();
      mRooted = true;
    }
  }

  void UnrootResultVal()
  {
    if (mRooted) {
      UnrootResultValInternal();
      mRooted = false;
    }
  }

  nsCOMPtr<nsISupports> mSource;
  nsRefPtr<IDBTransaction> mTransaction;

  NS_DECL_EVENT_HANDLER(success)
  NS_DECL_EVENT_HANDLER(error)

  jsval mResultVal;

  PRUint16 mErrorCode;
  bool mHaveResultOrErrorCode;
  bool mRooted;
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
  Create(nsIScriptContext* aScriptContext,
         nsPIDOMWindow* aOwner,
         JSObject* aScriptOwner);

  static
  already_AddRefed<IDBOpenDBRequest>
  Create(IDBWrapperCache* aOwnerCache)
  {
    return Create(aOwnerCache->GetScriptContext(), aOwnerCache->GetOwner(),
                  aOwnerCache->GetScriptOwner());
  }

  void SetTransaction(IDBTransaction* aTransaction);

protected:
  ~IDBOpenDBRequest();

  virtual void RootResultValInternal();
  virtual void UnrootResultValInternal();

  
  NS_DECL_EVENT_HANDLER(blocked)
  NS_DECL_EVENT_HANDLER(upgradeneeded)
};

END_INDEXEDDB_NAMESPACE

#endif
