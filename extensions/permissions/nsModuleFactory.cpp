



































#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsContentBlocker.h"
#include "nsXPIDLString.h"
#include "nsICategoryManager.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsContentBlocker, Init)

static NS_METHOD
RegisterContentPolicy(nsIComponentManager *aCompMgr, nsIFile *aPath,
                      const char *registryLocation, const char *componentType,
                      const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
      do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;
  nsXPIDLCString previous;
  return catman->AddCategoryEntry("content-policy",
                                  NS_CONTENTBLOCKER_CONTRACTID,
                                  NS_CONTENTBLOCKER_CONTRACTID,
                                  PR_TRUE, PR_TRUE, getter_Copies(previous));
}

static NS_METHOD
UnregisterContentPolicy(nsIComponentManager *aCompMgr, nsIFile *aPath,
                        const char *registryLocation,
                        const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catman =
      do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;

  return catman->DeleteCategoryEntry("content-policy",
                                     NS_CONTENTBLOCKER_CONTRACTID,
                                     PR_TRUE);
}


static const nsModuleComponentInfo components[] = {
  { "ContentBlocker",
    NS_CONTENTBLOCKER_CID,
    NS_CONTENTBLOCKER_CONTRACTID,
    nsContentBlockerConstructor,
    RegisterContentPolicy, UnregisterContentPolicy
  }
};

NS_IMPL_NSGETMODULE(nsPermissionsModule, components)
