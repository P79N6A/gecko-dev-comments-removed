






































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

    NS_IMETHOD CreateRenderingContext(nsIDrawingSurface *aSurface, nsIRenderingContext *&aContext);
    NS_IMETHOD CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext);
    NS_IMETHOD CreateRenderingContext(nsIRenderingContext *&aContext);
    NS_IMETHOD CreateRenderingContextInstance(nsIRenderingContext *&aContext);

    NS_IMETHOD SupportsNativeWidgets(PRBool& aSupportsWidgets);
    NS_IMETHOD PrepareNativeWidget(nsIWidget *aWidget, void **aOut);

    NS_IMETHOD GetSystemFont(nsSystemFontID aID, nsFont *aFont) const;

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

    nsNativeWidget GetWidget() { return mWidget; }
#ifdef XP_WIN
    HDC GetPrintHDC() {
        if (mPrintingSurface) {
            NS_ASSERTION(mPrintingSurface->GetType() == gfxASurface::SurfaceTypeWin32, "invalid surface type");
            return reinterpret_cast<gfxWindowsSurface*>(mPrintingSurface.get())->GetDC();
        }
        return nsnull;
    }
#elif defined(XP_OS2)
    
    HDC GetPrintDC() {
        if (mPrintingSurface) {
            NS_ASSERTION(mPrintingSurface->GetType() == gfxASurface::SurfaceTypeOS2, "invalid surface type");
            return GpiQueryDevice(reinterpret_cast<gfxOS2Surface*>(mPrintingSurface.get())->GetPS());
        }
        return nsnull;
    }
#endif

protected:
    nsresult SetDPI();
    void ComputeClientRectUsingScreen(nsRect *outRect);
    void ComputeFullAreaUsingScreen(nsRect *outRect);
    void FindScreen(nsIScreen **outScreen);
    void CalcPrintingSize();

    PRUint32 mDepth;

private:
    nsCOMPtr<nsIScreenManager> mScreenManager;

    nscoord mWidth;
    nscoord mHeight;

    nsRefPtrHashtable<nsISupportsHashKey, gfxASurface> mWidgetSurfaceCache;

    nsRefPtr<gfxASurface> mPrintingSurface;
    nsIDeviceContextSpec *mDeviceContextSpec;
};

#endif 

