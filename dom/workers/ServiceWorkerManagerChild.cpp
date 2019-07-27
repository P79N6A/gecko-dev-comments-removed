





#include "ServiceWorkerManagerChild.h"
#include "ServiceWorkerManager.h"
#include "mozilla/unused.h"

namespace mozilla {

using namespace ipc;

namespace dom {
namespace workers {

bool
ServiceWorkerManagerChild::RecvNotifyRegister(
                                     const ServiceWorkerRegistrationData& aData)
{
  if (mShuttingDown) {
    return true;
  }

  nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  MOZ_ASSERT(swm);

  swm->LoadRegistration(aData);
  return true;
}

bool
ServiceWorkerManagerChild::RecvNotifySoftUpdate(
                                      const OriginAttributes& aOriginAttributes,
                                      const nsString& aScope)
{
  if (mShuttingDown) {
    return true;
  }

  nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  MOZ_ASSERT(swm);

  swm->SoftUpdate(aOriginAttributes, NS_ConvertUTF16toUTF8(aScope));
  return true;
}

bool
ServiceWorkerManagerChild::RecvNotifyUnregister(const PrincipalInfo& aPrincipalInfo,
                                                const nsString& aScope)
{
  if (mShuttingDown) {
    return true;
  }

  nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
  MOZ_ASSERT(swm);

  nsCOMPtr<nsIPrincipal> principal = PrincipalInfoToPrincipal(aPrincipalInfo);
  if (NS_WARN_IF(!principal)) {
    return true;
  }

  nsresult rv = swm->Unregister(principal, nullptr, aScope);
  unused << NS_WARN_IF(NS_FAILED(rv));
  return true;
}

} 
} 
} 
