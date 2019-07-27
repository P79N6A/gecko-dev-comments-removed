





#ifndef mozilla_dom_workers_serviceworkerclient_h
#define mozilla_dom_workers_serviceworkerclient_h

#include "nsCOMPtr.h"
#include "nsWrapperCache.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/ClientBinding.h"

namespace mozilla {
namespace dom {
namespace workers {

class ServiceWorkerClient;
class ServiceWorkerWindowClient;



class ServiceWorkerClientInfo final
{
  friend class ServiceWorkerClient;
  friend class ServiceWorkerWindowClient;

public:
  explicit ServiceWorkerClientInfo(nsIDocument* aDoc);

private:
  nsString mClientId;
  uint64_t mWindowId;
  nsString mUrl;

  
  VisibilityState mVisibilityState;
  bool mFocused;
  FrameType mFrameType;
};

class ServiceWorkerClient : public nsISupports,
                            public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(ServiceWorkerClient)

  ServiceWorkerClient(nsISupports* aOwner,
                      const ServiceWorkerClientInfo& aClientInfo)
    : mOwner(aOwner)
    , mId(aClientInfo.mClientId)
    , mUrl(aClientInfo.mUrl)
    , mWindowId(aClientInfo.mWindowId)
  {
    MOZ_ASSERT(aOwner);
  }

  nsISupports*
  GetParentObject() const
  {
    return mOwner;
  }

  void GetId(nsString& aRetval) const
  {
    aRetval = mId;
  }

  void
  GetUrl(nsAString& aUrl) const
  {
    aUrl.Assign(mUrl);
  }

  void
  PostMessage(JSContext* aCx, JS::Handle<JS::Value> aMessage,
              const Optional<Sequence<JS::Value>>& aTransferable,
              ErrorResult& aRv);

  JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

protected:
  virtual ~ServiceWorkerClient()
  { }

private:
  nsCOMPtr<nsISupports> mOwner;
  nsString mId;
  nsString mUrl;

protected:
  uint64_t mWindowId;
};

} 
} 
} 

#endif 
