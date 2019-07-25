






































#include "nsDeviceContext.h"
#include "nsCRT.h"
#include "nsFontMetrics.h"
#include "nsRenderingContext.h"
#include "nsIView.h"
#include "nsIWidget.h"

#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "nsIServiceManager.h"
#include "nsILanguageAtomService.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"

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
#elif defined(XP_OS2)
#include "nsSystemFontsOS2.h"
#include "gfxOS2Surface.h"
#include "gfxPDFSurface.h"
static nsSystemFontsOS2 *gSystemFonts = nsnull;
#elif XP_MACOSX
#include "nsSystemFontsMac.h"
#include "gfxQuartzSurface.h"
static nsSystemFontsMac *gSystemFonts = nsnull;
#elif defined(MOZ_WIDGET_QT)
#include "nsSystemFontsQt.h"
#include "gfxPDFSurface.h"
static nsSystemFontsQt *gSystemFonts = nsnull;
#elif defined(ANDROID)
#include "nsSystemFontsAndroid.h"
#include "gfxPDFSurface.h"
static nsSystemFontsAndroid *gSystemFonts = nsnull;
#else
#error Need to declare gSystemFonts!
#endif

using namespace mozilla;
using mozilla::services::GetObserverService;

class nsFontCache : public nsIObserver
{
public:
    nsFontCache()   { MOZ_COUNT_CTOR(nsFontCache); }
    ~nsFontCache()  { MOZ_COUNT_DTOR(nsFontCache); }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    void Init(nsDeviceContext* aContext);
    void Destroy();

    nsresult GetMetricsFor(const nsFont& aFont, nsIAtom* aLanguage,
                           gfxUserFontSet* aUserFontSet,
                           nsFontMetrics*& aMetrics);

    void FontMetricsDeleted(const nsFontMetrics* aFontMetrics);
    void Compact();
    void Flush();

protected:
    nsDeviceContext*          mContext; 
    nsCOMPtr<nsIAtom>         mLocaleLanguage;
    nsTArray<nsFontMetrics*>  mFontMetrics;
};

NS_IMPL_ISUPPORTS1(nsFontCache, nsIObserver)




void
nsFontCache::Init(nsDeviceContext* aContext)
{
    mContext = aContext;
    
    
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    if (obs)
        obs->AddObserver(this, "memory-pressure", PR_FALSE);

    nsCOMPtr<nsILanguageAtomService> langService;
    langService = do_GetService(NS_LANGUAGEATOMSERVICE_CONTRACTID);
    if (langService) {
        mLocaleLanguage = langService->GetLocaleLanguage();
    }
    if (!mLocaleLanguage) {
        mLocaleLanguage = do_GetAtom("x-western");
    }
}

void
nsFontCache::Destroy()
{
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    if (obs)
        obs->RemoveObserver(this, "memory-pressure");
    Flush();
}

NS_IMETHODIMP
nsFontCache::Observe(nsISupports*, const char* aTopic, const PRUnichar*)
{
    if (!nsCRT::strcmp(aTopic, "memory-pressure"))
        Compact();
    return NS_OK;
}

