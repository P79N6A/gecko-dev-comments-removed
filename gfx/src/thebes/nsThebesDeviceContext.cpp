






































#include "nsFont.h"
#include "nsGfxCIID.h"
#include "nsIFontMetrics.h"
#include "nsHashtable.h"
#include "nsILanguageAtomService.h"
#include "nsUnicharUtils.h"

#include "nsIServiceManager.h"
#include "nsIPrefService.h"
#include "nsCRT.h"

#include "nsThebesDeviceContext.h"
#include "nsThebesRenderingContext.h"
#include "gfxUserFontSet.h"

#include "nsIWidget.h"
#include "nsIView.h"
#include "nsILookAndFeel.h"

#ifdef MOZ_ENABLE_GTK2

#include <cstdlib>

#include <cmath>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "nsFont.h"

#include <pango/pango.h>
#ifdef MOZ_X11
#include <gdk/gdkx.h>
#include <pango/pangox.h>
#endif 
#include <pango/pango-fontmap.h>
#endif 

#include "gfxImageSurface.h"

#ifdef MOZ_ENABLE_GTK2
#include "nsSystemFontsGTK2.h"
#include "gfxPDFSurface.h"
#include "gfxPSSurface.h"
static nsSystemFontsGTK2 *gSystemFonts = nsnull;
#elif XP_WIN
#include "nsSystemFontsWin.h"
#include "gfxWindowsSurface.h"
#include "gfxPDFSurface.h"
static nsSystemFontsWin *gSystemFonts = nsnull;
#ifndef WINCE
#include <usp10.h>
#endif
#elif defined(XP_OS2)
#include "nsSystemFontsOS2.h"
#include "gfxPDFSurface.h"
static nsSystemFontsOS2 *gSystemFonts = nsnull;
#elif defined(XP_BEOS)
#include "nsSystemFontsBeOS.h"
static nsSystemFontsBeOS *gSystemFonts = nsnull;
#elif XP_MACOSX
#include "nsSystemFontsMac.h"
#include "gfxQuartzSurface.h"
#include "gfxImageSurface.h"
static nsSystemFontsMac *gSystemFonts = nsnull;
#elif defined(MOZ_WIDGET_QT)
#include "nsSystemFontsQt.h"
static nsSystemFontsQt *gSystemFonts = nsnull;
#else
#error Need to declare gSystemFonts!
#endif

#if defined(MOZ_ENABLE_GTK2) && defined(MOZ_X11)
extern "C" {
static int x11_error_handler (Display *dpy, XErrorEvent *err) {
    NS_ASSERTION(PR_FALSE, "X Error");
    return 0;
}
}
#endif

#ifdef PR_LOGGING
PRLogModuleInfo* gThebesGFXLog = nsnull;
#endif

class nsFontCache
{
public:
    nsFontCache();
    ~nsFontCache();

    nsresult Init(nsIDeviceContext* aContext);
    nsresult GetMetricsFor(const nsFont& aFont, nsIAtom* aLangGroup,
                           gfxUserFontSet* aUserFontSet,
                           nsIFontMetrics*& aMetrics);

    nsresult FontMetricsDeleted(const nsIFontMetrics* aFontMetrics);
    nsresult Compact();
    nsresult Flush();
    nsresult CreateFontMetricsInstance(nsIFontMetrics** fm);

protected:
    nsTArray<nsIFontMetrics*> mFontMetrics;
    nsIDeviceContext         *mContext; 
                                        
};

nsFontCache::nsFontCache()
{
    MOZ_COUNT_CTOR(nsFontCache);
    mContext = nsnull;
}

nsFontCache::~nsFontCache()
{
    MOZ_COUNT_DTOR(nsFontCache);
    Flush();
}

nsresult
nsFontCache::Init(nsIDeviceContext* aContext)
{
    NS_PRECONDITION(nsnull != aContext, "null ptr");
    
    
    mContext = aContext;
    return NS_OK;
}

