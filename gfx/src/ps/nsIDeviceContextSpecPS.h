





































#ifndef nsIDeviceContextSpecPS_h___
#define nsIDeviceContextSpecPS_h___

#include "nsISupports.h"


#define NS_POSTSCRIPT_DRIVER_NAME "PostScript/"

#define NS_POSTSCRIPT_DRIVER_NAME_LEN (11) 

#define NS_IDEVICE_CONTEXT_SPEC_PS_IID { 0xa4ef8910, 0xdd65, 0x11d2, { 0xa8, 0x32, 0x0, 0x10, 0x5a, 0x18, 0x34, 0x19 } }

class nsIDeviceContextSpecPS : public nsISupports
{

public:
   NS_DECLARE_STATIC_IID_ACCESSOR(NS_IDEVICE_CONTEXT_SPEC_PS_IID)

  





   NS_IMETHOD GetToPrinter( PRBool &aToPrinter ) = 0; 

  





   NS_IMETHOD GetIsPrintPreview( PRBool &aIsPPreview ) = 0; 

  





   NS_IMETHOD GetFirstPageFirst (  PRBool &aFpf ) = 0;     

  





   NS_IMETHOD GetGrayscale(  PRBool &aGrayscale ) = 0;   

  





   NS_IMETHOD GetTopMargin (  float &aValue ) = 0; 

  





   NS_IMETHOD GetBottomMargin (  float &aValue ) = 0; 

  





   NS_IMETHOD GetLeftMargin (  float &aValue ) = 0; 

  





   NS_IMETHOD GetRightMargin (  float &aValue ) = 0; 

  





   NS_IMETHOD GetCommand ( const char **aCommand ) = 0;   

  





   NS_IMETHOD GetPrinterName ( const char **aPrinter ) = 0;   

  





   NS_IMETHOD GetLandscape ( PRBool &aLandscape ) = 0;   

  





   NS_IMETHOD GetPath ( const char **aPath ) = 0;    

  





   NS_IMETHOD GetUserCancelled(  PRBool &aCancel ) = 0;      

  





   NS_IMETHOD GetPaperName ( const char **aPaperName ) = 0;

   


   NS_IMETHOD GetPageSizeInTwips(PRInt32 *aWidth, PRInt32 *aHeight) = 0;
 
   





   NS_IMETHOD GetCopies ( int &aCopies ) = 0;

};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIDeviceContextSpecPS,
                              NS_IDEVICE_CONTEXT_SPEC_PS_IID)

#endif 

