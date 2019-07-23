






































#include <string.h>

#include "nscore.h"
#include "pratom.h"
#include "prmem.h"
#include "prio.h"
#include "plstr.h"
#include "prlog.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"
#include "nsJAR.h"
#include "nsIJARFactory.h"
#include "nsRecyclingAllocator.h"
#include "nsXPTZipLoader.h"
#include "nsJARProtocolHandler.h"
#include "nsJARURI.h"

extern nsRecyclingAllocator *gZlibAllocator;

NS_GENERIC_FACTORY_CONSTRUCTOR(nsXPTZipLoader)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJAR)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZipReaderCache)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsJARProtocolHandler,
                                         nsJARProtocolHandler::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJARURI)


static const nsModuleComponentInfo components[] = 
{
    { "XPT Zip Reader",
      NS_XPTZIPREADER_CID,
      NS_XPTLOADER_CONTRACTID_PREFIX "zip",
      nsXPTZipLoaderConstructor
    },
    { "Zip Reader", 
       NS_ZIPREADER_CID,
      "@mozilla.org/libjar/zip-reader;1", 
      nsJARConstructor
    },
    { "Zip Reader Cache", 
       NS_ZIPREADERCACHE_CID,
      "@mozilla.org/libjar/zip-reader-cache;1", 
      nsZipReaderCacheConstructor
    },
    { NS_JARPROTOCOLHANDLER_CLASSNAME,
      NS_JARPROTOCOLHANDLER_CID,
      NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "jar", 
      nsJARProtocolHandlerConstructor
    },
    { NS_JARURI_CLASSNAME, 
      NS_JARURI_CID,
      nsnull,
      nsJARURIConstructor
    }
};


static void PR_CALLBACK nsJarShutdown(nsIModule *module)
{
    
    delete gZlibAllocator;
    NS_IF_RELEASE(gJarHandler);
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsJarModule, components, nsJarShutdown)
