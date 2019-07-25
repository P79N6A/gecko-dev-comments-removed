




































#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIChromeRegistry.h"
#include "nscore.h"
#include "nsChromeProtocolHandler.h"
#include "nsChromeRegistry.h"
#include "nsChromeRegistryChrome.h"

#ifdef MOZ_IPC
#include "nsXULAppAPI.h"
#include "nsChromeRegistryContent.h"
#endif

static nsChromeRegistry* GetSingleton()
{
    nsChromeRegistry* chromeRegistry = nsChromeRegistry::gChromeRegistry;
    if (chromeRegistry) {
        NS_ADDREF(chromeRegistry);
        return chromeRegistry;
    }
    
#ifdef MOZ_IPC
    if (XRE_GetProcessType() == GeckoProcessType_Content)
        chromeRegistry = new nsChromeRegistryContent;
#endif
    if (!chromeRegistry)
        chromeRegistry = new nsChromeRegistryChrome;

    if (chromeRegistry) {
        NS_ADDREF(chromeRegistry);
        if (NS_FAILED(chromeRegistry->Init()))
            NS_RELEASE(chromeRegistry);
    }
    return chromeRegistry;
}

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsChromeRegistry, GetSingleton);
NS_GENERIC_FACTORY_CONSTRUCTOR(nsChromeProtocolHandler)


static const nsModuleComponentInfo components[] = 
{
    { "Chrome Registry", 
      NS_CHROMEREGISTRY_CID,
      NS_CHROMEREGISTRY_CONTRACTID, 
      nsChromeRegistryConstructor
    },

    { "Chrome Protocol Handler", 
      NS_CHROMEPROTOCOLHANDLER_CID,
      NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "chrome", 
      nsChromeProtocolHandlerConstructor
    }
};

NS_IMPL_NSGETMODULE(nsChromeModule, components)

