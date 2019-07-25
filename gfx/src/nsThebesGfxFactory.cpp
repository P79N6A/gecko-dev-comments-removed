





































#include "mozilla/ModuleUtils.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsThebesFontEnumerator.h"
#include "nsThebesRegion.h"
#include "nsScriptableRegion.h"

#include "nsDeviceContext.h"
#include "gfxPlatform.h"








namespace {
class GfxInitialization : public nsISupports {
    NS_DECL_ISUPPORTS
};

NS_IMPL_ISUPPORTS0(GfxInitialization)
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesFontEnumerator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesRegion)

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

NS_GENERIC_FACTORY_CONSTRUCTOR(GfxInitialization)

NS_DEFINE_NAMED_CID(NS_FONT_ENUMERATOR_CID);
NS_DEFINE_NAMED_CID(NS_REGION_CID);
NS_DEFINE_NAMED_CID(NS_SCRIPTABLE_REGION_CID);
NS_DEFINE_NAMED_CID(NS_GFX_INITIALIZATION_CID);

static const mozilla::Module::CIDEntry kThebesCIDs[] = {
    { &kNS_FONT_ENUMERATOR_CID, false, NULL, nsThebesFontEnumeratorConstructor },
    { &kNS_REGION_CID, false, NULL, nsThebesRegionConstructor },
    { &kNS_SCRIPTABLE_REGION_CID, false, NULL, nsScriptableRegionConstructor },
    { &kNS_GFX_INITIALIZATION_CID, false, NULL, GfxInitializationConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kThebesContracts[] = {
    { "@mozilla.org/gfx/fontenumerator;1", &kNS_FONT_ENUMERATOR_CID },
    { "@mozilla.org/gfx/region/nsThebes;1", &kNS_REGION_CID },
    { "@mozilla.org/gfx/region;1", &kNS_SCRIPTABLE_REGION_CID },
    { "@mozilla.org/gfx/init;1", &kNS_GFX_INITIALIZATION_CID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kThebesCategories[] = {
    { "app-startup", "Gfx Initialization", "service,@mozilla.org/gfx/init;1" },
    { NULL }
};

static nsresult
nsThebesGfxModuleCtor()
{
    gfxPlatform::Init();
    return NS_OK;
}

static void
nsThebesGfxModuleDtor()
{
    nsDeviceContext::ClearCachedSystemFonts();
    gfxPlatform::Shutdown();
}

static const mozilla::Module kThebesModule = {
    mozilla::Module::kVersion,
    kThebesCIDs,
    kThebesContracts,
    kThebesCategories,
    NULL,
    nsThebesGfxModuleCtor,
    nsThebesGfxModuleDtor
};

NSMODULE_DEFN(nsGfxModule) = &kThebesModule;
