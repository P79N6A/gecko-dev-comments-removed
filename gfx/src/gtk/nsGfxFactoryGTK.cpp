






































#include "gfx-config.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsBlender.h"
#include "nsRenderingContextGTK.h"
#include "nsDeviceContextGTK.h"
#include "nsScriptableRegion.h"
#include "nsDeviceContextGTK.h"
#include "nsImageGTK.h"
#include "nsFontList.h"
#include "nsRegionGTK.h"
#include "nsGCCache.h"
#ifdef MOZ_ENABLE_PANGO
#include "nsFontMetricsPango.h"
#endif
#ifdef MOZ_ENABLE_XFT
#include "nsFontMetricsXft.h"
#endif
#ifdef MOZ_ENABLE_COREXFONTS
#include "nsFontMetricsGTK.h"
#endif
#include "nsFontMetricsUtils.h"
#include "gfxImageFrame.h"
#ifdef MOZ_ENABLE_FREETYPE2
#include "nsFT2FontCatalog.h"
#include "nsFreeType.h"
#endif



NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextGTK)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextGTK)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageGTK)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionGTK)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)

NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)
#ifdef MOZ_ENABLE_FREETYPE2
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFT2FontCatalog)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsFreeType2, Init)
#endif



static nsresult
nsFontMetricsConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsIFontMetrics *result;

  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  *aResult = nsnull;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

#ifdef MOZ_ENABLE_PANGO
  if (NS_IsPangoEnabled()) {
    result = new nsFontMetricsPango();
    if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
  } else {
#endif
#ifdef MOZ_ENABLE_XFT
  if (NS_IsXftEnabled()) {
    result = new nsFontMetricsXft();
    if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
  } else {
#endif
#ifdef MOZ_ENABLE_COREXFONTS
    result = new nsFontMetricsGTK();
    if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
#endif
#ifdef MOZ_ENABLE_XFT
  }
#endif
#ifdef MOZ_ENABLE_PANGO
  }
#endif

  NS_ADDREF(result);
  nsresult rv = result->QueryInterface(aIID, aResult);
  NS_RELEASE(result);

  return rv;
}

static nsresult
nsFontEnumeratorConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsIFontEnumerator *result;

  if (!aResult)
    return NS_ERROR_NULL_POINTER;

  *aResult = nsnull;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

#ifdef MOZ_ENABLE_PANGO
  if (NS_IsPangoEnabled()) {
    result = new nsFontEnumeratorPango();
    if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
  } else {
#endif
#ifdef MOZ_ENABLE_XFT
  if (NS_IsXftEnabled()) {
    result = new nsFontEnumeratorXft();
    if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
  } else {
#endif
#ifdef MOZ_ENABLE_COREXFONTS
    result = new nsFontEnumeratorGTK();
    if (!result)
      return NS_ERROR_OUT_OF_MEMORY;
#endif
#ifdef MOZ_ENABLE_XFT
  }
#endif
#ifdef MOZ_ENABLE_PANGO
  }
#endif

  NS_ADDREF(result);
  nsresult rv = result->QueryInterface(aIID, aResult);
  NS_RELEASE(result);

  return rv;
}

static NS_IMETHODIMP nsScriptableRegionConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;

  nsIScriptableRegion *inst;

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
  NS_NEWXPCOM(rgn, nsRegionGTK);
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
  { "Gtk Font Metrics",
    NS_FONT_METRICS_CID,
    
    "@mozilla.org/gfx/fontmetrics;1",
    nsFontMetricsConstructor },
  { "Gtk Device Context",
    NS_DEVICE_CONTEXT_CID,
    
    "@mozilla.org/gfx/devicecontext;1",
    nsDeviceContextGTKConstructor },
  { "Gtk Rendering Context",
    NS_RENDERING_CONTEXT_CID,
    
    "@mozilla.org/gfx/renderingcontext;1",
    nsRenderingContextGTKConstructor },
  { "Gtk Image",
    NS_IMAGE_CID,
    
    "@mozilla.org/gfx/image;1",
    nsImageGTKConstructor },
  { "Gtk Region",
    NS_REGION_CID,
    "@mozilla.org/gfx/region/gtk;1",
    nsRegionGTKConstructor },
  { "Scriptable Region",
    NS_SCRIPTABLE_REGION_CID,
    
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },
  { "Blender",
    NS_BLENDER_CID,
    
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },
  { "GTK Font Enumerator",
    NS_FONT_ENUMERATOR_CID,
    
    "@mozilla.org/gfx/fontenumerator;1",
    nsFontEnumeratorConstructor },
  { "Font List",  
    NS_FONTLIST_CID,
    
    NS_FONTLIST_CONTRACTID,
    nsFontListConstructor },
  { "windows image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor, },
#ifdef MOZ_ENABLE_FREETYPE2
  { "TrueType Font Catalog Service",
    NS_FONTCATALOGSERVICE_CID,
    "@mozilla.org/gfx/xfontcatalogservice;1",
    nsFT2FontCatalogConstructor },
  { "FreeType2 routines",
    NS_FREETYPE2_CID,
    NS_FREETYPE2_CONTRACTID,
    nsFreeType2Constructor },
#endif
};

PR_STATIC_CALLBACK(nsresult)
nsGfxGTKModuleCtor(nsIModule *self)
{
  nsImageGTK::Startup();
  return NS_OK;
}

PR_STATIC_CALLBACK(void)
nsGfxGTKModuleDtor(nsIModule *self)
{
  nsRenderingContextGTK::Shutdown();
  nsDeviceContextGTK::Shutdown();
  nsImageGTK::Shutdown();
  nsGCCache::Shutdown();
#ifdef MOZ_WIDGET_GTK
  nsRegionGTK::Shutdown();
#endif
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsGfxGTKModule, components,
                                   nsGfxGTKModuleCtor, nsGfxGTKModuleDtor)
