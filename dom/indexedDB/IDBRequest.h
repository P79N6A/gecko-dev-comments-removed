







































#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBRequest.h"
#include "nsIVariant.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

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

      IDBRequest* GenerateRequest() {
        IDBRequest* request = new IDBRequest(this, false);
        if (!mLiveRequests.AppendElement(request)) {
          NS_ERROR("Append failed!");
        }
        return request;
      }

      IDBRequest* GenerateWriteRequest() {
        IDBRequest* request = new IDBRequest(this, true);
        if (!mLiveRequests.AppendElement(request)) {
          NS_ERROR("Append failed!");
        }
        return request;
      }

      void NoteDyingRequest(IDBRequest* aRequest) {
        NS_ASSERTION(mLiveRequests.Contains(aRequest), "Unknown request!");
        mLiveRequests.RemoveElement(aRequest);
      }

    private:
      
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
  
  IDBRequest(Generator* aGenerator,
             bool aWriteRequest);

  nsRefPtr<Generator> mGenerator;

protected:
  
  ~IDBRequest();

  PRUint16 mReadyState;
  PRBool mAborted;
  PRBool mWriteRequest;
  nsRefPtr<nsDOMEventListenerWrapper> mOnSuccessListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
