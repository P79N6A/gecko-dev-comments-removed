




































#include "nscore.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsGfxCIID.h"
#include "nsFontMetricsWin.h"
#include "nsRenderingContextWin.h"
#include "nsImageWin.h"
#include "nsDeviceContextWin.h"
#include "nsRegionWin.h"
#include "nsBlender.h"
#include "nsScriptableRegion.h"
#include "nsFontList.h"
#include "nsIGenericFactory.h"
#include "gfxImageFrame.h"



NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDrawingSurfaceWin)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)
NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)

PRBool
UseAFunctions()
{
#ifndef WINCE
  static PRBool useAFunctions = PR_FALSE;
  static PRBool init = PR_FALSE;
  if (!init) {
    init = 1;
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    ::GetVersionEx(&os);
    if ((os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
        (os.dwMajorVersion == 4) &&
        (os.dwMinorVersion == 0) &&    
        (::GetACP() == 932)) {         
      useAFunctions = 1;
    }
  }

  return useAFunctions;
#else
  return PR_FALSE;
#endif
}

static NS_IMETHODIMP
nsFontMetricsWinConstructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsFontMetricsWin* result;
  if (UseAFunctions())
    result = new nsFontMetricsWinA();
  else
    result = new nsFontMetricsWin();

  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  NS_ADDREF(result);
  rv = result->QueryInterface(aIID, aResult);
  NS_RELEASE(result);
  return rv;
}

static NS_IMETHODIMP
nsScriptableRegionConstructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsRegionWin* region = new nsRegionWin();
  if (! region)
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(region);

  nsresult rv = NS_ERROR_OUT_OF_MEMORY;
  nsScriptableRegion* result = new nsScriptableRegion(region);
  if (result) {
    NS_ADDREF(result);
    rv = result->QueryInterface(aIID, aResult);
    NS_RELEASE(result);
  }

  NS_RELEASE(region);
  return rv;
}

static const nsModuleComponentInfo components[] =
{
  { "nsFontMetricsWin",
    NS_FONT_METRICS_CID,
    "@mozilla.org/gfx/fontmetrics;1",
    nsFontMetricsWinConstructor },

  { "nsDeviceContextWin",
    NS_DEVICE_CONTEXT_CID,
    "@mozilla.org/gfx/devicecontext;1",
    nsDeviceContextWinConstructor },

  { "nsRenderingContextWin",
    NS_RENDERING_CONTEXT_CID,
    "@mozilla.org/gfx/renderingcontext;1",
    nsRenderingContextWinConstructor },

  { "nsImageWin",
    NS_IMAGE_CID,
    "@mozilla.org/gfx/image;1",
    nsImageWinConstructor },

  { "nsRegionWin",
    NS_REGION_CID,
    "@mozilla.org/gfx/unscriptable-region;1",
    nsRegionWinConstructor },

  { "nsBlender",
    NS_BLENDER_CID,
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },

  { "nsDrawingSurfaceWin",
    NS_DRAWING_SURFACE_CID,
    "@mozilla.org/gfx/drawing-surface;1",
    nsDrawingSurfaceWinConstructor },

  { "nsScriptableRegion",
    NS_SCRIPTABLE_REGION_CID,
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },

  { "nsFontEnumeratorWin",
    NS_FONT_ENUMERATOR_CID,
    "@mozilla.org/gfx/fontenumerator;1",
    nsFontEnumeratorWinConstructor },

  { "nsFontList",
    NS_FONTLIST_CID,
    "@mozilla.org/gfx/fontlist;1",
    nsFontListConstructor },

  { "windows image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor, },
};

NS_IMPL_NSGETMODULE(nsGfxModule, components)
