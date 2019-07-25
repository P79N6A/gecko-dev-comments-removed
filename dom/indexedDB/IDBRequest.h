







































#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBRequest.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

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
                                      nsPIDOMWindow* aOwner);

  already_AddRefed<nsISupports> Source()
  {
    nsCOMPtr<nsISupports> source(mSource);
    return source.forget();
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
  IDBRequest()
  : mReadyState(nsIIDBRequest::LOADING)
  { }

  ~IDBRequest()
  {
    if (mListenerManager) {
      mListenerManager->Disconnect();
    }
  }

  nsCOMPtr<nsISupports> mSource;

  nsRefPtr<nsDOMEventListenerWrapper> mOnSuccessListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;

  PRUint16 mReadyState;
};

END_INDEXEDDB_NAMESPACE

#endif 
