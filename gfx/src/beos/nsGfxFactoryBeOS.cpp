




































#include "nsIGenericFactory.h" 
#include "nsIModule.h" 
#include "nsCOMPtr.h" 
#include "nsGfxCIID.h" 
 
#include "nsBlender.h" 
#include "nsFontMetricsBeOS.h"
#include "nsRenderingContextBeOS.h"
#include "nsScriptableRegion.h" 
#include "nsDeviceContextBeOS.h" 
#include "nsImageBeOS.h" 
#include "nsFontList.h"
#include "gfxImageFrame.h"



NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontMetricsBeOS) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextBeOS) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextBeOS) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageBeOS) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionBeOS) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorBeOS) 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)

NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)
 

 
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
  NS_NEWXPCOM(rgn, nsRegionBeOS); 
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
  { "BeOS Font Metrics", 
    NS_FONT_METRICS_CID, 
    
    "@mozilla.org/gfx/fontmetrics;1", 
    nsFontMetricsBeOSConstructor }, 
  { "BeOS Device Context", 
    NS_DEVICE_CONTEXT_CID, 
    
    "@mozilla.org/gfx/devicecontext;1", 
    nsDeviceContextBeOSConstructor }, 
  { "BeOS Rendering Context", 
    NS_RENDERING_CONTEXT_CID, 
    
    "@mozilla.org/gfx/renderingcontext;1", 
    nsRenderingContextBeOSConstructor }, 
  { "BeOS Image", 
    NS_IMAGE_CID, 
    
    "@mozilla.org/gfx/image;1", 
    nsImageBeOSConstructor }, 
  { "BeOS Region", 
    NS_REGION_CID, 
    "@mozilla.org/gfx/region/beos;1", 
    nsRegionBeOSConstructor }, 
  { "Scriptable Region", 
    NS_SCRIPTABLE_REGION_CID, 
    
    "@mozilla.org/gfx/region;1", 
    nsScriptableRegionConstructor }, 
  { "Blender", 
    NS_BLENDER_CID, 
    
    "@mozilla.org/gfx/blender;1", 
    nsBlenderConstructor }, 
  { "BeOS Font Enumerator", 
    NS_FONT_ENUMERATOR_CID, 
    
    "@mozilla.org/gfx/fontenumerator;1", 
    nsFontEnumeratorBeOSConstructor }, 
  { "Font List",  
    NS_FONTLIST_CID,
    
    NS_FONTLIST_CONTRACTID,
    nsFontListConstructor },
  { "windows image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor, },
};   

NS_IMPL_NSGETMODULE(nsGfxBeOSModule, components) 
