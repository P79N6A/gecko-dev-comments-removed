



































#include "nsAlertsService.h"
#include "nsToolkitCompsCID.h"
#include "nsIGenericFactory.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsServiceManagerUtils.h"
#include "nsICategoryManager.h"
#include "nsMemory.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAlertsService, Init)

static
NS_METHOD
nsAlertsServiceRegister(nsIComponentManager* aCompMgr,
                        nsIFile* aPath,
                        const char* registryLocation,
                        const char* componentType,
                        const nsModuleComponentInfo* info)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  char* prev = nsnull;
  rv = catman->AddCategoryEntry("app-startup", "nsAlertsService",
                                NS_ALERTSERVICE_CONTRACTID, PR_TRUE, PR_TRUE,
                                &prev);
  if (prev)
    nsMemory::Free(prev);

  return rv;
}

static
NS_METHOD
nsAlertsServiceUnregister(nsIComponentManager* aCompMgr,
                          nsIFile* aPath,
                          const char* registryLocation,
                          const nsModuleComponentInfo* info)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->DeleteCategoryEntry("app-startup", "nsAlertsService", PR_TRUE);

  return rv;
}

static const nsModuleComponentInfo components[] =
{
  { "Alerts Service",
    NS_ALERTSSERVICE_CID,
    NS_ALERTSERVICE_CONTRACTID,
    nsAlertsServiceConstructor,
    nsAlertsServiceRegister,
    nsAlertsServiceUnregister },
};

NS_IMPL_NSGETMODULE(nsAlertsServiceModule, components)
