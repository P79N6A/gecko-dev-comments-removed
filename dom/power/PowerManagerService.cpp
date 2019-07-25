




































#include "mozilla/Hal.h"
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
  hal::Reboot();
  return NS_OK;
}

NS_IMETHODIMP
PowerManagerService::PowerOff()
{
  hal::PowerOff();
  return NS_OK;
}

} 
} 
} 