nsresult
nsFontCache::GetMetricsFor(const nsFont& aFont, nsIAtom* aLangGroup,
  gfxUserFontSet* aUserFontSet, nsIFontMetrics*& aMetrics)
{
    
    

    nsIFontMetrics* fm;
    PRInt32 n = mFontMetrics.Length() - 1;
    for (PRInt32 i = n; i >= 0; --i) {
        fm = mFontMetrics[i];
        nsIThebesFontMetrics* tfm = static_cast<nsIThebesFontMetrics*>(fm);
        if (fm->Font().Equals(aFont) && tfm->GetUserFontSet() == aUserFontSet) {
            nsCOMPtr<nsIAtom> langGroup;
            fm->GetLangGroup(getter_AddRefs(langGroup));
            if (aLangGroup == langGroup.get()) {
                if (i != n) {
                    
                    mFontMetrics.RemoveElementAt(i);
                    mFontMetrics.AppendElement(fm);
                }
                tfm->GetThebesFontGroup()->UpdateFontList();
                NS_ADDREF(aMetrics = fm);
                return NS_OK;
            }
        }
    }

    

    aMetrics = nsnull;
    nsresult rv = CreateFontMetricsInstance(&fm);
    if (NS_FAILED(rv)) return rv;
    rv = fm->Init(aFont, aLangGroup, mContext, aUserFontSet);
    if (NS_SUCCEEDED(rv)) {
        
        
        mFontMetrics.AppendElement(fm);
        aMetrics = fm;
        NS_ADDREF(aMetrics);
        return NS_OK;
    }
    fm->Destroy();
    NS_RELEASE(fm);

    
    
    

    Compact();
    rv = CreateFontMetricsInstance(&fm);
    if (NS_FAILED(rv)) return rv;
    rv = fm->Init(aFont, aLangGroup, mContext, aUserFontSet);
    if (NS_SUCCEEDED(rv)) {
        mFontMetrics.AppendElement(fm);
        aMetrics = fm;
        NS_ADDREF(aMetrics);
        return NS_OK;
    }
    fm->Destroy();
    NS_RELEASE(fm);

    
    

    n = mFontMetrics.Length() - 1; 
    if (n >= 0) {
        aMetrics = mFontMetrics[n];
        NS_ADDREF(aMetrics);
        return NS_OK;
    }

    NS_POSTCONDITION(NS_SUCCEEDED(rv), "font metrics should not be null - bug 136248");
    return rv;
}

nsresult
nsFontCache::CreateFontMetricsInstance(nsIFontMetrics** fm)
{
    static NS_DEFINE_CID(kFontMetricsCID, NS_FONT_METRICS_CID);
    return CallCreateInstance(kFontMetricsCID, fm);
}

nsresult nsFontCache::FontMetricsDeleted(const nsIFontMetrics* aFontMetrics)
{
    mFontMetrics.RemoveElement(aFontMetrics);
    return NS_OK;
}

nsresult nsFontCache::Compact()
{
    
    
    for (PRInt32 i = mFontMetrics.Length()-1; i >= 0; --i) {
        nsIFontMetrics* fm = mFontMetrics[i];
        nsIFontMetrics* oldfm = fm;
        
        
        NS_RELEASE(fm); 
        
        
        if (mFontMetrics.IndexOf(oldfm) != mFontMetrics.NoIndex) { 
            
            NS_ADDREF(oldfm);
        }
    }
    return NS_OK;
}

nsresult nsFontCache::Flush()
{
    for (PRInt32 i = mFontMetrics.Length()-1; i >= 0; --i) {
        nsIFontMetrics* fm = mFontMetrics[i];
        
        
        
        fm->Destroy();
        NS_RELEASE(fm);
    }

    mFontMetrics.Clear();

    return NS_OK;
}

NS_IMPL_ISUPPORTS3(nsThebesDeviceContext, nsIDeviceContext, nsIObserver, nsISupportsWeakReference)

nsThebesDeviceContext::nsThebesDeviceContext()
{
#ifdef PR_LOGGING
    if (!gThebesGFXLog)
        gThebesGFXLog = PR_NewLogModule("thebesGfx");
#endif

    PR_LOG(gThebesGFXLog, PR_LOG_DEBUG, ("#### Creating DeviceContext %p\n", this));

    mAppUnitsPerDevPixel = nscoord(-1);
    mAppUnitsPerInch = nscoord(-1);
    mAppUnitsPerDevNotScaledPixel = nscoord(-1);
    mPixelScale = 1.0f;

    mFontCache = nsnull;
    mWidget = nsnull;
    mFontAliasTable = nsnull;

#ifdef NS_DEBUG
    mInitialized = PR_FALSE;
#endif

    mDepth = 0;
    mWidth = 0;
    mHeight = 0;
    mPrintingScale = 1.0f;

#if defined(XP_WIN) && !defined(WINCE)
    SCRIPT_DIGITSUBSTITUTE sds;
    ScriptRecordDigitSubstitution(LOCALE_USER_DEFAULT, &sds);
#endif
}

static PRBool DeleteValue(nsHashKey* aKey, void* aValue, void* closure)
{
    delete ((nsString*)aValue);
    return PR_TRUE;
}

nsThebesDeviceContext::~nsThebesDeviceContext()
{
    nsCOMPtr<nsIObserverService> obs(do_GetService("@mozilla.org/observer-service;1"));
    if (obs)
        obs->RemoveObserver(this, "memory-pressure");

    if (nsnull != mFontCache) {
        delete mFontCache;
        mFontCache = nsnull;
    }

    if (nsnull != mFontAliasTable) {
        mFontAliasTable->Enumerate(DeleteValue);
        delete mFontAliasTable;
    }
}

NS_IMETHODIMP
nsThebesDeviceContext::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aSomeData)
{
    if (mFontCache && !nsCRT::strcmp(aTopic, "memory-pressure")) {
        mFontCache->Compact();
    }
    return NS_OK;
}

NS_IMETHODIMP nsThebesDeviceContext::CreateFontCache()
{
    mFontCache = new nsFontCache();
    if (!mFontCache) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    return mFontCache->Init(this);
}

