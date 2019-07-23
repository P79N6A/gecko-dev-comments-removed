






































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsBlender.h"
#include "nsFontMetricsQt.h"
#include "nsRenderingContextQt.h"
#include "nsDeviceContextSpecQt.h"
#include "nsScreenManagerQt.h"
#include "nsScriptableRegion.h"
#include "nsDeviceContextQt.h"
#include "nsImageQt.h"
#include "nsFontList.h"
#include "nsPrintSession.h"
#include "gfxImageFrame.h"

#include "qtlog.h"


PRLogModuleInfo *gQtLogModule = PR_NewLogModule("QtGfx");



NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontMetricsQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextSpecQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorQt)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsScreenManagerQt)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrintSession, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)


static nsresult nsScriptableRegionConstructor(nsISupports *aOuter,REFNSIID aIID,void **aResult)
{
    nsresult rv;

    nsIScriptableRegion *inst = 0;

    if (NULL == aResult) {
        rv = NS_ERROR_NULL_POINTER;
        return rv;
    }
    *aResult = NULL;
    if (NULL != aOuter) {
        rv = NS_ERROR_NO_AGGREGATION;
        return rv;
    }
    
    nsCOMPtr <nsIRegion> rgn;
    NS_NEWXPCOM(rgn, nsRegionQt);
    if (rgn != nsnull) {
        nsCOMPtr<nsIScriptableRegion> scriptableRgn = new nsScriptableRegion(rgn);
        inst = scriptableRgn;
    }
    if (NULL == inst) {
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
    { "Qt Font Metrics",
      NS_FONT_METRICS_CID,
      "@mozilla.org/gfx/fontmetrics;1",
      nsFontMetricsQtConstructor },
    { "Qt Device Context",
      NS_DEVICE_CONTEXT_CID,
      "@mozilla.org/gfx/devicecontext;1",
      nsDeviceContextQtConstructor },
    { "Qt Rendering Context",
      NS_RENDERING_CONTEXT_CID,
      "@mozilla.org/gfx/renderingcontext;1",
      nsRenderingContextQtConstructor },
    { "Qt Image",
      NS_IMAGE_CID,
      "@mozilla.org/gfx/image;1",
      nsImageQtConstructor },
    { "Qt Region",
      NS_REGION_CID,
      "@mozilla.org/gfx/region/qt;1",
      nsRegionQtConstructor },
    { "Scriptable Region",
      NS_SCRIPTABLE_REGION_CID,
      "@mozilla.org/gfx/region;1",
      nsScriptableRegionConstructor },
    { "Blender",
      NS_BLENDER_CID,
      "@mozilla.org/gfx/blender;1",
      nsBlenderConstructor },
    { "Qt Device Context Spec",
      NS_DEVICE_CONTEXT_SPEC_CID,
      "@mozilla.org/gfx/devicecontextspec;1",
      nsDeviceContextSpecQtConstructor },
    { "Qt Font Enumerator",
      NS_FONT_ENUMERATOR_CID,
      "@mozilla.org/gfx/fontenumerator;1",
      nsFontEnumeratorQtConstructor },
    { "Font List",
      NS_FONTLIST_CID,
      
      NS_FONTLIST_CONTRACTID,
      nsFontListConstructor },
    { "Qt Screen Manager",
      NS_SCREENMANAGER_CID,
      "@mozilla.org/gfx/screenmanager;1",
      nsScreenManagerQtConstructor },
    { "shared image frame",
      GFX_IMAGEFRAME_CID,
      "@mozilla.org/gfx/image/frame;2",
      gfxImageFrameConstructor, },
    { "Print Session",
      NS_PRINTSESSION_CID,
      "@mozilla.org/gfx/printsession;1",
      nsPrintSessionConstructor }
};

NS_IMPL_NSGETMODULE(nsGfxQtModule, components)

