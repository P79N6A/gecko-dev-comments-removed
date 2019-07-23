



































#include "nsAlertsService.h"
#include "nsToolkitCompsCID.h"
#include "nsIGenericFactory.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"
#include "nsICategoryManager.h"
#include "nsMemory.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAlertsService, Init)

static const nsModuleComponentInfo components[] =
{
  { "Alerts Service",
    NS_ALERTSSERVICE_CID,
    NS_ALERTSERVICE_CONTRACTID,
    nsAlertsServiceConstructor },
};

NS_IMPL_NSGETMODULE(nsAlertsServiceModule, components)
