




#ifndef _NS_DEVICECONTEXT_H_
#define _NS_DEVICECONTEXT_H_

#include "nsCOMPtr.h"
#include "nsIDeviceContextSpec.h"
#include "nsIScreenManager.h"
#include "nsIWidget.h"
#include "nsCoord.h"
#include "gfxContext.h"

class nsIAtom;
class nsFontCache;
class gfxUserFontSet;

class nsDeviceContext
{
public:
    nsDeviceContext();
    ~nsDeviceContext();

    NS_INLINE_DECL_REFCOUNTING(nsDeviceContext)

    




    nsresult Init(nsIWidget *aWidget);

    




    nsresult InitForPrinting(nsIDeviceContextSpec *aDevSpec);

    





    nsresult CreateRenderingContext(nsRenderingContext *&aContext);

    



    static int32_t AppUnitsPerCSSPixel() { return 60; }

    




    int32_t AppUnitsPerDevPixel() const { return mAppUnitsPerDevPixel; }

    



    nscoord GfxUnitsToAppUnits(gfxFloat aGfxUnits) const
    { return nscoord(NS_round(aGfxUnits * AppUnitsPerDevPixel())); }

    


    gfxFloat AppUnitsToGfxUnits(nscoord aAppUnits) const
    { return gfxFloat(aAppUnits) / AppUnitsPerDevPixel(); }

    



    int32_t AppUnitsPerPhysicalInch() const
    { return mAppUnitsPerPhysicalInch; }

    



    static int32_t AppUnitsPerCSSInch() { return 96 * AppUnitsPerCSSPixel(); }

    



    int32_t UnscaledAppUnitsPerDevPixel() const
    { return mAppUnitsPerDevNotScaledPixel; }

    








    nsresult GetMetricsFor(const nsFont& aFont, nsIAtom* aLanguage,
                           gfxUserFontSet* aUserFontSet,
                           nsFontMetrics*& aMetrics);

    



    nsresult FontMetricsDeleted(const nsFontMetrics* aFontMetrics);

    




    nsresult FlushFontCache();

    


    nsresult GetDepth(uint32_t& aDepth);

    






    nsresult GetDeviceSurfaceDimensions(nscoord& aWidth, nscoord& aHeight);

    








    nsresult GetRect(nsRect& aRect);

    










    nsresult GetClientRect(nsRect& aRect);

    













    nsresult BeginDocument(PRUnichar  *aTitle,
                           PRUnichar  *aPrintToFileName,
                           int32_t     aStartPage,
                           int32_t     aEndPage);

    





    nsresult EndDocument();

    




    nsresult AbortDocument();

    





    nsresult BeginPage();

    





    nsresult EndPage();

    





    bool CheckDPIChange();

    




    bool SetPixelScale(float aScale);

    


    float GetPixelScale() const { return mPixelScale; }

    


    bool IsPrinterSurface();

protected:
    void SetDPI();
    void ComputeClientRectUsingScreen(nsRect *outRect);
    void ComputeFullAreaUsingScreen(nsRect *outRect);
    void FindScreen(nsIScreen **outScreen);
    void CalcPrintingSize();
    void UpdateScaledAppUnits();

    nscoord  mWidth;
    nscoord  mHeight;
    uint32_t mDepth;
    int32_t  mAppUnitsPerDevPixel;
    int32_t  mAppUnitsPerDevNotScaledPixel;
    int32_t  mAppUnitsPerPhysicalInch;
    float    mPixelScale;
    float    mPrintingScale;

    nsFontCache*                   mFontCache;
    nsCOMPtr<nsIWidget>            mWidget;
    nsCOMPtr<nsIScreenManager>     mScreenManager;
    nsCOMPtr<nsIDeviceContextSpec> mDeviceContextSpec;
    nsRefPtr<gfxASurface>          mPrintingSurface;
#ifdef XP_MACOSX
    nsRefPtr<gfxASurface>          mCachedPrintingSurface;
#endif
};

#endif 
