





































#ifndef nsDeviceContextSpecOS2_h___
#define nsDeviceContextSpecOS2_h___

#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#define INCL_SPLDOSPRINT
#define INCL_DEV
#define INCL_DEVDJP
#define INCL_GRE_DEVICE

#include "nsCOMPtr.h"
#include "nsIDeviceContextSpec.h"
#include "nsIPrintOptions.h"
#include "nsIPrintSettings.h"
#include "nsVoidArray.h"
#include "nsPrintdOS2.h"
#include <os2.h>
#include <pmddim.h>

#include "nsPrintOS2.h"




class nsDeviceContextSpecOS2 : public nsIDeviceContextSpec
{
public:




  nsDeviceContextSpecOS2();

  NS_DECL_ISUPPORTS









  NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);
  
  NS_IMETHOD ClosePrintManager();

  NS_IMETHOD GetDestination ( int &aDestination ); 

  NS_IMETHOD GetPrinterName ( char **aPrinter );

  NS_IMETHOD GetCopies ( int &aCopies );

  NS_IMETHOD GetPath ( char **aPath );    

  NS_IMETHOD GetUserCancelled( PRBool &aCancel );      

  NS_IMETHOD GetPRTQUEUE(PRTQUEUE *&p);

#ifdef MOZ_CAIRO_GFX
  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **nativeSurface);
  NS_IMETHOD BeginDocument(PRUnichar* aTitle, PRUnichar* aPrintToFileName,
                           PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD EndDocument();
  NS_IMETHOD BeginPage();
  NS_IMETHOD EndPage();
#endif





  virtual ~nsDeviceContextSpecOS2();

  static PRINTDLG PrnDlg;
  static nsresult SetPrintSettingsFromDevMode(nsIPrintSettings* aPrintSettings, ULONG printer);

protected:

  OS2PrData mPrData;
  PRTQUEUE *mQueue;
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
};




class nsPrinterEnumeratorOS2 : public nsIPrinterEnumerator
{
public:
  nsPrinterEnumeratorOS2();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR

protected:
};


#endif

