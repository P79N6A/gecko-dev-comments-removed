




































#include "PowerManager.h"
#include "nsContentUtils.h"
#include "nsDOMClassInfoID.h"
#include "nsIPowerManagerService.h"
#include "nsServiceManagerUtils.h"

DOMCI_DATA(MozPowerManager, mozilla::dom::power::PowerManager)

namespace mozilla {
namespace dom {
namespace power {

NS_INTERFACE_MAP_BEGIN(PowerManager)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozPowerManager)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMozPowerManager)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozPowerManager)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(PowerManager)
NS_IMPL_RELEASE(PowerManager)

NS_IMETHODIMP
PowerManager::Reboot()
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_DOM_SECURITY_ERR);

  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(pmService, NS_OK);

  pmService->Reboot();

  return NS_OK;
}

NS_IMETHODIMP
PowerManager::PowerOff()
{
  NS_ENSURE_TRUE(nsContentUtils::IsCallerChrome(), NS_ERROR_DOM_SECURITY_ERR);

  nsCOMPtr<nsIPowerManagerService> pmService =
    do_GetService(POWERMANAGERSERVICE_CONTRACTID);
  NS_ENSURE_TRUE(pmService, NS_OK);

  pmService->PowerOff();

  return NS_OK;
}

} 
} 
} 
