







































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsBlender.h"
#include "nsFontMetricsXlib.h"
#include "nsRenderingContextXlib.h"

#include "nsDeviceContextSpecXlib.h"
#include "nsScriptableRegion.h"
#include "nsDeviceContextXlib.h"
#include "nsImageXlib.h"
#include "nsFontList.h"
#include "nsPrintOptionsXlib.h"
#include "nsPrintSession.h"
#include "gfxImageFrame.h"



NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontMetricsXlib)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextXlib)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextXlib)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageXlib)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionXlib)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecXlib)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorXlib)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintOptionsXlib, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrinterEnumeratorXlib)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)



static nsresult nsScriptableRegionConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;

  nsIScriptableRegion *inst = nsnull;

  if ( NULL == aResult )
  {
    rv = NS_ERROR_NULL_POINTER;
    return rv;
  }
  *aResult = NULL;
  if (NULL != aOuter)
  {
    rv = NS_ERROR_NO_AGGREGATION;
    return rv;
  }
  
  nsCOMPtr <nsIRegion> rgn;
  NS_NEWXPCOM(rgn, nsRegionXlib);
  if (rgn != nsnull)
  {
    nsCOMPtr<nsIScriptableRegion> scriptableRgn = new nsScriptableRegion(rgn);
    inst = scriptableRgn;
  }
  if (NULL == inst)
  {
    rv = NS_ERROR_OUT_OF_MEMORY;
    return rv;
  }
  NS_ADDREF(inst);
  rv = inst->QueryInterface(aIID, aResult);
  NS_RELEASE(inst);

  return rv;
}

static const nsModuleComponentInfo components[] =
{
  { "Xlib Font Metrics",
    NS_FONT_METRICS_CID,
    
    "@mozilla.org/gfx/fontmetrics;1",
    nsFontMetricsXlibConstructor },
  { "Xlib Device Context",
    NS_DEVICE_CONTEXT_CID,
    
    "@mozilla.org/gfx/devicecontext;1",
    nsDeviceContextXlibConstructor },
  { "Xlib Rendering Context",
    NS_RENDERING_CONTEXT_CID,
    
    "@mozilla.org/gfx/renderingcontext;1",
    nsRenderingContextXlibConstructor },
  { "Xlib Image",
    NS_IMAGE_CID,
    
    "@mozilla.org/gfx/image;1",
    nsImageXlibConstructor },
  { "Xlib Region",
    NS_REGION_CID,
    "@mozilla.org/gfx/region/xlib;1",
    nsRegionXlibConstructor },
  { "Scriptable Region",
    NS_SCRIPTABLE_REGION_CID,
    
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },
  { "Blender",
    NS_BLENDER_CID,
    
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },
  { "Xlib Device Context Spec",
    NS_DEVICE_CONTEXT_SPEC_CID,
    
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecXlibConstructor },
  { "PrintSettings Service",
     NS_PRINTSETTINGSSERVICE_CID,
     
     "@mozilla.org/gfx/printsettings-service;1",
     nsPrintOptionsXlibConstructor },      
  { "Xlib Font Enumerator",
    NS_FONT_ENUMERATOR_CID,
    
    "@mozilla.org/gfx/fontenumerator;1",
      nsFontEnumeratorXlibConstructor },
  { "Font List",  
    NS_FONTLIST_CID,
    
    NS_FONTLIST_CONTRACTID,
    nsFontListConstructor },
  { "Xlib Printer Enumerator",
    NS_PRINTER_ENUMERATOR_CID,
    
    "@mozilla.org/gfx/printerenumerator;1",
    nsPrinterEnumeratorXlibConstructor },
  { "windows image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor, },
  { "Print Session",
    NS_PRINTSESSION_CID,
    "@mozilla.org/gfx/printsession;1",
    nsPrintSessionConstructor }
};

PR_STATIC_CALLBACK(void)
nsGfxXlibModuleDtor(nsIModule *self)
{
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsGfxXlibModule, components, nsGfxXlibModuleDtor)
