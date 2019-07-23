





































#include "nsIGenericFactory.h"
#include "nsAutoConfig.h"
#include "nsReadConfig.h"
#include "nsIAppStartupNotifier.h"
#include "nsICategoryManager.h"
#if defined(MOZ_LDAP_XPCOM)
#include "nsLDAPSyncQuery.h"
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsAutoConfig, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsReadConfig, Init)
#if defined(MOZ_LDAP_XPCOM)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLDAPSyncQuery)
#endif




static NS_METHOD 
RegisterReadConfig(nsIComponentManager *aCompMgr,
                   nsIFile *aPath,
                   const char *registryLocation,
                   const char *componentType,
                   const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> 
    categoryManager(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) {
    rv = categoryManager->AddCategoryEntry("pref-config-startup", 
                                           "ReadConfig Module",
                                           NS_READCONFIG_CONTRACTID,
                                           PR_TRUE, PR_TRUE, nsnull);
  }
  return rv;
}

static NS_METHOD 
UnRegisterReadConfig(nsIComponentManager *aCompMgr,
                     nsIFile *aPath,
                     const char *registryLocation,
                     const nsModuleComponentInfo *info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> 
    categoryManager(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv)) {
    rv = categoryManager->DeleteCategoryEntry(APPSTARTUP_CATEGORY,
                                              "ReadConfig Module", PR_TRUE);
  }
  return rv;
}



static const nsModuleComponentInfo components[] = 
{
  { 
    NS_AUTOCONFIG_CLASSNAME, 
    NS_AUTOCONFIG_CID, 
    NS_AUTOCONFIG_CONTRACTID, 
    nsAutoConfigConstructor
  },
  { 
    NS_READCONFIG_CLASSNAME,
    NS_READCONFIG_CID,
    NS_READCONFIG_CONTRACTID,
    nsReadConfigConstructor,
    RegisterReadConfig,
    UnRegisterReadConfig
  },
#if defined(MOZ_LDAP_XPCOM)
  { 
    "LDAPSyncQuery module", 
    NS_LDAPSYNCQUERY_CID, 
    "@mozilla.org/ldapsyncquery;1", 
    nsLDAPSyncQueryConstructor
  },
#endif
};

NS_IMPL_NSGETMODULE(nsAutoConfigModule, components)
