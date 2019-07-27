




#ifndef _NS_DEVICECONTEXT_H_
#define _NS_DEVICECONTEXT_H_

#include <stdint.h>                     
#include <sys/types.h>                  
#include "gfxTypes.h"                   
#include "gfxFont.h"                    
#include "mozilla/Assertions.h"         
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsCoord.h"                    
#include "nsError.h"                    
#include "nsISupports.h"                
#include "nsMathUtils.h"                
#include "nscore.h"                     
#include "mozilla/AppUnits.h"           

class gfxASurface;
class gfxContext;
class gfxTextPerfMetrics;
class gfxUserFontSet;
struct nsFont;
class nsFontCache;
class nsFontMetrics;
class nsIAtom;
class nsIDeviceContextSpec;
class nsIScreen;
class nsIScreenManager;
class nsIWidget;
struct nsRect;

class nsDeviceContext final
{
public:
    nsDeviceContext();

    NS_INLINE_DECL_REFCOUNTING(nsDeviceContext)

    




    nsresult Init(nsIWidget *aWidget);

    




    nsresult InitForPrinting(nsIDeviceContextSpec *aDevSpec);

    





    already_AddRefed<gfxContext> CreateRenderingContext();

    



    static int32_t AppUnitsPerCSSPixel() { return mozilla::AppUnitsPerCSSPixel(); }

    




    int32_t AppUnitsPerDevPixel() const { return mAppUnitsPerDevPixel; }

    



    nscoord GfxUnitsToAppUnits(gfxFloat aGfxUnits) const
    { return nscoord(NS_round(aGfxUnits * AppUnitsPerDevPixel())); }

    


    gfxFloat AppUnitsToGfxUnits(nscoord aAppUnits) const
    { return gfxFloat(aAppUnits) / AppUnitsPerDevPixel(); }

    



    int32_t AppUnitsPerPhysicalInch() const
    { return mAppUnitsPerPhysicalInch; }

    



    static int32_t AppUnitsPerCSSInch() { return mozilla::AppUnitsPerCSSInch(); }

    



    int32_t AppUnitsPerDevPixelAtUnitFullZoom() const
    { return mAppUnitsPerDevPixelAtUnitFullZoom; }

    








    nsresult GetMetricsFor(const nsFont& aFont,
                           nsIAtom* aLanguage, bool aExplicitLanguage,
                           gfxFont::Orientation aOrientation,
                           gfxUserFontSet* aUserFontSet,
                           gfxTextPerfMetrics* aTextPerf,
                           nsFontMetrics*& aMetrics);

    



    nsresult FontMetricsDeleted(const nsFontMetrics* aFontMetrics);

    




    nsresult FlushFontCache();

    


    nsresult GetDepth(uint32_t& aDepth);

    






    nsresult GetDeviceSurfaceDimensions(nscoord& aWidth, nscoord& aHeight);

    








    nsresult GetRect(nsRect& aRect);

    










    nsresult GetClientRect(nsRect& aRect);

    













    nsresult BeginDocument(const nsAString& aTitle,
                           char16_t*       aPrintToFileName,
                           int32_t          aStartPage,
                           int32_t          aEndPage);

    





    nsresult EndDocument();

    




    nsresult AbortDocument();

    





    nsresult BeginPage();

    





    nsresult EndPage();

    





    bool CheckDPIChange();

    




    bool SetFullZoom(float aScale);

    


    float GetFullZoom() const { return mFullZoom; }

    


    bool IsPrinterSurface();

private:
    
    ~nsDeviceContext();

    void SetDPI();
    void ComputeClientRectUsingScreen(nsRect *outRect);
    void ComputeFullAreaUsingScreen(nsRect *outRect);
    void FindScreen(nsIScreen **outScreen);

    
    bool CalcPrintingSize();
    void UpdateAppUnitsForFullZoom();

    nscoord  mWidth;
    nscoord  mHeight;
    uint32_t mDepth;
    int32_t  mAppUnitsPerDevPixel;
    int32_t  mAppUnitsPerDevPixelAtUnitFullZoom;
    int32_t  mAppUnitsPerPhysicalInch;
    float    mFullZoom;
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
