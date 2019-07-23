





































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsBlender.h"
#include "nsFontMetricsPh.h"
#include "nsRenderingContextPh.h"
#include "nsDeviceContextSpecPh.h"
#include "nsScreenManagerPh.h"
#include "nsScriptableRegion.h"
#include "nsDeviceContextPh.h"
#include "nsPrintOptionsPh.h"
#include "nsPrintSession.h"
#include "nsFontList.h"
#include "gfxImageFrame.h"



NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontMetricsPh)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextPh)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextPh)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImagePh)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionPh)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecPh)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorPh)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerPh)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPrinterEnumeratorPh)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintOptionsPh, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)



static nsresult nsScriptableRegionConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;

  nsIScriptableRegion *inst;

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
  NS_NEWXPCOM(rgn, nsRegionPh);
  nsCOMPtr<nsIScriptableRegion> scriptableRgn;
  if (rgn != nsnull)
  {
    scriptableRgn = new nsScriptableRegion(rgn);
    inst = scriptableRgn;
  }
  if (NULL == inst)
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
  { "Ph Font Metrics",
    NS_FONT_METRICS_CID,
    "@mozilla.org/gfx/fontmetrics;1",
    nsFontMetricsPhConstructor },

  { "Ph Device Context",
    NS_DEVICE_CONTEXT_CID,
    "@mozilla.org/gfx/devicecontext;1",
    nsDeviceContextPhConstructor },

  { "Ph Rendering Context",
    NS_RENDERING_CONTEXT_CID,
    "@mozilla.org/gfx/renderingcontext;1",
    nsRenderingContextPhConstructor },

  { "Ph Image",
    NS_IMAGE_CID,
    "@mozilla.org/gfx/image;1",
    nsImagePhConstructor },

  { "Ph Region",
    NS_REGION_CID,
    "@mozilla.org/gfx/region/Ph;1",
    nsRegionPhConstructor },

  { "Scriptable Region",
    NS_SCRIPTABLE_REGION_CID,
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },

  { "Blender",
    NS_BLENDER_CID,
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },

  { "Ph Device Context Spec",
    NS_DEVICE_CONTEXT_SPEC_CID,
    "@mozilla.org/gfx/devicecontextspec;1",
    nsDeviceContextSpecPhConstructor },

  { "PrintSettings Service",
		NS_PRINTSETTINGSSERVICE_CID,
		"@mozilla.org/gfx/printsettings-service;1",
    nsPrintOptionsPhConstructor },

   { "Ph Font Enumerator",
    NS_FONT_ENUMERATOR_CID,
    "@mozilla.org/gfx/fontenumerator;1",
    nsFontEnumeratorPhConstructor },

	{ "Font List",
		NS_FONTLIST_CID,
		NS_FONTLIST_CONTRACTID,
		nsFontListConstructor },

  { "Ph Screen Manager",
    NS_SCREENMANAGER_CID,
    "@mozilla.org/gfx/screenmanager;1",
    nsScreenManagerPhConstructor },

	{ "Ph Printer Enumerator",
	  NS_PRINTER_ENUMERATOR_CID,
	"@mozilla.org/gfx/printerenumerator;1",
	nsPrinterEnumeratorPhConstructor },

  { "Print Session",
    NS_PRINTSESSION_CID,
    "@mozilla.org/gfx/printsession;1",
    nsPrintSessionConstructor },

  { "windows image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor, },
};

NS_IMPL_NSGETMODULE(nsGfxPhModule, components)

