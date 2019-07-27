





#ifndef mozilla_dom_workers_serviceworkerwindowclient_h
#define mozilla_dom_workers_serviceworkerwindowclient_h

#include "ServiceWorkerClient.h"

namespace mozilla {
namespace dom {
namespace workers {

class ServiceWorkerWindowClient final : public ServiceWorkerClient
{
public:
  ServiceWorkerWindowClient(nsISupports* aOwner,
                            const ServiceWorkerClientInfo& aClientInfo)
    : ServiceWorkerClient(aOwner, aClientInfo),
      mVisibilityState(aClientInfo.mVisibilityState),
      mFocused(aClientInfo.mFocused),
      mFrameType(aClientInfo.mFrameType)
  {
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  mozilla::dom::VisibilityState
  VisibilityState() const
  {
    return mVisibilityState;
  }

  bool
  Focused() const
  {
    return mFocused;
  }

  mozilla::dom::FrameType
  FrameType() const
  {
    return mFrameType;
  }

  already_AddRefed<Promise>
  Focus(ErrorResult& aRv) const;

private:
  ~ServiceWorkerWindowClient()
  { }

  mozilla::dom::VisibilityState mVisibilityState;
  bool mFocused;
  mozilla::dom::FrameType mFrameType;
};

} 
} 
} 

#endif 