NS_IMETHODIMP nsThebesDeviceContext::FontMetricsDeleted(const nsIFontMetrics* aFontMetrics)
{
    if (mFontCache) {
        mFontCache->FontMetricsDeleted(aFontMetrics);
    }
    return NS_OK;
}

void
nsThebesDeviceContext::GetLocaleLangGroup(void)
{
    if (!mLocaleLangGroup) {
        nsCOMPtr<nsILanguageAtomService> langService;
        langService = do_GetService(NS_LANGUAGEATOMSERVICE_CONTRACTID);
        if (langService) {
            mLocaleLangGroup = langService->GetLocaleLanguageGroup();
        }
        if (!mLocaleLangGroup) {
            mLocaleLangGroup = do_GetAtom("x-western");
        }
    }
}

NS_IMETHODIMP nsThebesDeviceContext::GetMetricsFor(const nsFont& aFont,
  nsIAtom* aLangGroup, gfxUserFontSet* aUserFontSet, nsIFontMetrics*& aMetrics)
{
    if (nsnull == mFontCache) {
        nsresult rv = CreateFontCache();
        if (NS_FAILED(rv)) {
            aMetrics = nsnull;
            return rv;
        }
        
        GetLocaleLangGroup();
    }

    
    if (!aLangGroup) {
        aLangGroup = mLocaleLangGroup;
    }

    return mFontCache->GetMetricsFor(aFont, aLangGroup, aUserFontSet, aMetrics);
}

NS_IMETHODIMP nsThebesDeviceContext::GetMetricsFor(const nsFont& aFont,
                                                   gfxUserFontSet* aUserFontSet,
                                                   nsIFontMetrics*& aMetrics)
{
    if (nsnull == mFontCache) {
        nsresult rv = CreateFontCache();
        if (NS_FAILED(rv)) {
            aMetrics = nsnull;
            return rv;
        }
        
        GetLocaleLangGroup();
    }
    return mFontCache->GetMetricsFor(aFont, mLocaleLangGroup, aUserFontSet,
                                     aMetrics);
}

struct FontEnumData {
    FontEnumData(nsIDeviceContext* aDC, nsString& aFaceName)
        : mDC(aDC), mFaceName(aFaceName)
    {}
    nsIDeviceContext* mDC;
    nsString&         mFaceName;
};

static PRBool FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
    FontEnumData* data = (FontEnumData*)aData;
    
    
    if (aGeneric) {
        data->mFaceName = aFamily;
        return PR_FALSE; 
    }
    else {
        nsAutoString local;
        PRBool       aliased;
        data->mDC->GetLocalFontName(aFamily, local, aliased);
        if (aliased || (NS_SUCCEEDED(data->mDC->CheckFontExistence(local)))) {
            data->mFaceName = local;
            return PR_FALSE; 
        }
    }
    return PR_TRUE; 
}

NS_IMETHODIMP nsThebesDeviceContext::FirstExistingFont(const nsFont& aFont, nsString& aFaceName)
{
    FontEnumData data(this, aFaceName);
    if (aFont.EnumerateFamilies(FontEnumCallback, &data)) {
        return NS_ERROR_FAILURE; 
    }
    return NS_OK;
}

class FontAliasKey: public nsHashKey
{
public:
    FontAliasKey(const nsString& aString)
    { mString.Assign(aString); }

    virtual PRUint32 HashCode(void) const;
    virtual PRBool Equals(const nsHashKey *aKey) const;
    virtual nsHashKey *Clone(void) const;

    nsString mString;
};

PRUint32 FontAliasKey::HashCode(void) const
{
    PRUint32 hash = 0;
    const PRUnichar* string = mString.get();
    PRUnichar ch;
    while ((ch = *string++) != 0) {
        
        ch = ToUpperCase(ch);
        hash = ((hash << 5) + (hash << 2) + hash) + ch;
    }
    return hash;
}

PRBool FontAliasKey::Equals(const nsHashKey *aKey) const
{
    return mString.Equals(((FontAliasKey*)aKey)->mString, nsCaseInsensitiveStringComparator());
}

nsHashKey* FontAliasKey::Clone(void) const
{
    return new FontAliasKey(mString);
}

nsresult nsThebesDeviceContext::CreateFontAliasTable()
{
    nsresult result = NS_OK;

    if (nsnull == mFontAliasTable) {
        mFontAliasTable = new nsHashtable();
        if (nsnull != mFontAliasTable) {

            nsAutoString times;         times.AssignLiteral("Times");
            nsAutoString timesNewRoman; timesNewRoman.AssignLiteral("Times New Roman");
            nsAutoString timesRoman;    timesRoman.AssignLiteral("Times Roman");
            nsAutoString arial;         arial.AssignLiteral("Arial");
            nsAutoString helvetica;     helvetica.AssignLiteral("Helvetica");
            nsAutoString courier;       courier.AssignLiteral("Courier");
            nsAutoString courierNew;    courierNew.AssignLiteral("Courier New");
            nsAutoString nullStr;

            AliasFont(times, timesNewRoman, timesRoman, PR_FALSE);
            AliasFont(timesRoman, timesNewRoman, times, PR_FALSE);
            AliasFont(timesNewRoman, timesRoman, times, PR_FALSE);
            AliasFont(arial, helvetica, nullStr, PR_FALSE);
            AliasFont(helvetica, arial, nullStr, PR_FALSE);
            AliasFont(courier, courierNew, nullStr, PR_TRUE);
            AliasFont(courierNew, courier, nullStr, PR_FALSE);
        }
        else {
            result = NS_ERROR_OUT_OF_MEMORY;
        }
    }
    return result;
}

