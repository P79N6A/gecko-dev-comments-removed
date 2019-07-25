







































#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBRequest.h"
#include "nsIIDBOpenDBRequest.h"

#include "nsDOMEventTargetWrapperCache.h"
#include "nsCycleCollectionParticipant.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class HelperBase;
class IDBTransaction;

class IDBRequest : public nsDOMEventTargetHelper,
                   public nsIIDBRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBREQUEST
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(IDBRequest,
                                                         nsDOMEventTargetHelper)

  static
  already_AddRefed<IDBRequest> Create(nsISupports* aSource,
                                      nsIScriptContext* aScriptContext,
                                      nsPIDOMWindow* aOwner,
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

  nsIScriptContext* ScriptContext()
  {
    NS_ASSERTION(mScriptContext, "This should never be null!");
    return mScriptContext;
  }

  nsPIDOMWindow* Owner()
  {
    NS_ASSERTION(mOwner, "This should never be null!");
    return mOwner;
  }

  virtual void RootResultVal();
  virtual void UnrootResultVal();

protected:
  IDBRequest();
  ~IDBRequest();

  nsCOMPtr<nsISupports> mSource;
  nsRefPtr<IDBTransaction> mTransaction;

  nsRefPtr<nsDOMEventListenerWrapper> mOnSuccessListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;

  jsval mResultVal;

  PRUint16 mErrorCode;
  bool mResultValRooted;
  bool mHaveResultOrErrorCode;
};

class IDBOpenDBRequest : public IDBRequest,
                         public nsIIDBOpenDBRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIIDBREQUEST(IDBRequest::)
  NS_DECL_NSIIDBOPENDBREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBOpenDBRequest,
                                           IDBRequest)

  static
  already_AddRefed<IDBOpenDBRequest>
  Create(nsIScriptContext* aScriptContext,
         nsPIDOMWindow* aOwner);

  void SetTransaction(IDBTransaction* aTransaction);

  virtual void RootResultVal();
  virtual void UnrootResultVal();

protected:
  ~IDBOpenDBRequest();

  nsRefPtr<nsDOMEventListenerWrapper> mOnblockedListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnupgradeneededListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
