




































#include <stdio.h>
#include "nscore.h"
#include "nsIFactory.h"
#include "nsIGenericFactory.h"     
#include "nsISupports.h"
#include "nsGfxCIID.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsDeviceContextXP.h"
#include "nsGfxXPrintCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextXp)
 
static const nsModuleComponentInfo components[] =
{
  { "GFX Xprint Device Context",
    NS_DEVICECONTEXTXP_CID,
    "@mozilla.org/gfx/decidecontext/xprint;1",
    nsDeviceContextXpConstructor }  
};
 
NS_IMPL_NSGETMODULE(nsGfxXprintModule, components)

