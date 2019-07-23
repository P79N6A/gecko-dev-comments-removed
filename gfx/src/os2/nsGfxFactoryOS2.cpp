





































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsCOMPtr.h"
#include "nsGfxCIID.h"

#include "nsBlender.h"
#include "nsFontMetricsOS2.h"
#include "nsRenderingContextOS2.h"
#include "nsScriptableRegion.h"
#include "nsDeviceContextOS2.h"
#include "nsImageOS2.h"
#include "nsRegionOS2.h"
#include "gfxImageFrame.h"
#include "nsFontList.h"
#include "nsIServiceManager.h"
#include "prenv.h"
#include "nsOS2Uni.h"
#include "nsPaletteOS2.h"



#if !defined(USE_FREETYPE)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontMetricsOS2)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDeviceContextOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRenderingContextOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBlender)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegionOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontEnumeratorOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsFontList)
NS_GENERIC_FACTORY_CONSTRUCTOR(gfxImageFrame)

#ifdef USE_FREETYPE

typedef ULONG   HKEY;
#define HKEY_LOCAL_MACHINE      0xFFFFFFEFL
#define HKEY_CURRENT_USER       0xFFFFFFEEL
#define READ_CONTROL            0x00020000
#define KEY_QUERY_VALUE         0x0001
#define KEY_ENUMERATE_SUB_KEYS  0x0008
#define KEY_NOTIFY              0x0010
#define KEY_READ                READ_CONTROL | KEY_QUERY_VALUE | \
                                KEY_ENUMERATE_SUB_KEYS | KEY_NOTIFY
#define ERROR_SUCCESS           0L

static PRBool
UseFTFunctions()
{
  static PRBool init = PR_FALSE;
  if (!init) {
    init = PR_TRUE;

    
    
    
    char* useFT = PR_GetEnv("MOZILLA_USE_EXTENDED_FT2LIB");
    if (useFT == nsnull || *useFT == '\0' || stricmp(useFT, "t") != 0) {
      return PR_FALSE;
    }

    
    APIRET rc;
    HMODULE hmod = NULLHANDLE;
    char LoadError[CCHMAXPATH];
    rc = DosLoadModule(LoadError, CCHMAXPATH, "REGISTRY", &hmod);
    if (rc != NO_ERROR) {
      NS_WARNING("REGISTRY.DLL could not be loaded");
      return PR_FALSE;
    }
    LONG _System (*APIENTRY RegOpenKeyEx)(HKEY, const char*, ULONG, ULONG,
                                           HKEY* );
    LONG _System (*APIENTRY RegQueryValueEx)(HKEY, const char*, ULONG*, ULONG*,
                                              UCHAR*, ULONG*);

    rc = DosQueryProcAddr(hmod, 0L, "RegOpenKeyExA", (PFN*)&RegOpenKeyEx);
    rc += DosQueryProcAddr(hmod, 0L, "RegQueryValueExA", (PFN*)&RegQueryValueEx);
    if (rc != NO_ERROR) {
      NS_WARNING("Registry function(s) were not found in REGISTRY.DLL");
      DosFreeModule(hmod);
      return PR_FALSE;
    }

    
    HKEY key;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER,
                               "Software\\Innotek\\InnoTek Font Engine", 0,
                               KEY_READ, &key);
    if (result != ERROR_SUCCESS) {
      result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                            "Software\\Innotek\\InnoTek Font Engine", 0,
                            KEY_READ, &key);
      if (result != ERROR_SUCCESS) {
        DosFreeModule(hmod);
        return PR_FALSE;
      }
    }

    ULONG value;
    ULONG length = sizeof(value);
    result = RegQueryValueEx(key, "Enabled", NULL, NULL, (UCHAR*)&value,
                             &length);
    if (result != ERROR_SUCCESS || value == 0) {
      
      DosFreeModule(hmod);
      return PR_FALSE;
    }

    
    PPIB ppib;
    PTIB ptib;
    char buffer[CCHMAXPATH], name[_MAX_FNAME], ext[_MAX_EXT], keystr[256];
    DosGetInfoBlocks(&ptib, &ppib);
    DosQueryModuleName(ppib->pib_hmte, CCHMAXPATH, buffer);
    _splitpath(buffer, NULL, NULL, name, ext);
    strcpy(keystr, "Software\\Innotek\\InnoTek Font Engine\\Applications\\");
    strcat(keystr, name);
    strcat(keystr, ext);
    result = RegOpenKeyEx(HKEY_CURRENT_USER, keystr, 0, KEY_READ, &key);
    if (result != ERROR_SUCCESS) {
      result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, keystr, 0, KEY_READ, &key);
      if (result != ERROR_SUCCESS) {
        DosFreeModule(hmod);
        return PR_FALSE;
      }
    }
    result = RegQueryValueEx(key, "Enabled", NULL, NULL, (UCHAR*)&value,
                             &length);
    if (result != ERROR_SUCCESS || value == 0) {
      
      DosFreeModule(hmod);
      return PR_FALSE;
    }

    
    DosFreeModule(hmod);

    
    rc = DosLoadModule(LoadError, 0, "FT2LIB", &hmod);
    if (rc == NO_ERROR) {
      rc = DosQueryProcAddr(hmod, 0, "Ft2EnableFontEngine",
                            (PFN*)&nsFontMetricsOS2FT::pfnFt2EnableFontEngine);
      if (rc == NO_ERROR) {
        DosQueryProcAddr(hmod, 0, "Ft2FontSupportsUnicodeChar1",
                         (PFN*)&nsFontOS2FT::pfnFt2FontSupportsUnicodeChar1);
#ifdef USE_EXPANDED_FREETYPE_FUNCS
        DosQueryProcAddr(hmod, 0, "Ft2QueryTextBoxW",
                         (PFN*)&nsFontOS2FT::pfnFt2QueryTextBoxW);
        DosQueryProcAddr(hmod, 0, "Ft2CharStringPosAtW",
                         (PFN*)&nsFontOS2FT::pfnFt2CharStringPosAtW);
#endif 
        NS_WARNING("*** Now using Freetype functions ***");
        nsFontMetricsOS2::gUseFTFunctions = PR_TRUE;
      }
      DosFreeModule(hmod);
    }
  }
  return nsFontMetricsOS2::gUseFTFunctions;
}

