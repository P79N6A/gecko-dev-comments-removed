





































#ifndef nsDeviceContextSpecB_h___
#define nsDeviceContextSpecB_h___

#include "nsCOMPtr.h"
#include "nsIDeviceContextSpec.h"
#include "nsVoidArray.h"
#include "nsIPrintSettings.h" 
#include "nsIPrintOptions.h" 

#include "nsIWidget.h" 
#include "nsPrintdBeOS.h" 
 
class nsDeviceContextSpecBeOS : public nsIDeviceContextSpec
{
public:




  nsDeviceContextSpecBeOS();

  NS_DECL_ISUPPORTS









  NS_IMETHOD Init(nsIWidget *aWidget,
                  nsIPrintSettings* aPS,
                  PRBool aIsPrintPreview);
  
  





  NS_IMETHOD ClosePrintManager();

  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **nativeSurface) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }





protected:
  virtual ~nsDeviceContextSpecBeOS();
 
public:
  int InitializeGlobalPrinters();
  void FreeGlobalPrinters();

protected:
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
