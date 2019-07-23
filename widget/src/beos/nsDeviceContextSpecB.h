





































#ifndef nsDeviceContextSpecB_h___
#define nsDeviceContextSpecB_h___

#include "nsCOMPtr.h"
#include "nsIDeviceContextSpec.h"
#include "nsVoidArray.h"
#ifdef USE_POSTSCRIPT
#include "nsIDeviceContextSpecPS.h"
#endif 
#include "nsIPrintSettings.h" 
#include "nsIPrintOptions.h" 

#include "nsIWidget.h" 
#include "nsPrintdBeOS.h" 
 
class nsDeviceContextSpecBeOS : public nsIDeviceContextSpec
#ifdef USE_POSTSCRIPT
                              , public nsIDeviceContextSpecPS 
#endif
{
public:




  nsDeviceContextSpecBeOS();

  NS_DECL_ISUPPORTS









  NS_IMETHOD Init(nsIWidget *aWidget,
                  nsIPrintSettings* aPS,
                  PRBool aIsPrintPreview);
  
  





  NS_IMETHOD ClosePrintManager();
  
  NS_IMETHOD GetPageSizeInTwips(PRInt32 *aWidth, PRInt32 *aHeight);

  NS_IMETHOD GetToPrinter( PRBool &aToPrinter ); 

  NS_IMETHOD GetPrinterName ( const char **aPrinter );

  NS_IMETHOD GetCopies ( int &aCopies ); 

  NS_IMETHOD GetFirstPageFirst ( PRBool &aFpf );     
 
  NS_IMETHOD GetGrayscale( PRBool &aGrayscale );   
 
  NS_IMETHOD GetSize ( int &aSize ); 
 
  NS_IMETHOD GetTopMargin ( float &value ); 
 
  NS_IMETHOD GetBottomMargin ( float &value ); 
 
  NS_IMETHOD GetLeftMargin ( float &value ); 
 
  NS_IMETHOD GetRightMargin ( float &value ); 
 
  NS_IMETHOD GetCommand ( const char **aCommand );   
 
  NS_IMETHOD GetPath ( const char **aPath );    
 
  NS_IMETHOD GetPageDimensions (float &aWidth, float &aHeight ); 
 
  NS_IMETHOD GetLandscape (PRBool &aLandscape);

  NS_IMETHOD GetUserCancelled( PRBool &aCancel );      

#ifdef MOZ_CAIRO_GFX
   NS_IMETHOD GetSurfaceForPrinter(gfxASurface **nativeSurface) {
       return NS_ERROR_NOT_IMPLEMENTED;
   }
#endif





protected:
  virtual ~nsDeviceContextSpecBeOS();
 
public:
  int InitializeGlobalPrinters();
  void FreeGlobalPrinters();

protected:
  BeOSPrData mPrData;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;	
};




class nsPrinterEnumeratorBeOS : public nsIPrinterEnumerator
{
public:
  nsPrinterEnumeratorBeOS();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR

protected:
};

#endif
