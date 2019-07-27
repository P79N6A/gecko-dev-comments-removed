





#ifndef mozilla_dom_ServiceWorkerManagerParent_h
#define mozilla_dom_ServiceWorkerManagerParent_h

#include "mozilla/dom/PServiceWorkerManagerParent.h"

namespace mozilla {

class OriginAttributes;

namespace ipc {
class BackgroundParentImpl;
}

namespace dom {
namespace workers {

class ServiceWorkerManagerService;

class ServiceWorkerManagerParent final : public PServiceWorkerManagerParent
{
  friend class mozilla::ipc::BackgroundParentImpl;

public:
  uint64_t ID() const
  {
    return mID;
  }

private:
  ServiceWorkerManagerParent();
  ~ServiceWorkerManagerParent();

  virtual bool RecvRegister(
                           const ServiceWorkerRegistrationData& aData) override;

  virtual bool RecvUnregister(const PrincipalInfo& aPrincipalInfo,
                              const nsString& aScope) override;

  virtual bool RecvPropagateSoftUpdate(const OriginAttributes& aOriginAttributes,
                                       const nsString& aScope) override;

  virtual bool RecvPropagateUnregister(const PrincipalInfo& aPrincipalInfo,
                                       const nsString& aScope) override;

  virtual bool RecvPropagateRemove(const nsCString& aHost) override;

  virtual bool RecvPropagateRemoveAll() override;

  virtual bool RecvShutdown() override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  nsRefPtr<ServiceWorkerManagerService> mService;

  
  
  uint64_t mID;
};

} 
} 
} 

#endif 
