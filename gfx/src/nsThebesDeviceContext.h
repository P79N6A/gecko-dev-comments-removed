






































#ifndef _NS_THEBESDEVICECONTEXT_H_
#define _NS_THEBESDEVICECONTEXT_H_

#include "nsIScreenManager.h"
#include "nsIDeviceContext.h"
#include "nsIDeviceContextSpec.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsWeakReference.h"
#include "gfxContext.h"

#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gThebesGFXLog;
#endif

#ifdef XP_WIN
#include "gfxWindowsSurface.h"
#elif defined(XP_OS2)
#include "gfxOS2Surface.h"
#endif

class nsFontCache;

class nsThebesDeviceContext : public nsIDeviceContext,
                              public nsIObserver,
                              public nsSupportsWeakReference
{
public:
    nsThebesDeviceContext();
    virtual ~nsThebesDeviceContext();

    static void Shutdown();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER

    NS_IMETHOD Init(nsIWidget *aWidget);
    NS_IMETHOD InitForPrinting(nsIDeviceContextSpec *aDevSpec);
    NS_IMETHOD CreateRenderingContext(nsIView *aView, nsRenderingContext *&aContext);
    NS_IMETHOD CreateRenderingContext(nsIWidget *aWidget, nsRenderingContext *&aContext);
    NS_IMETHOD CreateRenderingContext(nsRenderingContext *&aContext);
    NS_IMETHOD CreateRenderingContextInstance(nsRenderingContext *&aContext);

    NS_IMETHOD GetMetricsFor(const nsFont& aFont, nsIAtom* aLanguage,
                             gfxUserFontSet* aUserFontSet,
                             nsFontMetrics*& aMetrics);
    NS_IMETHOD GetMetricsFor(const nsFont& aFont,
                             gfxUserFontSet* aUserFontSet,
                             nsFontMetrics*& aMetrics);

    NS_IMETHOD FirstExistingFont(const nsFont& aFont, nsString& aFaceName);

    NS_IMETHOD GetLocalFontName(const nsString& aFaceName, nsString& aLocalName,
                                PRBool& aAliased);

    NS_IMETHOD CreateFontCache();
    NS_IMETHOD FontMetricsDeleted(const nsFontMetrics* aFontMetrics);
    NS_IMETHOD FlushFontCache(void);

    NS_IMETHOD PrepareNativeWidget(nsIWidget *aWidget, void **aOut);

    NS_IMETHOD GetSystemFont(nsSystemFontID aID, nsFont *aFont) const;
    NS_IMETHOD ClearCachedSystemFonts();

    NS_IMETHOD CheckFontExistence(const nsString& aFaceName);

    NS_IMETHOD GetDepth(PRUint32& aDepth);

    NS_IMETHOD GetDeviceSurfaceDimensions(nscoord& aWidth, nscoord& aHeight);
    NS_IMETHOD GetRect(nsRect& aRect);
    NS_IMETHOD GetClientRect(nsRect& aRect);

    
    NS_IMETHOD PrepareDocument(PRUnichar *aTitle, 
                               PRUnichar *aPrintToFileName);

    NS_IMETHOD BeginDocument(PRUnichar  *aTitle, 
                             PRUnichar  *aPrintToFileName,
                             PRInt32     aStartPage, 
                             PRInt32     aEndPage);

    NS_IMETHOD EndDocument(void);
    NS_IMETHOD AbortDocument(void);
    NS_IMETHOD BeginPage(void);
    NS_IMETHOD EndPage(void);
    

    virtual PRBool CheckDPIChange();

    virtual PRBool SetPixelScale(float aScale);

    PRBool IsPrinterSurface(void);

#if defined(XP_WIN) || defined(XP_OS2)
    HDC GetPrintHDC();
#endif

protected:
    void GetLocaleLanguage(void);
    nsresult SetDPI();
    void ComputeClientRectUsingScreen(nsRect *outRect);
    void ComputeFullAreaUsingScreen(nsRect *outRect);
    void FindScreen(nsIScreen **outScreen);
    void CalcPrintingSize();
    void UpdateScaledAppUnits();

    PRUint32          mDepth;
    nsFontCache*      mFontCache;
    nsCOMPtr<nsIAtom> mLocaleLanguage; 
    nsCOMPtr<nsIWidget> mWidget;

private:
    nsCOMPtr<nsIScreenManager> mScreenManager;

    nscoord mWidth;
    nscoord mHeight;

    nsRefPtr<gfxASurface> mPrintingSurface;
    float mPrintingScale;
    nsCOMPtr<nsIDeviceContextSpec> mDeviceContextSpec;
};

#endif 

