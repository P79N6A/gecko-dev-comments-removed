




#include <string.h>

#include "nscore.h"

#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"
#include "nsIJARFactory.h"
#include "nsJARProtocolHandler.h"
#include "nsJARURI.h"
#include "nsJAR.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsJAR)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZipReaderCache)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsJARProtocolHandler,
                                         nsJARProtocolHandler::GetSingleton)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJARURI)

NS_DEFINE_NAMED_CID(NS_ZIPREADER_CID);
NS_DEFINE_NAMED_CID(NS_ZIPREADERCACHE_CID);
NS_DEFINE_NAMED_CID(NS_JARPROTOCOLHANDLER_CID);
NS_DEFINE_NAMED_CID(NS_JARURI_CID);

static const mozilla::Module::CIDEntry kJARCIDs[] = {
    { &kNS_ZIPREADER_CID, false, nullptr, nsJARConstructor },
    { &kNS_ZIPREADERCACHE_CID, false, nullptr, nsZipReaderCacheConstructor },
    { &kNS_JARPROTOCOLHANDLER_CID, false, nullptr, nsJARProtocolHandlerConstructor },
    { &kNS_JARURI_CID, false, nullptr, nsJARURIConstructor },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kJARContracts[] = {
    { "@mozilla.org/libjar/zip-reader;1", &kNS_ZIPREADER_CID },
    { "@mozilla.org/libjar/zip-reader-cache;1", &kNS_ZIPREADERCACHE_CID },
    { NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "jar", &kNS_JARPROTOCOLHANDLER_CID },
    { nullptr }
};


static void nsJarShutdown()
{
    
    
    nsJARProtocolHandler *handler = gJarHandler;
    NS_IF_RELEASE(handler);
}

static const mozilla::Module kJARModule = {
    mozilla::Module::kVersion,
    kJARCIDs,
    kJARContracts,
    nullptr,
    nullptr,
    nullptr,
    nsJarShutdown
};

NSMODULE_DEFN(nsJarModule) = &kJARModule;
