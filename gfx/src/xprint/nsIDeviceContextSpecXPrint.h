






































#ifndef nsIDeviceContextSpecXP_h___
#define nsIDeviceContextSpecXP_h___

#include "nsISupports.h"


#define NS_IDEVICE_CONTEXT_SPEC_XP_IID { 0x12ab7845, 0xa341, 0x41ba, { 0x60, 0x25, 0xe0, 0xb1, 0x1e, 0x0e } }

class nsIDeviceContextSpecXp : public nsISupports
{

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDEVICE_CONTEXT_SPEC_XP_IID)

  





   NS_IMETHOD GetToPrinter( PRBool &aToPrinter ) = 0; 

  





   NS_IMETHOD GetFirstPageFirst (  PRBool &aFpf ) = 0;     

  





   NS_IMETHOD GetGrayscale(  PRBool &aGrayscale ) = 0;   

  





   NS_IMETHOD GetTopMargin (  float &aValue ) = 0; 

  





   NS_IMETHOD GetBottomMargin (  float &aValue ) = 0; 

  





   NS_IMETHOD GetLeftMargin (  float &aValue ) = 0; 

  





   NS_IMETHOD GetRightMargin (  float &aValue ) = 0; 

  





   NS_IMETHOD GetPrinterName ( const char **aPrinter ) = 0;   

  





   NS_IMETHOD GetLandscape ( PRBool &aLandscape ) = 0;   

  





   NS_IMETHOD GetPath ( const char **aPath ) = 0;    

  





   NS_IMETHOD GetUserCancelled(  PRBool &aCancel ) = 0;
   
  





   NS_IMETHOD GetPaperName ( const char **aPaperName ) = 0;   

  






   NS_IMETHOD GetPlexName ( const char **aPlexName ) = 0;   

  







   NS_IMETHOD GetResolutionName ( const char **aResolutionName ) = 0;   
   
  







   NS_IMETHOD GetColorspace ( const char **aColorspace ) = 0;   

  





   NS_IMETHOD GetDownloadFonts( PRBool &aDownloadFonts ) = 0;   

  




   
   NS_IMETHOD GetCopies ( int &aCopies ) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDeviceContextSpecXp,
                              NS_IDEVICE_CONTEXT_SPEC_XP_IID)

#endif 

