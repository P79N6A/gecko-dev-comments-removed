





#ifndef mozilla_dom_DataStoreRevision_h
#define mozilla_dom_DataStoreRevision_h

#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsIDOMEventListener.h"
#include "nsString.h"

namespace mozilla {
namespace dom {

namespace indexedDB {
class IDBObjectStore;
class IDBRequest;
}

class DataStoreRevisionCallback;

class DataStoreRevision final : public nsIDOMEventListener
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

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) override;

private:
  ~DataStoreRevision() {}
  nsRefPtr<DataStoreRevisionCallback> mCallback;
  nsRefPtr<indexedDB::IDBRequest> mRequest;
  nsString mRevisionID;
};

} 
} 

#endif 
