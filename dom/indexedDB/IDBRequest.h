







































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

class AsyncConnectionHelper;
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

  nsresult SetDone(AsyncConnectionHelper* aHelper);

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

class IDBVersionChangeRequest : public IDBRequest,
                                public nsIIDBVersionChangeRequest
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_NSIIDBREQUEST(IDBRequest::)
  NS_DECL_NSIIDBVERSIONCHANGEREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBVersionChangeRequest,
                                           IDBRequest)

  ~IDBVersionChangeRequest();

  static
  already_AddRefed<IDBVersionChangeRequest>
  Create(nsISupports* aSource,
         nsIScriptContext* aScriptContext,
         nsPIDOMWindow* aOwner,
         IDBTransaction* aTransaction);

  virtual void RootResultVal();
  virtual void UnrootResultVal();

protected:
  nsRefPtr<nsDOMEventListenerWrapper> mOnBlockedListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
