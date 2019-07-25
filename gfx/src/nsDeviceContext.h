






































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

typedef enum {
    eSystemFont_Caption,         
    eSystemFont_Icon,
    eSystemFont_Menu,
    eSystemFont_MessageBox,
    eSystemFont_SmallCaption,
    eSystemFont_StatusBar,

    eSystemFont_Window,          
    eSystemFont_Document,
    eSystemFont_Workspace,
    eSystemFont_Desktop,
    eSystemFont_Info,
    eSystemFont_Dialog,
    eSystemFont_Button,
    eSystemFont_PullDownMenu,
    eSystemFont_List,
    eSystemFont_Field,

    eSystemFont_Tooltips,        
    eSystemFont_Widget
} nsSystemFontID;

class nsDeviceContext
{
public:
    nsDeviceContext();
    ~nsDeviceContext();

    NS_INLINE_DECL_REFCOUNTING(nsDeviceContext)

    




    nsresult Init(nsIWidget *aWidget);

    




    nsresult InitForPrinting(nsIDeviceContextSpec *aDevSpec);

    





    nsresult CreateRenderingContext(nsRenderingContext *&aContext);

    



    static PRInt32 AppUnitsPerCSSPixel() { return 60; }

    




    PRInt32 AppUnitsPerDevPixel() const { return mAppUnitsPerDevPixel; }

    



    nscoord GfxUnitsToAppUnits(gfxFloat aGfxUnits) const
    { return nscoord(NS_round(aGfxUnits * AppUnitsPerDevPixel())); }

    


    gfxFloat AppUnitsToGfxUnits(nscoord aAppUnits) const
    { return gfxFloat(aAppUnits) / AppUnitsPerDevPixel(); }

    



    PRInt32 AppUnitsPerPhysicalInch() const
    { return mAppUnitsPerPhysicalInch; }

    



    static PRInt32 AppUnitsPerCSSInch() { return 96 * AppUnitsPerCSSPixel(); }

    



    PRInt32 UnscaledAppUnitsPerDevPixel() const
    { return mAppUnitsPerDevNotScaledPixel; }

    








    nsresult GetSystemFont(nsSystemFontID aID, nsFont *aFont) const;

    


    static void ClearCachedSystemFonts();

    








    nsresult GetMetricsFor(const nsFont& aFont, nsIAtom* aLanguage,
                           gfxUserFontSet* aUserFontSet,
                           nsFontMetrics*& aMetrics);

    



    nsresult FontMetricsDeleted(const nsFontMetrics* aFontMetrics);

    




    nsresult FlushFontCache();

    


    nsresult GetDepth(PRUint32& aDepth);

    






    nsresult GetDeviceSurfaceDimensions(nscoord& aWidth, nscoord& aHeight);

    








    nsresult GetRect(nsRect& aRect);

    










    nsresult GetClientRect(nsRect& aRect);

    













    nsresult BeginDocument(PRUnichar  *aTitle,
                           PRUnichar  *aPrintToFileName,
                           PRInt32     aStartPage,
                           PRInt32     aEndPage);

    





    nsresult EndDocument();

    




    nsresult AbortDocument();

    





    nsresult BeginPage();

    





    nsresult EndPage();

    





    PRBool CheckDPIChange();

    




    PRBool SetPixelScale(float aScale);

    


    PRBool IsPrinterSurface();

protected:
    void SetDPI();
    void ComputeClientRectUsingScreen(nsRect *outRect);
    void ComputeFullAreaUsingScreen(nsRect *outRect);
    void FindScreen(nsIScreen **outScreen);
    void CalcPrintingSize();
    void UpdateScaledAppUnits();

    nscoord  mWidth;
    nscoord  mHeight;
    PRUint32 mDepth;
    PRInt32  mAppUnitsPerDevPixel;
    PRInt32  mAppUnitsPerDevNotScaledPixel;
    PRInt32  mAppUnitsPerPhysicalInch;
    float    mPixelScale;
    float    mPrintingScale;

    nsFontCache*                   mFontCache;
    nsCOMPtr<nsIWidget>            mWidget;
    nsCOMPtr<nsIScreenManager>     mScreenManager;
    nsCOMPtr<nsIDeviceContextSpec> mDeviceContextSpec;
    nsRefPtr<gfxASurface>          mPrintingSurface;
};

#endif 
