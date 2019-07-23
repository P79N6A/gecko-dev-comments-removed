





































#include "nsIGenericFactory.h"
#include "nsAccessProxy.h"
#include "nsIServiceManager.h"
#include "nsIRegistry.h"
#include "prprf.h"
#include "nsCRT.h"
#include "nsICategoryManager.h"











static NS_METHOD nsAccessProxyRegistrationProc(nsIComponentManager *aCompMgr,
  nsIFile *aPath, const char *registryLocation, const char *componentType,
  const nsModuleComponentInfo *info)
{
  
  
  

  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) 
    rv = categoryManager->AddCategoryEntry(APPSTARTUP_CATEGORY, "Access Proxy", 
      "service," NS_ACCESSPROXY_CONTRACTID, PR_TRUE, PR_TRUE, nsnull);
  return rv;
}


NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsAccessProxy,nsAccessProxy::GetInstance)

static void PR_CALLBACK AccessProxyModuleDtor(nsIModule* self)
{
    nsAccessProxy::ReleaseInstance();
}

static const nsModuleComponentInfo components[] =
{
  { "AccessProxy Component", NS_ACCESSPROXY_CID, NS_ACCESSPROXY_CONTRACTID,
    nsAccessProxyConstructor, nsAccessProxyRegistrationProc,
    nsnull  
  }
};

NS_IMPL_NSGETMODULE_WITH_DTOR(nsAccessProxy, components, AccessProxyModuleDtor)




