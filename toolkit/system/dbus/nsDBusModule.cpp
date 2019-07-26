






#include "nsServiceManagerUtils.h"
#include "nsICategoryManager.h"
#include "mozilla/ModuleUtils.h"
#include "nsIAppStartupNotifier.h"
#include "nsNetworkManagerListener.h"
#include "nsNetCID.h"

#define NS_DBUS_NETWORK_LINK_SERVICE_CID    \
  { 0x75a500a2,                                        \
    0x0030,                                            \
    0x40f7,                                            \
    { 0x86, 0xf8, 0x63, 0xf2, 0x25, 0xb9, 0x40, 0xae } \
  }
  


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsNetworkManagerListener, Init)
NS_DEFINE_NAMED_CID(NS_DBUS_NETWORK_LINK_SERVICE_CID);

static const mozilla::Module::CIDEntry kDBUSCIDs[] = {
    { &kNS_DBUS_NETWORK_LINK_SERVICE_CID, false, NULL, nsNetworkManagerListenerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kDBUSContracts[] = {
    { NS_NETWORK_LINK_SERVICE_CONTRACTID, &kNS_DBUS_NETWORK_LINK_SERVICE_CID },
    { NULL }
};

static const mozilla::Module kDBUSModule = {
    mozilla::Module::kVersion,
    kDBUSCIDs,
    kDBUSContracts
};

NSMODULE_DEFN(nsDBusModule) = &kDBUSModule;
