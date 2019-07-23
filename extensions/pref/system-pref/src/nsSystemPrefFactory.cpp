








































#include "nsICategoryManager.h"
#include "nsIGenericFactory.h"
#include "nsSystemPref.h"
#include "nsSystemPrefService.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSystemPref, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsSystemPrefService, Init)




static NS_METHOD
RegisterSystemPref(nsIComponentManager *aCompMgr,
                   nsIFile *aPath,
                   const char *registryLocation,
                   const char *componentType,
                   const nsModuleComponentInfo *info)
{
    nsresult rv;

    nsCOMPtr<nsICategoryManager>
        categoryManager(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv)) {
        rv = categoryManager->AddCategoryEntry(APPSTARTUP_CATEGORY,
                                               "SystemPref Module",
                                               NS_SYSTEMPREF_CONTRACTID,
                                               PR_TRUE, PR_TRUE, nsnull);
    }

    return rv;
}

static NS_METHOD 
UnRegisterSystemPref(nsIComponentManager *aCompMgr,
                     nsIFile *aPath,
                     const char *registryLocation,
                     const nsModuleComponentInfo *info)
{
    nsresult rv;
    nsCOMPtr<nsICategoryManager>
        categoryManager(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv)) {
        rv = categoryManager->DeleteCategoryEntry(APPSTARTUP_CATEGORY,
                                                  "SystemPref Module",
                                                  PR_TRUE);
    }
    return rv;
}

static const nsModuleComponentInfo components[] = {
    { NS_SYSTEMPREF_CLASSNAME,
      NS_SYSTEMPREF_CID,
      NS_SYSTEMPREF_CONTRACTID,
      nsSystemPrefConstructor,
      RegisterSystemPref,
      UnRegisterSystemPref,
    },
    { NS_SYSTEMPREF_SERVICE_CLASSNAME,
      NS_SYSTEMPREF_SERVICE_CID,
      NS_SYSTEMPREF_SERVICE_CONTRACTID,
      nsSystemPrefServiceConstructor,
    },
};

NS_IMPL_NSGETMODULE(nsSystemPrefModule, components)
