





































#ifndef nsIDeviceContext_h___
#define nsIDeviceContext_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "gfxTypes.h"
#include "nsStringFwd.h"

class nsIView;
class nsIFontMetrics;
class nsIWidget;
class nsIDeviceContextSpec;
class nsIAtom;
class gfxUserFontSet;
class nsRenderingContext;

struct nsFont;


typedef void * nsNativeDeviceContext;


#define NS_ERROR_GFX_PRINTER_BASE (1) /* adjustable :-) */

#define NS_ERROR_GFX_PRINTER_CMD_NOT_FOUND          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+1)
  
#define NS_ERROR_GFX_PRINTER_CMD_FAILURE            \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+2)

#define NS_ERROR_GFX_PRINTER_NO_PRINTER_AVAILABLE  \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+3)

#define NS_ERROR_GFX_PRINTER_NAME_NOT_FOUND         \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+4)

#define NS_ERROR_GFX_PRINTER_ACCESS_DENIED          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+5)

#define NS_ERROR_GFX_PRINTER_INVALID_ATTRIBUTE      \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+6)

#define NS_ERROR_GFX_PRINTER_PRINTER_NOT_READY     \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+8)

#define NS_ERROR_GFX_PRINTER_OUT_OF_PAPER           \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+9)

#define NS_ERROR_GFX_PRINTER_PRINTER_IO_ERROR       \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+10)

#define NS_ERROR_GFX_PRINTER_COULD_NOT_OPEN_FILE    \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+11)

#define NS_ERROR_GFX_PRINTER_FILE_IO_ERROR          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+12)

#define NS_ERROR_GFX_PRINTER_PRINTPREVIEW          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+13)

#define NS_ERROR_GFX_PRINTER_STARTDOC          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+14)

#define NS_ERROR_GFX_PRINTER_ENDDOC          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+15)

#define NS_ERROR_GFX_PRINTER_STARTPAGE          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+16)

#define NS_ERROR_GFX_PRINTER_ENDPAGE          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+17)

#define NS_ERROR_GFX_PRINTER_PRINT_WHILE_PREVIEW          \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+18)

#define NS_ERROR_GFX_PRINTER_PAPER_SIZE_NOT_SUPPORTED \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+19)

#define NS_ERROR_GFX_PRINTER_ORIENTATION_NOT_SUPPORTED \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+20)

#define NS_ERROR_GFX_PRINTER_COLORSPACE_NOT_SUPPORTED \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+21)

#define NS_ERROR_GFX_PRINTER_TOO_MANY_COPIES \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+22)

#define NS_ERROR_GFX_PRINTER_DRIVER_CONFIGURATION_ERROR \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+23)

#define NS_ERROR_GFX_PRINTER_DOC_IS_BUSY_PP \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+24)

#define NS_ERROR_GFX_PRINTER_DOC_WAS_DESTORYED \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+25)

#define NS_ERROR_GFX_PRINTER_NO_XUL \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+26)

#define NS_ERROR_GFX_NO_PRINTDIALOG_IN_TOOLKIT \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+27)

#define NS_ERROR_GFX_NO_PRINTROMPTSERVICE \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+28)

#define NS_ERROR_GFX_PRINTER_PLEX_NOT_SUPPORTED \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+29)

#define NS_ERROR_GFX_PRINTER_DOC_IS_BUSY \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+30)

#define NS_ERROR_GFX_PRINTING_NOT_IMPLEMENTED \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+31)

#define NS_ERROR_GFX_COULD_NOT_LOAD_PRINT_MODULE \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+32)   

#define NS_ERROR_GFX_PRINTER_RESOLUTION_NOT_SUPPORTED \
  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GFX,NS_ERROR_GFX_PRINTER_BASE+33)
      



#ifdef NS_PRINT_PREVIEW
const PRUint8 kUseAltDCFor_NONE            = 0x00; 
const PRUint8 kUseAltDCFor_FONTMETRICS     = 0x01; 
const PRUint8 kUseAltDCFor_CREATERC_REFLOW = 0x02; 
const PRUint8 kUseAltDCFor_CREATERC_PAINT  = 0x04; 
const PRUint8 kUseAltDCFor_SURFACE_DIM     = 0x08; 
#endif

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

  





  NS_IMETHOD  CreateRenderingContext(nsIView *aView, nsIRenderingContext *&aContext) = 0;

  





  NS_IMETHOD  CreateRenderingContext(nsIWidget *aWidget, nsIRenderingContext *&aContext) = 0;

  




  NS_IMETHOD  CreateRenderingContext(nsIRenderingContext *&aContext) = 0;

  




  NS_IMETHOD  CreateRenderingContextInstance(nsIRenderingContext *&aContext) = 0;

  







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
                            nsIFontMetrics*& aMetrics) = 0;

  







  NS_IMETHOD  GetMetricsFor(const nsFont& aFont, gfxUserFontSet* aUserFontSet,
                            nsIFontMetrics*& aMetrics) = 0;

  




  NS_IMETHOD CheckFontExistence(const nsString& aFaceName) = 0;
  NS_IMETHOD FirstExistingFont(const nsFont& aFont, nsString& aFaceName) = 0;

  NS_IMETHOD GetLocalFontName(const nsString& aFaceName, nsString& aLocalName,
                              PRBool& aAliased) = 0;

  



  NS_IMETHOD FontMetricsDeleted(const nsIFontMetrics* aFontMetrics) = 0;

  




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
