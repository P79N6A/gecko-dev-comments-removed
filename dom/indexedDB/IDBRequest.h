







































#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBRequest.h"
#include "nsIIDBVersionChangeRequest.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class IDBTransaction;

class IDBRequest : public nsDOMEventTargetHelper,
                   public nsIIDBRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBRequest,
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

  void Reset()
  {
    mReadyState = nsIIDBRequest::LOADING;
  }

  void SetDone()
  {
    NS_ASSERTION(mReadyState != nsIIDBRequest::DONE, "Already set!");
    mReadyState = nsIIDBRequest::DONE;
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

protected:
  IDBRequest();
  ~IDBRequest();

  nsCOMPtr<nsISupports> mSource;
  nsRefPtr<IDBTransaction> mTransaction;

  nsRefPtr<nsDOMEventListenerWrapper> mOnSuccessListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;

  PRUint16 mReadyState;
};

class IDBVersionChangeRequest : public IDBRequest,
                                public nsIIDBVersionChangeRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIIDBREQUEST(IDBRequest::)
  NS_DECL_NSIIDBVERSIONCHANGEREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBVersionChangeRequest,
                                           IDBRequest)

  static
  already_AddRefed<IDBVersionChangeRequest>
  Create(nsISupports* aSource,
         nsIScriptContext* aScriptContext,
         nsPIDOMWindow* aOwner,
         IDBTransaction* aTransaction);

protected:
  nsRefPtr<nsDOMEventListenerWrapper> mOnBlockedListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
