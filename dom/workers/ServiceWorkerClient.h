





#ifndef mozilla_dom_workers_serviceworkerclient_h
#define mozilla_dom_workers_serviceworkerclient_h

#include "nsCOMPtr.h"
#include "nsWrapperCache.h"

namespace mozilla {
namespace dom {

class Promise;

namespace workers {

class ServiceWorkerClient MOZ_FINAL : public nsISupports,
                                      public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ServiceWorkerClient)

  ServiceWorkerClient(nsISupports* aOwner, uint64_t aId)
    : mOwner(aOwner),
      mId(aId)
  {
    SetIsDOMBinding();
  }

  uint32_t Id() const
  {
    return mId;
  }

  nsISupports* GetParentObject() const
  {
    return mOwner;
  }

  JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

private:
  ~ServiceWorkerClient()
  {
  }

  nsCOMPtr<nsISupports> mOwner;
  uint64_t mId;
};

} 
} 
} 

#endif 
