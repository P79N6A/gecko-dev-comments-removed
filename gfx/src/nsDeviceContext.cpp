




#include "nsDeviceContext.h"
#include <algorithm>                    
#include "gfxASurface.h"                
#include "gfxFont.h"                    
#include "gfxImageSurface.h"            
#include "gfxPoint.h"                   
#include "mozilla/Attributes.h"         
#include "mozilla/Preferences.h"        
#include "mozilla/Services.h"           
#include "mozilla/mozalloc.h"           
#include "nsCRT.h"                      
#include "nsDebug.h"                    
#include "nsFont.h"                     
#include "nsFontMetrics.h"              
#include "nsIAtom.h"                    
#include "nsID.h"
#include "nsIDeviceContextSpec.h"       
#include "nsILanguageAtomService.h"     
#include "nsIObserver.h"                
#include "nsIObserverService.h"         
#include "nsIScreen.h"                  
#include "nsIScreenManager.h"           
#include "nsISupportsImpl.h"            
#include "nsISupportsUtils.h"           
#include "nsIWidget.h"                  
#include "nsRect.h"                     
#include "nsRenderingContext.h"         
#include "nsServiceManagerUtils.h"      
#include "nsString.h"               
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              

#if !XP_MACOSX
#include "gfxPDFSurface.h"
#endif

#ifdef MOZ_WIDGET_GTK
#include "gfxPSSurface.h"
#elif XP_WIN
#include "gfxWindowsSurface.h"
#elif XP_MACOSX
#include "gfxQuartzSurface.h"
#endif

using namespace mozilla;
using mozilla::services::GetObserverService;

class nsFontCache MOZ_FINAL : public nsIObserver
{
public:
    nsFontCache()   { MOZ_COUNT_CTOR(nsFontCache); }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    void Init(nsDeviceContext* aContext);
    void Destroy();

    nsresult GetMetricsFor(const nsFont& aFont, nsIAtom* aLanguage,
                           gfxUserFontSet* aUserFontSet,
                           gfxTextPerfMetrics* aTextPerf,
                           nsFontMetrics*& aMetrics);

    void FontMetricsDeleted(const nsFontMetrics* aFontMetrics);
    void Compact();
    void Flush();

protected:
    ~nsFontCache()  { MOZ_COUNT_DTOR(nsFontCache); }

    nsDeviceContext*          mContext; 
    nsCOMPtr<nsIAtom>         mLocaleLanguage;
    nsTArray<nsFontMetrics*>  mFontMetrics;
};

NS_IMPL_ISUPPORTS(nsFontCache, nsIObserver)




