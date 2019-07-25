





































#include "mozilla/ModuleUtils.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsScriptableRegion.h"

#include "nsThebesDeviceContext.h"
#include "nsThebesRegion.h"
#include "nsThebesFontMetrics.h"
#include "nsThebesFontEnumerator.h"
#include "gfxPlatform.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesFontMetrics)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesDeviceContext)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesRegion)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesFontEnumerator)

static nsresult
nsScriptableRegionConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;

  nsIScriptableRegion *inst = nsnull;

  if ( !aResult )
  {
    rv = NS_ERROR_NULL_POINTER;
    return rv;
  }
  *aResult = nsnull;
  if (aOuter)
  {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }

  nsCOMPtr <nsIRegion> rgn = new nsThebesRegion();
  nsCOMPtr<nsIScriptableRegion> scriptableRgn;
  if (rgn != nsnull)
  {
    scriptableRgn = new nsScriptableRegion(rgn);
    inst = scriptableRgn;
  }
  if (!inst)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
    return rv;
  }
  NS_ADDREF(inst);
  
  
  scriptableRgn = nsnull;
  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

  return rv;
}

NS_DEFINE_NAMED_CID(NS_FONT_METRICS_CID);
NS_DEFINE_NAMED_CID(NS_FONT_ENUMERATOR_CID);
NS_DEFINE_NAMED_CID(NS_DEVICE_CONTEXT_CID);
NS_DEFINE_NAMED_CID(NS_REGION_CID);
NS_DEFINE_NAMED_CID(NS_SCRIPTABLE_REGION_CID);

static const mozilla::Module::CIDEntry kThebesCIDs[] = {
    { &kNS_FONT_METRICS_CID, false, NULL, nsThebesFontMetricsConstructor },
    { &kNS_FONT_ENUMERATOR_CID, false, NULL, nsThebesFontEnumeratorConstructor },
    { &kNS_DEVICE_CONTEXT_CID, false, NULL, nsThebesDeviceContextConstructor },
    { &kNS_REGION_CID, false, NULL, nsThebesRegionConstructor },
    { &kNS_SCRIPTABLE_REGION_CID, false, NULL, nsScriptableRegionConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kThebesContracts[] = {
    { "@mozilla.org/gfx/fontmetrics;1", &kNS_FONT_METRICS_CID },
    { "@mozilla.org/gfx/fontenumerator;1", &kNS_FONT_ENUMERATOR_CID },
    { "@mozilla.org/gfx/devicecontext;1", &kNS_DEVICE_CONTEXT_CID },
    { "@mozilla.org/gfx/region/nsThebes;1", &kNS_REGION_CID },
    { "@mozilla.org/gfx/region;1", &kNS_SCRIPTABLE_REGION_CID },
    { NULL }
};

static nsresult
nsThebesGfxModuleCtor()
{
    return gfxPlatform::Init();
}

static void
nsThebesGfxModuleDtor()
{
    nsThebesDeviceContext::Shutdown();
    gfxPlatform::Shutdown();
}

static const mozilla::Module kThebesModule = {
    mozilla::Module::kVersion,
    kThebesCIDs,
    kThebesContracts,
    NULL,
    NULL,
    nsThebesGfxModuleCtor,
    nsThebesGfxModuleDtor
};

NSMODULE_DEFN(nsGfxModule) = &kThebesModule;