nsresult nsThebesDeviceContext::AliasFont(const nsString& aFont,
                                          const nsString& aAlias,
                                          const nsString& aAltAlias,
                                          PRBool aForceAlias)
{
    nsresult result = NS_OK;

    if (nsnull != mFontAliasTable) {
        if (aForceAlias || NS_FAILED(CheckFontExistence(aFont))) {
            if (NS_SUCCEEDED(CheckFontExistence(aAlias))) {
                nsString* entry = new nsString(aAlias);
                if (nsnull != entry) {
                    FontAliasKey key(aFont);
                    mFontAliasTable->Put(&key, entry);
                }
                else {
                    result = NS_ERROR_OUT_OF_MEMORY;
                }
            }
            else if (!aAltAlias.IsEmpty() && NS_SUCCEEDED(CheckFontExistence(aAltAlias))) {
                nsString* entry = new nsString(aAltAlias);
                if (nsnull != entry) {
                    FontAliasKey key(aFont);
                    mFontAliasTable->Put(&key, entry);
                }
                else {
                    result = NS_ERROR_OUT_OF_MEMORY;
                }
            }
        }
    }
    else {
        result = NS_ERROR_FAILURE;
    }
    return result;
}

NS_IMETHODIMP nsThebesDeviceContext::GetLocalFontName(const nsString& aFaceName,
                                                      nsString& aLocalName,
                                                      PRBool& aAliased)
{
    nsresult result = NS_OK;

    if (nsnull == mFontAliasTable) {
        result = CreateFontAliasTable();
    }

    if (nsnull != mFontAliasTable) {
        FontAliasKey key(aFaceName);
        const nsString* alias = (const nsString*)mFontAliasTable->Get(&key);
        if (nsnull != alias) {
            aLocalName = *alias;
            aAliased = PR_TRUE;
        }
        else {
            aLocalName = aFaceName;
            aAliased = PR_FALSE;
        }
    }
    return result;
}

NS_IMETHODIMP nsThebesDeviceContext::FlushFontCache(void)
{
    if (nsnull != mFontCache)
        mFontCache->Flush();
    return NS_OK;
}

 void
nsThebesDeviceContext::Shutdown()
{
    delete gSystemFonts;
    gSystemFonts = nsnull;
}

PRBool
nsThebesDeviceContext::IsPrinterSurface()
{
  return(mPrintingSurface != NULL);
}

