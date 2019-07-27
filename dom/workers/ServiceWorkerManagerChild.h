





#ifndef mozilla_dom_ServiceWorkerManagerChild_h
#define mozilla_dom_ServiceWorkerManagerChild_h

#include "mozilla/dom/PServiceWorkerManagerChild.h"
#include "mozilla/ipc/BackgroundUtils.h"

namespace mozilla {

class OriginAttributes;

namespace ipc {
class BackgroundChildImpl;
}

namespace dom {
namespace workers {

class ServiceWorkerManagerChild final : public PServiceWorkerManagerChild
{
  friend class mozilla::ipc::BackgroundChildImpl;

public:
  NS_INLINE_DECL_REFCOUNTING(ServiceWorkerManagerChild)

  void ManagerShuttingDown()
  {
    mShuttingDown = true;
  }

  virtual bool RecvNotifyRegister(const ServiceWorkerRegistrationData& aData)
                                                                       override;

  virtual bool RecvNotifySoftUpdate(const OriginAttributes& aOriginAttributes,
                                    const nsString& aScope) override;

  virtual bool RecvNotifyUnregister(const PrincipalInfo& aPrincipalInfo,
                                    const nsString& aScope) override;

  virtual bool RecvNotifyRemove(const nsCString& aHost) override;

  virtual bool RecvNotifyRemoveAll() override;

private:
  ServiceWorkerManagerChild()
    : mShuttingDown(false)
  {}

  ~ServiceWorkerManagerChild() {}

  bool mShuttingDown;
};

} 
} 
} 

#endif 
