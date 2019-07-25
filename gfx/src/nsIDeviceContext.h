





































#ifndef nsIDeviceContext_h___
#define nsIDeviceContext_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "gfxTypes.h"
#include "nsStringFwd.h"

class nsIView;
class nsFontMetrics;
class nsIWidget;
class nsIDeviceContextSpec;
class nsIAtom;
class nsRect;
class gfxUserFontSet;
class nsRenderingContext;
struct nsFont;

#define NS_IDEVICE_CONTEXT_IID   \
{ 0x30a9d22f, 0x8e51, 0x40af, \
  { 0xa1, 0xf5, 0x48, 0xe3, 0x00, 0xaa, 0xa9, 0x27 } }

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

class nsIDeviceContext : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDEVICE_CONTEXT_IID)

  




  NS_IMETHOD  Init(nsIWidget* aWidget) = 0;

  




  NS_IMETHOD  InitForPrinting(nsIDeviceContextSpec* aDevSpec) = 0;

  





  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsRenderingContext *&aContext) = 0;

  





  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsRenderingContext *&aContext) = 0;

  




  NS_IMETHOD  CreateRenderingContext(nsRenderingContext *&aContext) = 0;

  




  NS_IMETHOD  CreateRenderingContextInstance(nsRenderingContext *&aContext) = 0;

  







  NS_IMETHOD PrepareNativeWidget(nsIWidget* aWidget, void** aOut) = 0;

  



  static PRInt32 AppUnitsPerCSSPixel() { return 60; }

  


  static gfxFloat AppUnitsToGfxCSSPixels(nscoord aAppUnits)
  { return gfxFloat(aAppUnits) / AppUnitsPerCSSPixel(); }

  



  PRInt32 AppUnitsPerDevPixel() const { return mAppUnitsPerDevPixel; }

  



  nscoord GfxUnitsToAppUnits(gfxFloat aGfxUnits) const
  { return nscoord(NS_round(aGfxUnits * AppUnitsPerDevPixel())); }

  


  gfxFloat AppUnitsToGfxUnits(nscoord aAppUnits) const
  { return gfxFloat(aAppUnits) / AppUnitsPerDevPixel(); }

  



  PRInt32 AppUnitsPerPhysicalInch() const { return mAppUnitsPerPhysicalInch; }

  



  static PRInt32 AppUnitsPerCSSInch() { return 96 * AppUnitsPerCSSPixel(); }

  








  NS_IMETHOD  GetSystemFont(nsSystemFontID aID, nsFont *aFont) const = 0;

  








  NS_IMETHOD  GetMetricsFor(const nsFont& aFont, nsIAtom* aLanguage,
                            gfxUserFontSet* aUserFontSet,
                            nsFontMetrics*& aMetrics) = 0;

  







  NS_IMETHOD  GetMetricsFor(const nsFont& aFont, gfxUserFontSet* aUserFontSet,
                            nsFontMetrics*& aMetrics) = 0;

  




  NS_IMETHOD CheckFontExistence(const nsString& aFaceName) = 0;
  NS_IMETHOD FirstExistingFont(const nsFont& aFont, nsString& aFaceName) = 0;

  NS_IMETHOD GetLocalFontName(const nsString& aFaceName, nsString& aLocalName,
                              PRBool& aAliased) = 0;

  



  NS_IMETHOD FontMetricsDeleted(const nsFontMetrics* aFontMetrics) = 0;

  




  NS_IMETHOD FlushFontCache(void) = 0;

  


  NS_IMETHOD GetDepth(PRUint32& aDepth) = 0;

  






  NS_IMETHOD GetDeviceSurfaceDimensions(nscoord &aWidth, nscoord &aHeight) = 0;

  






  NS_IMETHOD GetRect ( nsRect &aRect ) = 0;

  









  NS_IMETHOD GetClientRect(nsRect &aRect) = 0;

  







  NS_IMETHOD PrepareDocument(PRUnichar * aTitle, 
                             PRUnichar*  aPrintToFileName) = 0;

  
  













  NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                           PRUnichar*  aPrintToFileName,
                           PRInt32     aStartPage, 
                           PRInt32     aEndPage) = 0;

  





  NS_IMETHOD EndDocument(void) = 0;

  




  NS_IMETHOD AbortDocument(void) = 0;

  





  NS_IMETHOD BeginPage(void) = 0;

  





  NS_IMETHOD EndPage(void) = 0;

  






  NS_IMETHOD ClearCachedSystemFonts() = 0;

  




  virtual PRBool CheckDPIChange() = 0;

  




  virtual PRBool SetPixelScale(float aScale) = 0;

  



  float GetPixelScale() const { return mPixelScale; }

  



  PRInt32 UnscaledAppUnitsPerDevPixel() const { return mAppUnitsPerDevNotScaledPixel; }

protected:
  PRInt32 mAppUnitsPerDevPixel;
  PRInt32 mAppUnitsPerPhysicalInch;
  PRInt32 mAppUnitsPerDevNotScaledPixel;
  float  mPixelScale;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDeviceContext, NS_IDEVICE_CONTEXT_IID)

#endif 
