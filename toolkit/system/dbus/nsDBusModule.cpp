







































#include "nsServiceManagerUtils.h"
#include "nsICategoryManager.h"
#include "nsIGenericFactory.h"
#include "nsIAppStartupNotifier.h"
#include "nsNetworkManagerListener.h"
#include "nsNetCID.h"

#define NS_DBUS_NETWORK_LINK_SERVICE_CLASSNAME "DBus Network Link Status"
#define NS_DBUS_NETWORK_LINK_SERVICE_CID    \
  { 0x75a500a2,                                        \
    0x0030,                                            \
    0x40f7,                                            \
    { 0x86, 0xf8, 0x63, 0xf2, 0x25, 0xb9, 0x40, 0xae } \
  }
  


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsNetworkManagerListener, Init)

static const nsModuleComponentInfo components[] = {
    { NS_DBUS_NETWORK_LINK_SERVICE_CLASSNAME,
      NS_DBUS_NETWORK_LINK_SERVICE_CID,
      NS_NETWORK_LINK_SERVICE_CONTRACTID,
      nsNetworkManagerListenerConstructor,
      nsnull,
      nsnull,
    },
};

NS_IMPL_NSGETMODULE(nsDBusModule, components)
