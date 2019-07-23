





































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"

#include "nsDeviceContextPS.h"
#include "nsGfxPSCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextPS)

static const nsModuleComponentInfo components[] =
{
  { "GFX Postscript Device Context",
    NS_DEVICECONTEXTPS_CID,
    "@mozilla.org/gfx/decidecontext/ps;1",
    nsDeviceContextPSConstructor }  
};

NS_IMPL_NSGETMODULE(nsGfxPSModule, components)

