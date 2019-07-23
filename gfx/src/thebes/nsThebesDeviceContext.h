






































#ifndef _NS_THEBESDEVICECONTEXT_H_
#define _NS_THEBESDEVICECONTEXT_H_

#include "nsIScreenManager.h"

#include "nsDeviceContext.h"

#include "nsRefPtrHashtable.h"
#include "nsHashKeys.h"

#include "prlog.h"

#include "gfxContext.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gThebesGFXLog;
#endif

#ifdef XP_WIN
#include "gfxWindowsSurface.h"
#elif defined(XP_OS2)
#include "gfxOS2Surface.h"
#endif

class nsThebesDeviceContext : public DeviceContextImpl
{
public:
    nsThebesDeviceContext();
    virtual ~nsThebesDeviceContext();

    static void Shutdown();

    NS_DECL_ISUPPORTS_INHERITED

    NS_IMETHOD Init(nsNativeWidget aWidget);
    NS_IMETHOD InitForPrinting(nsIDeviceContextSpec *aDevSpec);
    NS_IMETHOD CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext);

    NS_IMETHOD CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext);
    NS_IMETHOD CreateRenderingContext(nsIRenderingContext *&aContext);
    NS_IMETHOD CreateRenderingContextInstance(nsIRenderingContext *&aContext);

    NS_IMETHOD SupportsNativeWidgets(PRBool& aSupportsWidgets);
    NS_IMETHOD PrepareNativeWidget(nsIWidget *aWidget, void **aOut);

    NS_IMETHOD GetSystemFont(nsSystemFontID aID, nsFont *aFont) const;
    NS_IMETHOD ClearCachedSystemFonts();

    NS_IMETHOD CheckFontExistence(const nsString& aFaceName);

    NS_IMETHOD GetDepth(PRUint32& aDepth);

    NS_IMETHOD GetPaletteInfo(nsPaletteInfo& aPaletteInfo);

    NS_IMETHOD ConvertPixel(nscolor aColor, PRUint32& aPixel);

    NS_IMETHOD GetDeviceSurfaceDimensions(PRInt32& aWidth, PRInt32& aHeight);
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
    

    static void DebugShowCairoSurface (const char *aName, cairo_surface_t *aSurface);

    virtual PRBool CheckDPIChange();

    virtual PRBool SetPixelScale(float aScale);

    PRBool IsPrinterSurface(void);

    nsNativeWidget GetWidget() { return mWidget; }
#if defined(XP_WIN) || defined(XP_OS2)
    HDC GetPrintHDC();
#endif

protected:
    nsresult SetDPI();
    void ComputeClientRectUsingScreen(nsRect *outRect);
    void ComputeFullAreaUsingScreen(nsRect *outRect);
    void FindScreen(nsIScreen **outScreen);
    void CalcPrintingSize();
    void UpdateScaledAppUnits();

    PRUint32 mDepth;

private:
    nsCOMPtr<nsIScreenManager> mScreenManager;

    nscoord mWidth;
    nscoord mHeight;

    nsRefPtr<gfxASurface> mPrintingSurface;
    float mPrintingScale;
    nsCOMPtr<nsIDeviceContextSpec> mDeviceContextSpec;
};

#endif 

