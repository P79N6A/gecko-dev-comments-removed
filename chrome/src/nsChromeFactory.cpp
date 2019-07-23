




































#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIChromeRegistry.h"
#include "nscore.h"
#include "nsChromeProtocolHandler.h"
#include "nsChromeRegistry.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsChromeRegistry, Init)
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

