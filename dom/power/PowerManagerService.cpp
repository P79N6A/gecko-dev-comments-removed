




































#include "PowerManagerService.h"

namespace mozilla {
namespace dom {
namespace power {

NS_IMPL_ISUPPORTS1(PowerManagerService, nsIPowerManagerService)

 already_AddRefed<nsIPowerManagerService>
PowerManagerService::GetInstance()
{
  nsCOMPtr<nsIPowerManagerService> pmService;

  pmService = new PowerManagerService();

  return pmService.forget();
}

NS_IMETHODIMP
PowerManagerService::Reboot()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
PowerManagerService::PowerOff()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
} 