nsresult
nsThebesDeviceContext::SetDPI()
{
    PRInt32 dpi = -1;
    PRBool dotsArePixels = PR_TRUE;
    
    
    
    float prefDevPixelsPerCSSPixel = -1.0;

    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
        const char *prefName = "layout.css.devPixelsPerPx";
        PRInt32 prefType = nsIPrefBranch::PREF_INVALID;
        PRInt32 prefInt;
        nsXPIDLCString prefString;
        nsresult rv;

        prefs->GetPrefType(prefName, &prefType);

        switch (prefType) {
        case nsIPrefBranch::PREF_INT:
            
            
            rv = prefs->GetIntPref(prefName, &prefInt);
            if (NS_SUCCEEDED(rv)) {
                prefDevPixelsPerCSSPixel = static_cast<float>(prefInt);
            }
            break;

        case nsIPrefBranch::PREF_STRING:
            rv = prefs->GetCharPref(prefName, getter_Copies(prefString));
            if (NS_SUCCEEDED(rv) && !prefString.IsEmpty()) {
                prefDevPixelsPerCSSPixel = static_cast<float>(atof(prefString));
            }
            break;
        }
    }

    
    if (mPrintingSurface &&
        (mPrintingSurface->GetType() == gfxASurface::SurfaceTypePDF ||
         mPrintingSurface->GetType() == gfxASurface::SurfaceTypePS ||
         mPrintingSurface->GetType() == gfxASurface::SurfaceTypeQuartz)) {
        dpi = 72;
        dotsArePixels = PR_FALSE;
    } else {
        nsresult rv;
        
        
        
        
        PRInt32 prefDPI = -1;
        if (prefs) {
            rv = prefs->GetIntPref("layout.css.dpi", &prefDPI);
            if (NS_FAILED(rv)) {
                prefDPI = -1;
            }
        }

#if defined(MOZ_ENABLE_GTK2)
        GdkScreen *screen = gdk_screen_get_default();
        gtk_settings_get_for_screen(screen); 
        PRInt32 OSVal = PRInt32(round(gdk_screen_get_resolution(screen)));

        if (prefDPI == 0) 
            dpi = OSVal;
        else  
            dpi = PR_MAX(OSVal, 96);

#elif defined(XP_WIN)
        
        HDC dc = GetPrintHDC();
        if (dc) {
            PRInt32 OSVal = GetDeviceCaps(dc, LOGPIXELSY);

            dpi = 144;
            mPrintingScale = float(OSVal)/dpi;
            dotsArePixels = PR_FALSE;
        } else {
            dc = GetDC((HWND)nsnull);

            PRInt32 OSVal = GetDeviceCaps(dc, LOGPIXELSY);

            ReleaseDC((HWND)nsnull, dc);

            if (OSVal != 0)
                dpi = OSVal;
        }

#elif defined(XP_OS2)
        
        HDC dc = GetPrintHDC();
        PRBool doCloseDC = PR_FALSE;
        if (dc <= 0) { 
            
            dc = DevOpenDC((HAB)1, OD_MEMORY,"*",0L, NULL, NULLHANDLE);
            doCloseDC = PR_TRUE;
        }
        if (dc > 0) {
            
            LONG lDPI;
            if (DevQueryCaps(dc, CAPS_VERTICAL_FONT_RES, 1, &lDPI))
                dpi = lDPI;
            if (doCloseDC)
                DevCloseDC(dc);
        }
        if (dpi < 0) 
            dpi = 96;
#elif defined(XP_MACOSX)

        
        dpi = 96;

#elif defined(MOZ_WIDGET_QT)
        
        dpi = 96;
#else
#error undefined platform dpi
#endif

        if (prefDPI > 0 && !mPrintingSurface)
            dpi = prefDPI;
    }

    NS_ASSERTION(dpi != -1, "no dpi set");

    if (dotsArePixels) {
        if (prefDevPixelsPerCSSPixel <= 0) {
            
            
            
            
            PRUint32 roundedDPIScaleFactor = (dpi + 48)/96;
#ifdef MOZ_WIDGET_GTK2
            
            
            roundedDPIScaleFactor = dpi/96;
#endif
            mAppUnitsPerDevNotScaledPixel =
                PR_MAX(1, AppUnitsPerCSSPixel() / PR_MAX(1, roundedDPIScaleFactor));
        } else {
            mAppUnitsPerDevNotScaledPixel =
                PR_MAX(1, static_cast<PRInt32>(AppUnitsPerCSSPixel() /
                                               prefDevPixelsPerCSSPixel));
        }
    } else {
        



        mAppUnitsPerDevNotScaledPixel = (AppUnitsPerCSSPixel() * 96) / dpi;
    }

    mAppUnitsPerInch = NSIntPixelsToAppUnits(dpi, mAppUnitsPerDevNotScaledPixel);

    UpdateScaledAppUnits();

    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::Init(nsIWidget *aWidget)
{
    mWidget = aWidget;

    SetDPI();

#ifdef NS_DEBUG
    NS_ASSERTION(!mInitialized, "device context is initialized twice!");
    mInitialized = PR_TRUE;
#endif

    
    
    nsCOMPtr<nsIObserverService> obs(do_GetService("@mozilla.org/observer-service;1"));
    if (obs)
        obs->AddObserver(this, "memory-pressure", PR_TRUE);

#if defined(MOZ_ENABLE_GTK2) && defined(MOZ_X11)
    if (getenv ("MOZ_X_SYNC")) {
        PR_LOG (gThebesGFXLog, PR_LOG_DEBUG, ("+++ Enabling XSynchronize\n"));
        XSynchronize (gdk_x11_get_default_xdisplay(), True);
        XSetErrorHandler(x11_error_handler);
    }

#endif

    mScreenManager = do_GetService("@mozilla.org/gfx/screenmanager;1");

    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::CreateRenderingContext(nsIView *aView,
                                              nsIRenderingContext *&aContext)
{
    
    NS_ENSURE_ARG_POINTER(aView);
    NS_PRECONDITION(aView->HasWidget(), "View has no widget!");

    nsCOMPtr<nsIWidget> widget;
    widget = aView->GetWidget();

    return CreateRenderingContext(widget, aContext);
}

NS_IMETHODIMP
nsThebesDeviceContext::CreateRenderingContext(nsIWidget *aWidget,
                                              nsIRenderingContext *&aContext)
{
    nsresult rv;

    aContext = nsnull;
    nsCOMPtr<nsIRenderingContext> pContext;
    rv = CreateRenderingContextInstance(*getter_AddRefs(pContext));
    if (NS_SUCCEEDED(rv)) {
        nsRefPtr<gfxASurface> surface(aWidget->GetThebesSurface());
        if (surface)
            rv = pContext->Init(this, surface);
        else
            rv = NS_ERROR_FAILURE;

        if (NS_SUCCEEDED(rv)) {
            aContext = pContext;
            NS_ADDREF(aContext);
        }
    }

    return rv;
}

NS_IMETHODIMP
nsThebesDeviceContext::CreateRenderingContext(nsIRenderingContext *&aContext)
{
    nsresult rv = NS_OK;

    aContext = nsnull;
    nsCOMPtr<nsIRenderingContext> pContext;
    rv = CreateRenderingContextInstance(*getter_AddRefs(pContext));
    if (NS_SUCCEEDED(rv)) {
        if (mPrintingSurface)
            rv = pContext->Init(this, mPrintingSurface);
        else
            rv = NS_ERROR_FAILURE;

        if (NS_SUCCEEDED(rv)) {
            pContext->Scale(mPrintingScale, mPrintingScale);
            aContext = pContext;
            NS_ADDREF(aContext);
        }
    }

    return rv;
}

NS_IMETHODIMP
nsThebesDeviceContext::CreateRenderingContextInstance(nsIRenderingContext *&aContext)
{
    nsCOMPtr<nsIRenderingContext> renderingContext = new nsThebesRenderingContext();
    if (!renderingContext)
        return NS_ERROR_OUT_OF_MEMORY;

    aContext = renderingContext;
    NS_ADDREF(aContext);

    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::SupportsNativeWidgets(PRBool &aSupportsWidgets)
{
    aSupportsWidgets = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::ClearCachedSystemFonts()
{
    
    if (gSystemFonts) {
        delete gSystemFonts;
        gSystemFonts = nsnull;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::GetSystemFont(nsSystemFontID aID, nsFont *aFont) const
{
    if (!gSystemFonts) {
#ifdef MOZ_ENABLE_GTK2
        gSystemFonts = new nsSystemFontsGTK2();
#elif XP_WIN
        gSystemFonts = new nsSystemFontsWin();
#elif XP_OS2
        gSystemFonts = new nsSystemFontsOS2();
#elif defined(XP_BEOS)
        gSystemFonts = new nsSystemFontsBeOS();
#elif XP_MACOSX
        gSystemFonts = new nsSystemFontsMac();
#elif defined(MOZ_WIDGET_QT)
        gSystemFonts = new nsSystemFontsQt();
#else
#error Need to know how to create gSystemFonts, fix me!
#endif
    }

    nsString fontName;
    gfxFontStyle fontStyle;
    nsresult rv = gSystemFonts->GetSystemFont(aID, &fontName, &fontStyle);
    NS_ENSURE_SUCCESS(rv, rv);

    aFont->name = fontName;
    aFont->style = fontStyle.style;
    aFont->systemFont = fontStyle.systemFont;
    aFont->variant = NS_FONT_VARIANT_NORMAL;
    aFont->familyNameQuirks = fontStyle.familyNameQuirks;
    aFont->weight = fontStyle.weight;
    aFont->stretch = fontStyle.stretch;
    aFont->decorations = NS_FONT_DECORATION_NONE;
    aFont->size = NSFloatPixelsToAppUnits(fontStyle.size, UnscaledAppUnitsPerDevPixel());
    
    aFont->sizeAdjust = fontStyle.sizeAdjust;

    return rv;
}

NS_IMETHODIMP
nsThebesDeviceContext::CheckFontExistence(const nsString& aFaceName)
{
    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::GetDepth(PRUint32& aDepth)
{
    if (mDepth == 0) {
        nsCOMPtr<nsIScreen> primaryScreen;
        mScreenManager->GetPrimaryScreen(getter_AddRefs(primaryScreen));
        primaryScreen->GetColorDepth(reinterpret_cast<PRInt32 *>(&mDepth));
    }

    aDepth = mDepth;
    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::ConvertPixel(nscolor aColor, PRUint32 & aPixel)
{
    aPixel = aColor;
    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::GetDeviceSurfaceDimensions(nscoord &aWidth, nscoord &aHeight)
{
    if (mPrintingSurface) {
        
        aWidth = mWidth;
        aHeight = mHeight;
    } else {
        nsRect area;
        ComputeFullAreaUsingScreen(&area);
        aWidth = area.width;
        aHeight = area.height;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::GetRect(nsRect &aRect)
{
    if (mPrintingSurface) {
        
        aRect.x = 0;
        aRect.y = 0;
        aRect.width = mWidth;
        aRect.height = mHeight;
    } else
        ComputeFullAreaUsingScreen ( &aRect );

    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::GetClientRect(nsRect &aRect)
{
    if (mPrintingSurface) {
        
        aRect.x = 0;
        aRect.y = 0;
        aRect.width = mWidth;
        aRect.height = mHeight;
    }
    else
        ComputeClientRectUsingScreen(&aRect);

    return NS_OK;
}

NS_IMETHODIMP
nsThebesDeviceContext::PrepareNativeWidget(nsIWidget* aWidget, void** aOut)
{
    *aOut = nsnull;
    return NS_OK;
}





NS_IMETHODIMP
nsThebesDeviceContext::InitForPrinting(nsIDeviceContextSpec *aDevice)
{
    NS_ENSURE_ARG_POINTER(aDevice);

    mDeviceContextSpec = aDevice;

    nsresult rv = aDevice->GetSurfaceForPrinter(getter_AddRefs(mPrintingSurface));
    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    Init(nsnull);

    CalcPrintingSize();

    return NS_OK;
}


NS_IMETHODIMP
nsThebesDeviceContext::PrepareDocument(PRUnichar * aTitle,
                                       PRUnichar*  aPrintToFileName)
{
    return NS_OK;
}


NS_IMETHODIMP
nsThebesDeviceContext::BeginDocument(PRUnichar*  aTitle,
                                     PRUnichar*  aPrintToFileName,
                                     PRInt32     aStartPage,
                                     PRInt32     aEndPage)
{
    static const PRUnichar kEmpty[] = { '\0' };
    nsresult rv;

    rv = mPrintingSurface->BeginPrinting(nsDependentString(aTitle ? aTitle : kEmpty),
                                         nsDependentString(aPrintToFileName ? aPrintToFileName : kEmpty));

    if (NS_SUCCEEDED(rv) && mDeviceContextSpec)
        rv = mDeviceContextSpec->BeginDocument(aTitle, aPrintToFileName, aStartPage, aEndPage);

    return rv;
}


NS_IMETHODIMP
nsThebesDeviceContext::EndDocument(void)
{
    nsresult rv = NS_OK;

    if (mPrintingSurface) {
        rv = mPrintingSurface->EndPrinting();
        if (NS_SUCCEEDED(rv))
            mPrintingSurface->Finish();
    }

    if (mDeviceContextSpec)
        mDeviceContextSpec->EndDocument();

    return rv;
}


NS_IMETHODIMP
nsThebesDeviceContext::AbortDocument(void)
{
    nsresult rv = mPrintingSurface->AbortPrinting();

    if (mDeviceContextSpec)
        mDeviceContextSpec->EndDocument();

    return rv;
}


NS_IMETHODIMP
nsThebesDeviceContext::BeginPage(void)
{
    nsresult rv = NS_OK;

    if (mDeviceContextSpec)
        rv = mDeviceContextSpec->BeginPage();

    if (NS_FAILED(rv)) return rv;

   
#ifdef XP_MACOSX
    mDeviceContextSpec->GetSurfaceForPrinter(getter_AddRefs(mPrintingSurface));
#endif
    rv = mPrintingSurface->BeginPage();

    return rv;
}

NS_IMETHODIMP
nsThebesDeviceContext::EndPage(void)
{
    nsresult rv = mPrintingSurface->EndPage();

    


#ifdef XP_MACOSX
    mPrintingSurface = nsnull;
#endif

    if (mDeviceContextSpec)
        mDeviceContextSpec->EndPage();

    return rv;
}



void
nsThebesDeviceContext::ComputeClientRectUsingScreen(nsRect* outRect)
{
    
    
    
    
    nsCOMPtr<nsIScreen> screen;
    FindScreen (getter_AddRefs(screen));
    if (screen) {
        PRInt32 x, y, width, height;
        screen->GetAvailRect(&x, &y, &width, &height);

        
        outRect->y = NSIntPixelsToAppUnits(y, AppUnitsPerDevPixel());
        outRect->x = NSIntPixelsToAppUnits(x, AppUnitsPerDevPixel());
        outRect->width = NSIntPixelsToAppUnits(width, AppUnitsPerDevPixel());
        outRect->height = NSIntPixelsToAppUnits(height, AppUnitsPerDevPixel());
    }
}

void
nsThebesDeviceContext::ComputeFullAreaUsingScreen(nsRect* outRect)
{
    
    
    
    
    nsCOMPtr<nsIScreen> screen;
    FindScreen ( getter_AddRefs(screen) );
    if ( screen ) {
        PRInt32 x, y, width, height;
        screen->GetRect ( &x, &y, &width, &height );

        
        outRect->y = NSIntPixelsToAppUnits(y, AppUnitsPerDevPixel());
        outRect->x = NSIntPixelsToAppUnits(x, AppUnitsPerDevPixel());
        outRect->width = NSIntPixelsToAppUnits(width, AppUnitsPerDevPixel());
        outRect->height = NSIntPixelsToAppUnits(height, AppUnitsPerDevPixel());

        mWidth = outRect->width;
        mHeight = outRect->height;
    }
}






void
nsThebesDeviceContext::FindScreen(nsIScreen** outScreen)
{
    if (mWidget && mWidget->GetNativeData(NS_NATIVE_WINDOW))
        mScreenManager->ScreenForNativeWidget(mWidget->GetNativeData(NS_NATIVE_WINDOW),
                                              outScreen);
    else
        mScreenManager->GetPrimaryScreen(outScreen);
}

void
nsThebesDeviceContext::CalcPrintingSize()
{
    if (!mPrintingSurface)
        return;

    PRBool inPoints = PR_TRUE;

    gfxSize size;
    switch (mPrintingSurface->GetType()) {
    case gfxASurface::SurfaceTypeImage:
        inPoints = PR_FALSE;
        size = reinterpret_cast<gfxImageSurface*>(mPrintingSurface.get())->GetSize();
        break;

#if defined(MOZ_ENABLE_GTK2) || defined(XP_WIN) || defined(XP_OS2)
    case gfxASurface::SurfaceTypePDF:
        inPoints = PR_TRUE;
        size = reinterpret_cast<gfxPDFSurface*>(mPrintingSurface.get())->GetSize();
        break;
#endif

#ifdef MOZ_ENABLE_GTK2
    case gfxASurface::SurfaceTypePS:
        inPoints = PR_TRUE;
        size = reinterpret_cast<gfxPSSurface*>(mPrintingSurface.get())->GetSize();
        break;
#endif

#ifdef XP_MACOSX
    case gfxASurface::SurfaceTypeQuartz:
        inPoints = PR_TRUE; 
        size = reinterpret_cast<gfxQuartzSurface*>(mPrintingSurface.get())->GetSize();
        break;
#endif

#ifdef XP_WIN
    case gfxASurface::SurfaceTypeWin32:
    case gfxASurface::SurfaceTypeWin32Printing:
    {
        inPoints = PR_FALSE;
        HDC dc =  GetPrintHDC();
        if (!dc)
            dc = GetDC((HWND)mWidget->GetNativeData(NS_NATIVE_WIDGET));
        size.width = NSFloatPixelsToAppUnits(::GetDeviceCaps(dc, HORZRES)/mPrintingScale, AppUnitsPerDevPixel());
        size.height = NSFloatPixelsToAppUnits(::GetDeviceCaps(dc, VERTRES)/mPrintingScale, AppUnitsPerDevPixel());
        mDepth = (PRUint32)::GetDeviceCaps(dc, BITSPIXEL);
        if (dc != (HDC)GetPrintHDC())
            ReleaseDC((HWND)mWidget->GetNativeData(NS_NATIVE_WIDGET), dc);
        break;
    }
#endif

#ifdef XP_OS2
    case gfxASurface::SurfaceTypeOS2:
    {
        inPoints = PR_FALSE;
        
        
        size = reinterpret_cast<gfxOS2Surface*>(mPrintingSurface.get())->GetSize();
        
        size.width = NSFloatPixelsToAppUnits(size.width, AppUnitsPerDevPixel());
        size.height = NSFloatPixelsToAppUnits(size.height, AppUnitsPerDevPixel());
        
        HDC dc = GetPrintHDC();
        LONG value;
        if (DevQueryCaps(dc, CAPS_COLOR_BITCOUNT, 1, &value))
            mDepth = value;
        else
            mDepth = 8; 
        break;
    }
#endif
    default:
        NS_ERROR("trying to print to unknown surface type");
    }

    if (inPoints) {
        mWidth = NSToCoordRound(float(size.width) * AppUnitsPerInch() / 72);
        mHeight = NSToCoordRound(float(size.height) * AppUnitsPerInch() / 72);
    } else {
        mWidth = NSToIntRound(size.width);
        mHeight = NSToIntRound(size.height);
    }
}

PRBool nsThebesDeviceContext::CheckDPIChange() {
    PRInt32 oldDevPixels = mAppUnitsPerDevNotScaledPixel;
    PRInt32 oldInches = mAppUnitsPerInch;

    SetDPI();

    return oldDevPixels != mAppUnitsPerDevNotScaledPixel ||
           oldInches != mAppUnitsPerInch;
}

PRBool
nsThebesDeviceContext::SetPixelScale(float aScale)
{
    if (aScale <= 0) {
        NS_NOTREACHED("Invalid pixel scale value");
        return PR_FALSE;
    }
    PRInt32 oldAppUnitsPerDevPixel = mAppUnitsPerDevPixel;
    mPixelScale = aScale;
    UpdateScaledAppUnits();
    return oldAppUnitsPerDevPixel != mAppUnitsPerDevPixel;
}

void
nsThebesDeviceContext::UpdateScaledAppUnits()
{
    mAppUnitsPerDevPixel = PR_MAX(1, PRInt32(float(mAppUnitsPerDevNotScaledPixel) / mPixelScale));
}

#if defined(XP_WIN) || defined(XP_OS2)
HDC
nsThebesDeviceContext::GetPrintHDC()
{
    if (mPrintingSurface) {
        switch (mPrintingSurface->GetType()) {
#ifdef XP_WIN
            case gfxASurface::SurfaceTypeWin32:
            case gfxASurface::SurfaceTypeWin32Printing:
                return reinterpret_cast<gfxWindowsSurface*>(mPrintingSurface.get())->GetDC();
#endif

#ifdef XP_OS2
            case gfxASurface::SurfaceTypeOS2:
                return GpiQueryDevice(reinterpret_cast<gfxOS2Surface*>(mPrintingSurface.get())->GetPS());
#endif

            default:
                NS_ERROR("invalid surface type in GetPrintHDC");
                break;
        }
    }

    return nsnull;
}
#endif