void
nsFontCache::Init(nsDeviceContext* aContext)
{
    mContext = aContext;
    
    
    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    if (obs)
        obs->AddObserver(this, "memory-pressure", false);

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
nsFontCache::Observe(nsISupports*, const char* aTopic, const char16_t*)
{
    if (!nsCRT::strcmp(aTopic, "memory-pressure"))
        Compact();
    return NS_OK;
}

nsresult
nsFontCache::GetMetricsFor(const nsFont& aFont, nsIAtom* aLanguage,
                           gfxUserFontSet* aUserFontSet,
                           gfxTextPerfMetrics* aTextPerf,
                           nsFontMetrics*& aMetrics)
{
    if (!aLanguage)
        aLanguage = mLocaleLanguage;

    
    

    nsFontMetrics* fm;
    int32_t n = mFontMetrics.Length() - 1;
    for (int32_t i = n; i >= 0; --i) {
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
    nsresult rv = fm->Init(aFont, aLanguage, mContext, aUserFontSet, aTextPerf);
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
    rv = fm->Init(aFont, aLanguage, mContext, aUserFontSet, aTextPerf);
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
    
    
    for (int32_t i = mFontMetrics.Length()-1; i >= 0; --i) {
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
    for (int32_t i = mFontMetrics.Length()-1; i >= 0; --i) {
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
      mFontCache(nullptr)
{
    MOZ_ASSERT(NS_IsMainThread(), "nsDeviceContext created off main thread");
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
                               gfxTextPerfMetrics* aTextPerf,
                               nsFontMetrics*& aMetrics)
{
    if (!mFontCache) {
        mFontCache = new nsFontCache();
        NS_ADDREF(mFontCache);
        mFontCache->Init(this);
    }

    return mFontCache->GetMetricsFor(aFont, aLanguage, aUserFontSet,
                                     aTextPerf, aMetrics);
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

bool
nsDeviceContext::IsPrinterSurface()
{
    return mPrintingSurface != nullptr;
}

void
nsDeviceContext::SetDPI()
{
    float dpi = -1.0f;

    
    
    if (mPrintingSurface) {
        switch (mPrintingSurface->GetType()) {
        case gfxSurfaceType::PDF:
        case gfxSurfaceType::PS:
        case gfxSurfaceType::Quartz:
            dpi = 72.0f;
            break;
#ifdef XP_WIN
        case gfxSurfaceType::Win32:
        case gfxSurfaceType::Win32Printing: {
            HDC dc = reinterpret_cast<gfxWindowsSurface*>(mPrintingSurface.get())->GetDC();
            int32_t OSVal = GetDeviceCaps(dc, LOGPIXELSY);
            dpi = 144.0f;
            mPrintingScale = float(OSVal) / dpi;
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
        
        
        
        
        int32_t prefDPI = Preferences::GetInt("layout.css.dpi", -1);

        if (prefDPI > 0) {
            dpi = prefDPI;
        } else if (mWidget) {
            dpi = mWidget->GetDPI();

            if (prefDPI < 0) {
                dpi = std::max(96.0f, dpi);
            }
        } else {
            dpi = 96.0f;
        }

        CSSToLayoutDeviceScale scale = mWidget ? mWidget->GetDefaultScale()
                                               : CSSToLayoutDeviceScale(1.0);
        double devPixelsPerCSSPixel = scale.scale;

        mAppUnitsPerDevNotScaledPixel =
            std::max(1, NS_lround(AppUnitsPerCSSPixel() / devPixelsPerCSSPixel));
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

already_AddRefed<nsRenderingContext>
nsDeviceContext::CreateRenderingContext()
{
    nsRefPtr<gfxASurface> printingSurface = mPrintingSurface;
#ifdef XP_MACOSX
    
    
    
    
    
    
    if (!printingSurface) {
      printingSurface = mCachedPrintingSurface;
    }
#endif
    nsRefPtr<nsRenderingContext> pContext = new nsRenderingContext();

    RefPtr<gfx::DrawTarget> dt =
      gfxPlatform::GetPlatform()->CreateDrawTargetForSurface(printingSurface,
                                                             gfx::IntSize(mWidth, mHeight));

#ifdef XP_MACOSX
    dt->AddUserData(&gfxContext::sDontUseAsSourceKey, dt, nullptr);
#endif

    pContext->Init(this, dt);
    pContext->ThebesContext()->SetFlag(gfxContext::FLAG_DISABLE_SNAPPING);
    pContext->ThebesContext()->SetMatrix(gfxMatrix::Scaling(mPrintingScale,
                                                            mPrintingScale));

    return pContext.forget();
}

nsresult
nsDeviceContext::GetDepth(uint32_t& aDepth)
{
    if (mDepth == 0) {
        nsCOMPtr<nsIScreen> primaryScreen;
        mScreenManager->GetPrimaryScreen(getter_AddRefs(primaryScreen));
        primaryScreen->GetColorDepth(reinterpret_cast<int32_t *>(&mDepth));
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

    Init(nullptr);

    CalcPrintingSize();

    return NS_OK;
}

nsresult
nsDeviceContext::BeginDocument(const nsAString& aTitle,
                               char16_t*       aPrintToFileName,
                               int32_t          aStartPage,
                               int32_t          aEndPage)
{
    static const char16_t kEmpty[] = { '\0' };
    nsresult rv;

    rv = mPrintingSurface->BeginPrinting(aTitle,
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
    
    
    
    
    
    
    
    mCachedPrintingSurface = mPrintingSurface;
    mPrintingSurface = nullptr;
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
        int32_t x, y, width, height;
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
        int32_t x, y, width, height;
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
    if (mWidget && mWidget->GetOwningTabChild()) {
        mScreenManager->ScreenForNativeWidget((void *)mWidget->GetOwningTabChild(),
                                              outScreen);
    }
    else if (mWidget && mWidget->GetNativeData(NS_NATIVE_WINDOW)) {
        mScreenManager->ScreenForNativeWidget(mWidget->GetNativeData(NS_NATIVE_WINDOW),
                                              outScreen);
    }
    else {
        mScreenManager->GetPrimaryScreen(outScreen);
    }
}

void
nsDeviceContext::CalcPrintingSize()
{
    if (!mPrintingSurface)
        return;

    bool inPoints = true;

    gfxSize size(0, 0);
    switch (mPrintingSurface->GetType()) {
    case gfxSurfaceType::Image:
        inPoints = false;
        size = reinterpret_cast<gfxImageSurface*>(mPrintingSurface.get())->GetSize();
        break;

#if defined(MOZ_PDF_PRINTING)
    case gfxSurfaceType::PDF:
        inPoints = true;
        size = reinterpret_cast<gfxPDFSurface*>(mPrintingSurface.get())->GetSize();
        break;
#endif

#ifdef MOZ_WIDGET_GTK
    case gfxSurfaceType::PS:
        inPoints = true;
        size = reinterpret_cast<gfxPSSurface*>(mPrintingSurface.get())->GetSize();
        break;
#endif

#ifdef XP_MACOSX
    case gfxSurfaceType::Quartz:
        inPoints = true; 
        size = reinterpret_cast<gfxQuartzSurface*>(mPrintingSurface.get())->GetSize();
        break;
#endif

#ifdef XP_WIN
    case gfxSurfaceType::Win32:
    case gfxSurfaceType::Win32Printing:
        {
            inPoints = false;
            HDC dc = reinterpret_cast<gfxWindowsSurface*>(mPrintingSurface.get())->GetDC();
            if (!dc)
                dc = GetDC((HWND)mWidget->GetNativeData(NS_NATIVE_WIDGET));
            size.width = NSFloatPixelsToAppUnits(::GetDeviceCaps(dc, HORZRES)/mPrintingScale, AppUnitsPerDevPixel());
            size.height = NSFloatPixelsToAppUnits(::GetDeviceCaps(dc, VERTRES)/mPrintingScale, AppUnitsPerDevPixel());
            mDepth = (uint32_t)::GetDeviceCaps(dc, BITSPIXEL);
            if (dc != reinterpret_cast<gfxWindowsSurface*>(mPrintingSurface.get())->GetDC())
                ReleaseDC((HWND)mWidget->GetNativeData(NS_NATIVE_WIDGET), dc);
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

bool nsDeviceContext::CheckDPIChange() {
    int32_t oldDevPixels = mAppUnitsPerDevNotScaledPixel;
    int32_t oldInches = mAppUnitsPerPhysicalInch;

    SetDPI();

    return oldDevPixels != mAppUnitsPerDevNotScaledPixel ||
        oldInches != mAppUnitsPerPhysicalInch;
}

bool
nsDeviceContext::SetPixelScale(float aScale)
{
    if (aScale <= 0) {
        NS_NOTREACHED("Invalid pixel scale value");
        return false;
    }
    int32_t oldAppUnitsPerDevPixel = mAppUnitsPerDevPixel;
    mPixelScale = aScale;
    UpdateScaledAppUnits();
    return oldAppUnitsPerDevPixel != mAppUnitsPerDevPixel;
}

void
nsDeviceContext::UpdateScaledAppUnits()
{
    mAppUnitsPerDevPixel =
        std::max(1, NSToIntRound(float(mAppUnitsPerDevNotScaledPixel) / mPixelScale));
    
    mPixelScale = float(mAppUnitsPerDevNotScaledPixel) / mAppUnitsPerDevPixel;
}