nsresult
nsFontCache::GetMetricsFor(const nsFont& aFont, nsIAtom* aLanguage,
                           gfxUserFontSet* aUserFontSet,
                           nsFontMetrics*& aMetrics)
{
    if (!aLanguage)
        aLanguage = mLocaleLanguage;

    
    

    nsFontMetrics* fm;
    PRInt32 n = mFontMetrics.Length() - 1;
    for (PRInt32 i = n; i >= 0; --i) {
        fm = mFontMetrics[i];
        if (fm->Font().Equals(aFont) && fm->GetUserFontSet() == aUserFontSet &&
            fm->Language() == aLanguage) {
            if (i != n) {
                
                mFontMetrics.RemoveElementAt(i);
                mFontMetrics.AppendElement(fm);
            }
            fm->GetThebesFontGroup()->UpdateFontList();
            NS_ADDREF(aMetrics = fm);
            return NS_OK;
        }
    }

    

    fm = new nsFontMetrics();
    NS_ADDREF(fm);
    nsresult rv = fm->Init(aFont, aLanguage, mContext, aUserFontSet);
    if (NS_SUCCEEDED(rv)) {
        
        
        mFontMetrics.AppendElement(fm);
        aMetrics = fm;
        NS_ADDREF(aMetrics);
        return NS_OK;
    }
    fm->Destroy();
    NS_RELEASE(fm);

    
    
    

    Compact();
    fm = new nsFontMetrics();
    NS_ADDREF(fm);
    rv = fm->Init(aFont, aLanguage, mContext, aUserFontSet);
    if (NS_SUCCEEDED(rv)) {
        mFontMetrics.AppendElement(fm);
        aMetrics = fm;
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

    NS_POSTCONDITION(NS_SUCCEEDED(rv),
                     "font metrics should not be null - bug 136248");
    return rv;
}

void
nsFontCache::FontMetricsDeleted(const nsFontMetrics* aFontMetrics)
{
    mFontMetrics.RemoveElement(aFontMetrics);
}

void
nsFontCache::Compact()
{
    
    
    for (PRInt32 i = mFontMetrics.Length()-1; i >= 0; --i) {
        nsFontMetrics* fm = mFontMetrics[i];
        nsFontMetrics* oldfm = fm;
        
        
        NS_RELEASE(fm); 
        
        
        if (mFontMetrics.IndexOf(oldfm) != mFontMetrics.NoIndex) {
            
            NS_ADDREF(oldfm);
        }
    }
}

void
nsFontCache::Flush()
{
    for (PRInt32 i = mFontMetrics.Length()-1; i >= 0; --i) {
        nsFontMetrics* fm = mFontMetrics[i];
        
        
        
        fm->Destroy();
        NS_RELEASE(fm);
    }
    mFontMetrics.Clear();
}

nsDeviceContext::nsDeviceContext()
    : mWidth(0), mHeight(0), mDepth(0),
      mAppUnitsPerDevPixel(-1), mAppUnitsPerDevNotScaledPixel(-1),
      mAppUnitsPerPhysicalInch(-1),
      mPixelScale(1.0f), mPrintingScale(1.0f),
      mFontCache(nsnull)
{
}




nsDeviceContext::~nsDeviceContext()
{
    if (mFontCache) {
        mFontCache->Destroy();
        NS_RELEASE(mFontCache);
    }
}

nsresult
nsDeviceContext::GetMetricsFor(const nsFont& aFont,
                               nsIAtom* aLanguage,
                               gfxUserFontSet* aUserFontSet,
                               nsFontMetrics*& aMetrics)
{
    if (!mFontCache) {
        mFontCache = new nsFontCache();
        NS_ADDREF(mFontCache);
        mFontCache->Init(this);
    }

    return mFontCache->GetMetricsFor(aFont, aLanguage, aUserFontSet, aMetrics);
}

nsresult
nsDeviceContext::FlushFontCache(void)
{
    if (mFontCache)
        mFontCache->Flush();
    return NS_OK;
}

nsresult
nsDeviceContext::FontMetricsDeleted(const nsFontMetrics* aFontMetrics)
{
    if (mFontCache) {
        mFontCache->FontMetricsDeleted(aFontMetrics);
    }
    return NS_OK;
}

PRBool
nsDeviceContext::IsPrinterSurface()
{
    return(mPrintingSurface != NULL);
}

void
nsDeviceContext::SetDPI()
{
    float dpi = -1.0f;

    
    
    if (mPrintingSurface) {
        switch (mPrintingSurface->GetType()) {
        case gfxASurface::SurfaceTypePDF:
        case gfxASurface::SurfaceTypePS:
        case gfxASurface::SurfaceTypeQuartz:
            dpi = 72.0f;
            break;
#ifdef XP_WIN
        case gfxASurface::SurfaceTypeWin32:
        case gfxASurface::SurfaceTypeWin32Printing: {
            HDC dc = reinterpret_cast<gfxWindowsSurface*>(mPrintingSurface.get())->GetDC();
            PRInt32 OSVal = GetDeviceCaps(dc, LOGPIXELSY);
            dpi = 144.0f;
            mPrintingScale = float(OSVal) / dpi;
            break;
        }
#endif
#ifdef XP_OS2
        case gfxASurface::SurfaceTypeOS2: {
            LONG lDPI;
            HDC dc = GpiQueryDevice(reinterpret_cast<gfxOS2Surface*>(mPrintingSurface.get())->GetPS());
            if (DevQueryCaps(dc, CAPS_VERTICAL_FONT_RES, 1, &lDPI))
                dpi = lDPI;
            break;
        }
#endif
        default:
            NS_NOTREACHED("Unexpected printing surface type");
            break;
        }

        mAppUnitsPerDevNotScaledPixel =
            NS_lround((AppUnitsPerCSSPixel() * 96) / dpi);
    } else {
        
        
        
        
        PRInt32 prefDPI = Preferences::GetInt("layout.css.dpi", -1);

        if (prefDPI > 0) {
            dpi = prefDPI;
        } else if (mWidget) {
            dpi = mWidget->GetDPI();

            if (prefDPI < 0) {
                dpi = PR_MAX(96.0f, dpi);
            }
        } else {
            dpi = 96.0f;
        }

        
        
        
        float devPixelsPerCSSPixel = -1.0;

        nsAdoptingCString prefString = Preferences::GetCString("layout.css.devPixelsPerPx");
        if (!prefString.IsEmpty()) {
            devPixelsPerCSSPixel = static_cast<float>(atof(prefString));
        }

        if (devPixelsPerCSSPixel <= 0) {
            if (mWidget) {
                devPixelsPerCSSPixel = mWidget->GetDefaultScale();
            } else {
                devPixelsPerCSSPixel = 1.0;
            }
        }

        mAppUnitsPerDevNotScaledPixel =
            PR_MAX(1, NS_lround(AppUnitsPerCSSPixel() / devPixelsPerCSSPixel));
    }

    NS_ASSERTION(dpi != -1.0, "no dpi set");

    mAppUnitsPerPhysicalInch = NS_lround(dpi * mAppUnitsPerDevNotScaledPixel);
    UpdateScaledAppUnits();
}

nsresult
nsDeviceContext::Init(nsIWidget *aWidget)
{
    if (mScreenManager && mWidget == aWidget)
        return NS_OK;

    mWidget = aWidget;
    SetDPI();

    if (mScreenManager)
        return NS_OK;

    mScreenManager = do_GetService("@mozilla.org/gfx/screenmanager;1");

    return NS_OK;
}

nsresult
nsDeviceContext::CreateRenderingContext(nsRenderingContext *&aContext)
{
    NS_ABORT_IF_FALSE(mPrintingSurface, "only call for printing dcs");

    nsRefPtr<nsRenderingContext> pContext = new nsRenderingContext();

    pContext->Init(this, mPrintingSurface);
    pContext->Scale(mPrintingScale, mPrintingScale);
    aContext = pContext;
    NS_ADDREF(aContext);

    return NS_OK;
}

 void
nsDeviceContext::ClearCachedSystemFonts()
{
    if (gSystemFonts) {
        delete gSystemFonts;
        gSystemFonts = nsnull;
    }
}

nsresult
nsDeviceContext::GetSystemFont(nsSystemFontID aID, nsFont *aFont) const
{
    if (!gSystemFonts) {
#ifdef MOZ_ENABLE_GTK2
        gSystemFonts = new nsSystemFontsGTK2();
#elif XP_WIN
        gSystemFonts = new nsSystemFontsWin();
#elif XP_OS2
        gSystemFonts = new nsSystemFontsOS2();
#elif XP_MACOSX
        gSystemFonts = new nsSystemFontsMac();
#elif defined(MOZ_WIDGET_QT)
        gSystemFonts = new nsSystemFontsQt();
#elif defined(ANDROID)
        gSystemFonts = new nsSystemFontsAndroid();
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
    aFont->weight = fontStyle.weight;
    aFont->stretch = fontStyle.stretch;
    aFont->decorations = NS_FONT_DECORATION_NONE;
    aFont->size = NSFloatPixelsToAppUnits(fontStyle.size, UnscaledAppUnitsPerDevPixel());
    
    aFont->sizeAdjust = fontStyle.sizeAdjust;

    return rv;
}

nsresult
nsDeviceContext::GetDepth(PRUint32& aDepth)
{
    if (mDepth == 0) {
        nsCOMPtr<nsIScreen> primaryScreen;
        mScreenManager->GetPrimaryScreen(getter_AddRefs(primaryScreen));
        primaryScreen->GetColorDepth(reinterpret_cast<PRInt32 *>(&mDepth));
    }

    aDepth = mDepth;
    return NS_OK;
}

nsresult
nsDeviceContext::GetDeviceSurfaceDimensions(nscoord &aWidth, nscoord &aHeight)
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

nsresult
nsDeviceContext::GetRect(nsRect &aRect)
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

nsresult
nsDeviceContext::GetClientRect(nsRect &aRect)
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

nsresult
nsDeviceContext::InitForPrinting(nsIDeviceContextSpec *aDevice)
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

nsresult
nsDeviceContext::BeginDocument(PRUnichar*  aTitle,
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


nsresult
nsDeviceContext::EndDocument(void)
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


nsresult
nsDeviceContext::AbortDocument(void)
{
    nsresult rv = mPrintingSurface->AbortPrinting();

    if (mDeviceContextSpec)
        mDeviceContextSpec->EndDocument();

    return rv;
}


nsresult
nsDeviceContext::BeginPage(void)
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

nsresult
nsDeviceContext::EndPage(void)
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
nsDeviceContext::ComputeClientRectUsingScreen(nsRect* outRect)
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
nsDeviceContext::ComputeFullAreaUsingScreen(nsRect* outRect)
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
nsDeviceContext::FindScreen(nsIScreen** outScreen)
{
    if (mWidget && mWidget->GetNativeData(NS_NATIVE_WINDOW))
        mScreenManager->ScreenForNativeWidget(mWidget->GetNativeData(NS_NATIVE_WINDOW),
                                              outScreen);
    else
        mScreenManager->GetPrimaryScreen(outScreen);
}

void
nsDeviceContext::CalcPrintingSize()
{
    if (!mPrintingSurface)
        return;

    PRBool inPoints = PR_TRUE;

    gfxSize size(0, 0);
    switch (mPrintingSurface->GetType()) {
    case gfxASurface::SurfaceTypeImage:
        inPoints = PR_FALSE;
        size = reinterpret_cast<gfxImageSurface*>(mPrintingSurface.get())->GetSize();
        break;

#if defined(MOZ_PDF_PRINTING)
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
            HDC dc = reinterpret_cast<gfxWindowsSurface*>(mPrintingSurface.get())->GetDC();
            if (!dc)
                dc = GetDC((HWND)mWidget->GetNativeData(NS_NATIVE_WIDGET));
            size.width = NSFloatPixelsToAppUnits(::GetDeviceCaps(dc, HORZRES)/mPrintingScale, AppUnitsPerDevPixel());
            size.height = NSFloatPixelsToAppUnits(::GetDeviceCaps(dc, VERTRES)/mPrintingScale, AppUnitsPerDevPixel());
            mDepth = (PRUint32)::GetDeviceCaps(dc, BITSPIXEL);
            if (dc != reinterpret_cast<gfxWindowsSurface*>(mPrintingSurface.get())->GetDC())
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
            
            HDC dc = GpiQueryDevice(reinterpret_cast<gfxOS2Surface*>(mPrintingSurface.get())->GetPS());
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
        
        
        mWidth = NSToCoordRound(float(size.width) * AppUnitsPerPhysicalInch() / 72);
        mHeight = NSToCoordRound(float(size.height) * AppUnitsPerPhysicalInch() / 72);
    } else {
        mWidth = NSToIntRound(size.width);
        mHeight = NSToIntRound(size.height);
    }
}

PRBool nsDeviceContext::CheckDPIChange() {
    PRInt32 oldDevPixels = mAppUnitsPerDevNotScaledPixel;
    PRInt32 oldInches = mAppUnitsPerPhysicalInch;

    SetDPI();

    return oldDevPixels != mAppUnitsPerDevNotScaledPixel ||
        oldInches != mAppUnitsPerPhysicalInch;
}

PRBool
nsDeviceContext::SetPixelScale(float aScale)
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
nsDeviceContext::UpdateScaledAppUnits()
{
    mAppUnitsPerDevPixel =
        PR_MAX(1, NSToIntRound(float(mAppUnitsPerDevNotScaledPixel) / mPixelScale));
}
