




































#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIChromeRegistry.h"
#include "nscore.h"
#include "nsChromeProtocolHandler.h"
#include "nsChromeRegistry.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsChromeRegistry, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsChromeProtocolHandler)

NS_DEFINE_NAMED_CID(NS_CHROMEREGISTRY_CID);
NS_DEFINE_NAMED_CID(NS_CHROMEPROTOCOLHANDLER_CID);

static const mozilla::Module::CIDEntry kChromeCIDs[] = {
    { &kNS_CHROMEREGISTRY_CID, false, NULL, nsChromeRegistryConstructor },
    { &kNS_CHROMEPROTOCOLHANDLER_CID, false, NULL, nsChromeProtocolHandlerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kChromeContracts[] = {
    { NS_CHROMEREGISTRY_CONTRACTID, &kNS_CHROMEREGISTRY_CID },
    { NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "chrome", &kNS_CHROMEPROTOCOLHANDLER_CID },
    { NULL }
};

static const mozilla::Module kChromeModule = {
    mozilla::Module::kVersion,
    kChromeCIDs,
    kChromeContracts
};

NSMODULE_DEFN(nsChromeModule) = &kChromeModule;

