





#ifndef mozilla_dom_DataStoreRevision_h
#define mozilla_dom_DataStoreRevision_h

#include "nsIDOMEventListener.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "jsapi.h"

namespace mozilla {
namespace dom {

namespace indexedDB {
class IDBObjectStore;
class IDBRequest;
}

class DataStoreRevisionCallback;

class DataStoreRevision MOZ_FINAL : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS

  enum RevisionType {
    RevisionVoid
  };

  nsresult AddRevision(JSContext* aCx,
                       indexedDB::IDBObjectStore* aStore,
                       uint32_t aObjectId,
                       RevisionType aRevisionType,
                       DataStoreRevisionCallback* aCallback);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

private:
  nsRefPtr<DataStoreRevisionCallback> mCallback;
  nsRefPtr<indexedDB::IDBRequest> mRequest;
  nsString mRevisionID;
};

} 
} 

#endif 