static NS_IMETHODIMP
nsFontMetricsOS2Constructor(nsISupports* aOuter, REFNSIID aIID, void** aResult)
{
  *aResult = nsnull;

  if (aOuter)
    return NS_ERROR_NO_AGGREGATION;

  nsFontMetricsOS2* result;
  if (UseFTFunctions())
    result = new nsFontMetricsOS2FT();
  else
    result = new nsFontMetricsOS2();

  if (! result)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv;
  NS_ADDREF(result);
  rv = result->QueryInterface(aIID, aResult);
  NS_RELEASE(result);
  return rv;
}
#endif 

static NS_IMETHODIMP
nsScriptableRegionConstructor(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;

  nsIScriptableRegion *inst = NULL;

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
  NS_NEWXPCOM(rgn, nsRegionOS2);
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
  { "OS/2 Font Metrics",
    NS_FONT_METRICS_CID,
    
    "@mozilla.org/gfx/fontmetrics;1",
    nsFontMetricsOS2Constructor },
  { "OS/2 Device Context",
    NS_DEVICE_CONTEXT_CID,
    
    "@mozilla.org/gfx/devicecontext;1",
    nsDeviceContextOS2Constructor },
  { "OS/2 Rendering Context",
    NS_RENDERING_CONTEXT_CID,
    
    "@mozilla.org/gfx/renderingcontext;1",
    nsRenderingContextOS2Constructor },
  { "OS/2 Image",
    NS_IMAGE_CID,
    
    "@mozilla.org/gfx/image;1",
    nsImageOS2Constructor },
  { "OS/2 Region",
    NS_REGION_CID,
    "@mozilla.org/gfx/region/gtk;1",
    nsRegionOS2Constructor },
  { "Scriptable Region",
    NS_SCRIPTABLE_REGION_CID,
    
    "@mozilla.org/gfx/region;1",
    nsScriptableRegionConstructor },
  { "Blender",
    NS_BLENDER_CID,
    
    "@mozilla.org/gfx/blender;1",
    nsBlenderConstructor },
  { "OS2 Font Enumerator",
    NS_FONT_ENUMERATOR_CID,
    
    "@mozilla.org/gfx/fontenumerator;1",
    nsFontEnumeratorOS2Constructor },
  { "Font List",  
    NS_FONTLIST_CID,
    
    NS_FONTLIST_CONTRACTID,
    nsFontListConstructor },
  { "windows image frame",
    GFX_IMAGEFRAME_CID,
    "@mozilla.org/gfx/image/frame;2",
    gfxImageFrameConstructor, },
};

PR_STATIC_CALLBACK(void)
nsGfxOS2ModuleDtor(nsIModule *self)
{
  OS2Uni::FreeUconvObjects();
  nsPaletteOS2::FreeGlobalPalette();

}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsGfxOS2Module, components, nsGfxOS2ModuleDtor)

