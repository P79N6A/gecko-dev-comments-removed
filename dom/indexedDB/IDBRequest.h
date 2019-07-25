







































#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBRequest.h"
#include "nsIVariant.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

class nsIScriptContext;
class nsPIDOMWindow;

BEGIN_INDEXEDDB_NAMESPACE

class AsyncConnectionHelper;
class IDBFactory;
class IDBDatabase;

class IDBRequest : public nsDOMEventTargetHelper,
                   public nsIIDBRequest
{
  friend class AsyncConnectionHelper;

public:
  class Generator : public nsISupports
  {
    protected:
      friend class IDBRequest;

      Generator() { }

      virtual ~Generator() {
        NS_ASSERTION(mLiveRequests.IsEmpty(), "Huh?!");
      }

      already_AddRefed<IDBRequest>
      GenerateRequest(nsIScriptContext* aScriptContext,
                      nsPIDOMWindow* aOwner) {
        return GenerateRequestInternal(aScriptContext, aOwner, PR_FALSE);
      }

      already_AddRefed<IDBRequest>
      GenerateWriteRequest(nsIScriptContext* aScriptContext,
                           nsPIDOMWindow* aOwner) {
        return GenerateRequestInternal(aScriptContext, aOwner, PR_TRUE);
      }

      void NoteDyingRequest(IDBRequest* aRequest) {
        NS_ASSERTION(mLiveRequests.Contains(aRequest), "Unknown request!");
        mLiveRequests.RemoveElement(aRequest);
      }

    private:
      already_AddRefed<IDBRequest>
      GenerateRequestInternal(nsIScriptContext* aScriptContext,
                              nsPIDOMWindow* aOwner,
                              PRBool aWriteRequest);

      
      nsAutoTArray<IDBRequest*, 1> mLiveRequests;
  };

  friend class IDBRequestGenerator;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIIDBREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBRequest,
                                           nsDOMEventTargetHelper)

  already_AddRefed<nsISupports> GetGenerator()
  {
    nsCOMPtr<nsISupports> generator(mGenerator);
    return generator.forget();
  }

private:
  
  IDBRequest()
  : mReadyState(nsIIDBRequest::INITIAL),
    mAborted(PR_FALSE),
    mWriteRequest(PR_FALSE)
  { }

  nsRefPtr<Generator> mGenerator;

protected:
  
  ~IDBRequest();

  nsRefPtr<nsDOMEventListenerWrapper> mOnSuccessListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;

  PRUint16 mReadyState;
  PRPackedBool mAborted;
  PRPackedBool mWriteRequest;
};

END_INDEXEDDB_NAMESPACE

#endif 
