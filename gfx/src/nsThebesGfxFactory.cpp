





































#include "mozilla/ModuleUtils.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsThebesFontEnumerator.h"
#include "nsScriptableRegion.h"

#include "gfxPlatform.h"







namespace {
class GfxInitialization : public nsISupports {
    NS_DECL_ISUPPORTS
};

NS_IMPL_ISUPPORTS0(GfxInitialization)
}

NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesFontEnumerator)

static nsresult
nsScriptableRegionConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  *aResult = nsnull;
  if (aOuter) {
    return NS_ERROR_NO_AGGREGATION;
  }

  nsCOMPtr<nsIScriptableRegion> scriptableRgn = new nsScriptableRegion();
  return scriptableRgn->QueryInterface(aIID, aResult);
}

NS_GENERIC_FACTORY_CONSTRUCTOR(GfxInitialization)

NS_DEFINE_NAMED_CID(NS_FONT_ENUMERATOR_CID);
NS_DEFINE_NAMED_CID(NS_SCRIPTABLE_REGION_CID);
NS_DEFINE_NAMED_CID(NS_GFX_INITIALIZATION_CID);

static const mozilla::Module::CIDEntry kThebesCIDs[] = {
    { &kNS_FONT_ENUMERATOR_CID, false, NULL, nsThebesFontEnumeratorConstructor },
    { &kNS_SCRIPTABLE_REGION_CID, false, NULL, nsScriptableRegionConstructor },
    { &kNS_GFX_INITIALIZATION_CID, false, NULL, GfxInitializationConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kThebesContracts[] = {
    { "@mozilla.org/gfx/fontenumerator;1", &kNS_FONT_ENUMERATOR_CID },
    { "@mozilla.org/gfx/region;1", &kNS_SCRIPTABLE_REGION_CID },
    { "@mozilla.org/gfx/init;1", &kNS_GFX_INITIALIZATION_CID },
    { NULL }
};

static void
nsThebesGfxModuleDtor()
{
    gfxPlatform::Shutdown();
}

static const mozilla::Module kThebesModule = {
    mozilla::Module::kVersion,
    kThebesCIDs,
    kThebesContracts,
    NULL,
    NULL,
    NULL,
    nsThebesGfxModuleDtor
};

NSMODULE_DEFN(nsGfxModule) = &kThebesModule;
