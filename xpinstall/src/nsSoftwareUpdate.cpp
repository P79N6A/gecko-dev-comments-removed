





































#include "nscore.h"
#include "nsIGenericFactory.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIServiceManager.h"
#include "nsICategoryManager.h"
#include "nsIScriptNameSpaceManager.h"
#include "nsCURILoader.h"

#include "nsSoftwareUpdateIIDs.h"

#include "nsInstallTrigger.h"
#include "nsXPInstallManager.h"






NS_GENERIC_FACTORY_CONSTRUCTOR(nsInstallTrigger)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPInstallManager)



static NS_METHOD
RegisterInstallTrigger( nsIComponentManager *aCompMgr,
                        nsIFile *aPath,
                        const char *registryLocation,
                        const char *componentType,
                        const nsModuleComponentInfo *info)
{
  nsresult rv = NS_OK;

  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsXPIDLCString previous;
  rv = catman->AddCategoryEntry(JAVASCRIPT_GLOBAL_PROPERTY_CATEGORY,
                                "InstallTrigger",
                                NS_INSTALLTRIGGERCOMPONENT_CONTRACTID,
                                true, true, getter_Copies(previous));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}



static const nsModuleComponentInfo components[] =
{
    { "InstallTrigger Component",
       NS_SoftwareUpdateInstallTrigger_CID,
       NS_INSTALLTRIGGERCOMPONENT_CONTRACTID,
       nsInstallTriggerConstructor,
       RegisterInstallTrigger
    },

    { "XPInstall Content Handler",
      NS_SoftwareUpdateInstallTrigger_CID,
      NS_CONTENT_HANDLER_CONTRACTID_PREFIX"application/x-xpinstall",
      nsInstallTriggerConstructor
    },

    { "XPInstallManager Component",
      NS_XPInstallManager_CID,
      NS_XPINSTALLMANAGERCOMPONENT_CONTRACTID,
      nsXPInstallManagerConstructor
    }
};

NS_IMPL_NSGETMODULE(nsSoftwareUpdate, components)
