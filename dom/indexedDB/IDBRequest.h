







































#ifndef mozilla_dom_indexeddb_idbrequest_h__
#define mozilla_dom_indexeddb_idbrequest_h__

#include "mozilla/dom/indexedDB/IndexedDatabase.h"

#include "nsIIDBRequest.h"
#include "nsIVariant.h"

#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

class nsIIDBDatabaseError;

BEGIN_INDEXEDDB_NAMESPACE

class IndexedDatabaseRequest;
class IDBDatabaseRequest;

class IDBRequest : public nsDOMEventTargetHelper,
                   public nsIIDBRequest
{
public:
  class Generator : public nsISupports
  {
    protected:
      friend class IDBRequest;

      Generator() { }

      virtual ~Generator() {
        PRUint32 count = mLiveRequests.Length();
        if (count) {
          nsTArray<IDBRequest*> requests(mLiveRequests);
          NS_ASSERTION(count == requests.Length(), "Copy failed!");
          for (PRUint32 index = 0; index < count; index++) {
            requests[index]->NoteDyingGenerator();
          }
        }
      }

      IDBRequest* GenerateRequest() {
        IDBRequest* request = new IDBRequest(this);
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

  NS_DECL_ISUPPORTS
  NS_DECL_NSIIDBREQUEST
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IDBRequest,
                                           nsDOMEventTargetHelper)

private:
  
  IDBRequest(Generator* aGenerator);

  nsRefPtr<Generator> mGenerator;

protected:
  
  ~IDBRequest();

  virtual void NoteDyingGenerator() {
    Abort();
  }

  PRUint16 mReadyState;
  nsRefPtr<nsDOMEventListenerWrapper> mOnSuccessListener;
  nsRefPtr<nsDOMEventListenerWrapper> mOnErrorListener;
};

END_INDEXEDDB_NAMESPACE

#endif 
