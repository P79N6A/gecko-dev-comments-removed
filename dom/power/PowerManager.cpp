




































#include "nsDOMClassInfoID.h"

#include "PowerManager.h"

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
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
PowerManager::PowerOff()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
} 
