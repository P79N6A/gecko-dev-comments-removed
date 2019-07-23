





































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsScriptableRegion.h"
#include "gfxImageFrame.h"

#include "nsThebesDeviceContext.h"
#include "nsThebesRenderingContext.h"
#include "nsThebesImage.h"
#include "nsThebesRegion.h"
#include "nsThebesBlender.h"
#include "nsThebesFontMetrics.h"
#include "nsThebesFontEnumerator.h"
#include "gfxPlatform.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesFontMetrics)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesDeviceContext)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesRenderingContext)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesImage)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesRegion)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsThebesFontEnumerator)

static NS_IMETHODIMP nsScriptableRegionConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
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

  nsCOMPtr <nsIRegion> rgn;
  NS_NEWXPCOM(rgn, nsThebesRegion);
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

static const nsModuleComponentInfo components[] =
{
  { "Thebes nsFontMetrics",
    NS_FONT_METRICS_CID,
    "@mozilla.org/gfx/fontmetrics;1",
    nsThebesFontMetricsConstructor },
  { "Thebes Font Enumerator",
    NS_FONT_ENUMERATOR_CID,
    "@mozilla.org/gfx/fontenumerator;1",
    nsThebesFontEnumeratorConstructor },
  { "Thebes Device Context",
    NS_DEVICE_CONTEXT_CID,
    "@mozilla.org/gfx/devicecontext;1",
    nsThebesDeviceContextConstructor },
  { "Thebes Rendering Context",
    NS_RENDERING_CONTEXT_CID,
    "@mozilla.org/gfx/renderingcontext;1",
    nsThebesRenderingContextConstructor },
  { "Thebes nsImage",
    NS_IMAGE_CID,
    "@mozilla.org/gfx/image;1",
    nsThebesImageConstructor },
  { "Thebes Region",
    NS_REGION_CID,
    "@mozilla.org/gfx/region/nsThebes;1",
    nsThebesRegionConstructor },
  { "Scriptable Region",
    NS_SCRIPTABLE_REGION_CID,
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },
  { "Thebes Blender",
    NS_BLENDER_CID,
    "@mozilla.org/gfx/blender;1",
    nsThebesBlenderConstructor },
  { "image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor },
};

PR_STATIC_CALLBACK(void)
nsThebesGfxModuleDtor(nsIModule *self)
{
    nsThebesDeviceContext::Shutdown();
    gfxPlatform::Shutdown();
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsGfxModule, components, nsThebesGfxModuleDtor)

