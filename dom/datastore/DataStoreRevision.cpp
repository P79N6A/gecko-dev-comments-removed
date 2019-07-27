





#include "DataStoreRevision.h"

#include "DataStoreCallbacks.h"
#include "DataStoreService.h"
#include "mozilla/dom/DataStoreBinding.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/dom/indexedDB/IDBObjectStore.h"
#include "mozilla/dom/indexedDB/IDBRequest.h"
#include "nsIDOMEvent.h"

namespace mozilla {
namespace dom {

using namespace indexedDB;

NS_IMPL_ISUPPORTS(DataStoreRevision, nsIDOMEventListener)



nsresult
DataStoreRevision::AddRevision(JSContext* aCx,
                               IDBObjectStore* aStore,
                               uint32_t aObjectId,
                               RevisionType aRevisionType,
                               DataStoreRevisionCallback* aCallback)
{
  MOZ_ASSERT(aStore);
  MOZ_ASSERT(aCallback);

  nsRefPtr<DataStoreService> service = DataStoreService::Get();
  if (!service) {
    return NS_ERROR_FAILURE;
  }

  nsString id;
  nsresult rv = service->GenerateUUID(mRevisionID);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  DataStoreRevisionData data;
  data.mRevisionId = mRevisionID;
  data.mObjectId = aObjectId;

  switch (aRevisionType) {
    case RevisionVoid:
      data.mOperation = NS_LITERAL_STRING("void");
      break;

    default:
      MOZ_CRASH("This should not happen");
  }

  JS::Rooted<JS::Value> value(aCx);
  if (!ToJSValue(aCx, data, &value)) {
    return NS_ERROR_FAILURE;
  }

  ErrorResult error;
  mRequest = aStore->Put(aCx, value, JS::UndefinedHandleValue, error);
  if (NS_WARN_IF(error.Failed())) {
    return error.StealNSResult();
  }

  rv = mRequest->EventTarget::AddEventListener(NS_LITERAL_STRING("success"),
                                               this, false);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  mCallback = aCallback;
  return NS_OK;
}

NS_IMETHODIMP
DataStoreRevision::HandleEvent(nsIDOMEvent* aEvent)
{
  nsString type;
  nsresult rv = aEvent->GetType(type);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!type.EqualsASCII("success")) {
    MOZ_CRASH("This should not happen");
  }

  mRequest->RemoveEventListener(NS_LITERAL_STRING("success"), this, false);
  mRequest = nullptr;

  mCallback->Run(mRevisionID);
  return NS_OK;
}

} 
} 
