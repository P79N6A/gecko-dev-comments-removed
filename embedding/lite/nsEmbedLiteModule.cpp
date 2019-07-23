




































#include "nsIGenericFactory.h"

#include "nsEmbedChromeRegistry.h"
#include "nsEmbedGlobalHistory.h"
#include "nsChromeProtocolHandler.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsEmbedGlobalHistory, Init)

static const nsModuleComponentInfo components[] =
{
#if 0 
    { "Chrome Registry", 
      NS_EMBEDCHROMEREGISTRY_CID,
      "@mozilla.org/chrome/chrome-registry;1", 
      nsEmbedChromeRegistryConstructor,
    },
    { "Chrome Protocol Handler",
      NS_CHROMEPROTOCOLHANDLER_CID,
      NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "chrome",
      nsChromeProtocolHandler::Create
    },
#endif
    { "Global History", 
      NS_EMBEDGLOBALHISTORY_CID,
      "@mozilla.org/browser/global-history;1", 
      nsEmbedGlobalHistoryConstructor,
    }
};

NS_IMPL_NSGETMODULE(EmbedLiteComponents, components)
