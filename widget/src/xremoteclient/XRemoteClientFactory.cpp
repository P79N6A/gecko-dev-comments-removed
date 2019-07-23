



































#define NS_XREMOTECLIENT_CID                         \
{ 0xcfae5900,                                        \
  0x1dd1,                                            \
  0x11b2,                                            \
  { 0x95, 0xd0, 0xad, 0x45, 0x4c, 0x23, 0x3d, 0xc6 } \
}



#ifdef MOZ_WIDGET_PHOTON
#include "PhRemoteClient.h"
#else
#include "XRemoteClient.h"
#endif

#include "nsXRemoteClientCID.h"
#include "nsIGenericFactory.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(XRemoteClient)

static const nsModuleComponentInfo components[] =
{
  { "XRemote Client",
    NS_XREMOTECLIENT_CID,
    NS_XREMOTECLIENT_CONTRACTID,
    XRemoteClientConstructor }
};

NS_IMPL_NSGETMODULE(XRemoteClientModule, components)


