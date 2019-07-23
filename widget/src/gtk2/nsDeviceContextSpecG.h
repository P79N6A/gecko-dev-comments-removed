





































#ifndef nsDeviceContextSpecGTK_h___
#define nsDeviceContextSpecGTK_h___

#include "nsIDeviceContextSpec.h"
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h" 
#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsCRT.h" 

#include <gtk/gtk.h>
#include <gtk/gtkprinter.h>
#include <gtk/gtkprintjob.h>

#define NS_PORTRAIT  0
#define NS_LANDSCAPE 1

typedef enum
{
  pmInvalid = 0,
  pmPostScript
} PrintMethod;

class nsDeviceContextSpecGTK : public nsIDeviceContextSpec
{
public:
  nsDeviceContextSpecGTK();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);

  NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);
  NS_IMETHOD BeginDocument(PRUnichar * aTitle, PRUnichar * aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD EndDocument();
  NS_IMETHOD BeginPage() { return NS_OK; }
  NS_IMETHOD EndPage() { return NS_OK; }

  NS_IMETHOD GetPath (const char **aPath);    
  NS_IMETHOD GetPrintMethod(PrintMethod &aMethod);
  static nsresult GetPrintMethod(const char *aPrinter, PrintMethod &aMethod);
  virtual ~nsDeviceContextSpecGTK();
  
protected:
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  PRPackedBool mToPrinter : 1;      
  PRPackedBool mIsPPreview : 1;     
  char   mPath[PATH_MAX];     
  char   mPrinter[256];       
  GtkPrintJob*      mPrintJob;
  GtkPrinter*       mGtkPrinter;
  GtkPrintSettings* mGtkPrintSettings;
  GtkPageSetup*     mGtkPageSetup;

  nsCString              mSpoolName;
  nsCOMPtr<nsILocalFile> mSpoolFile;

};




class nsPrinterEnumeratorGTK : public nsIPrinterEnumerator
{
public:
  nsPrinterEnumeratorGTK();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR
};

#endif
