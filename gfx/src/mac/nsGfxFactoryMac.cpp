





































#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsIFontList.h"
#include "nsGfxCIID.h"
#include "nsFontList.h"
#include "nsFontMetricsMac.h"
#include "nsRenderingContextMac.h"
#include "nsImageMac.h"
#include "nsDeviceContextMac.h"
#include "nsRegionMac.h"
#include "nsScriptableRegion.h"
#include "nsBlender.h"
#include "nsCOMPtr.h"
#include "nsUnicodeMappingUtil.h"
#include "gfxImageFrame.h"

#include "nsIGenericFactory.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontMetricsMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDrawingSurfaceMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorMac)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)
NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)

static NS_IMETHODIMP
nsScriptableRegionConstructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  NS_ENSURE_NO_AGGREGATION(aOuter);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  nsCOMPtr<nsIRegion> region = new nsRegionMac();
  NS_ENSURE_TRUE(region, NS_ERROR_OUT_OF_MEMORY);
  nsCOMPtr<nsIScriptableRegion> result(new nsScriptableRegion(region));
  NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);
  return result->QueryInterface(aIID, aResult);
}

static const nsModuleComponentInfo components[] =
{
  { "nsFontMetrics",
    NS_FONT_METRICS_CID,
    "@mozilla.org/gfx/fontmetrics;1",
    nsFontMetricsMacConstructor },
  { "nsDeviceContext",
    NS_DEVICE_CONTEXT_CID,
    "@mozilla.org/gfx/devicecontext;1",
    nsDeviceContextMacConstructor },
  { "nsRenderingContext",
    NS_RENDERING_CONTEXT_CID,
    "@mozilla.org/gfx/renderingcontext;1",
    nsRenderingContextMacConstructor },
  { "nsImage",
    NS_IMAGE_CID,
    "@mozilla.org/gfx/image;1",
    nsImageMacConstructor },
  { "nsRegion",
    NS_REGION_CID,
    "@mozilla.org/gfx/unscriptable-region;1",
    nsRegionMacConstructor },
  { "nsScriptableRegion",
    NS_SCRIPTABLE_REGION_CID,
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },
  { "nsBlender",
    NS_BLENDER_CID,
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },
  { "nsDrawingSurface",
    NS_DRAWING_SURFACE_CID,
    "@mozilla.org/gfx/drawing-surface;1",
    nsDrawingSurfaceMacConstructor },
  { "nsFontEnumerator",
    NS_FONT_ENUMERATOR_CID,
    "@mozilla.org/gfx/fontenumerator;1",
    nsFontEnumeratorMacConstructor },
  { "nsFontList",
    NS_FONTLIST_CID,
    "@mozilla.org/gfx/fontlist;1",
    nsFontListConstructor },
  { "windows image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor, },
};

PR_STATIC_CALLBACK(void)
nsGfxMacModuleDtor(nsIModule *self)
{
  nsUnicodeMappingUtil::FreeSingleton();
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsGfxMacModule, components, nsGfxMacModuleDtor)


