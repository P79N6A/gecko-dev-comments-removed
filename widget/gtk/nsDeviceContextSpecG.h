




#ifndef nsDeviceContextSpecGTK_h___
#define nsDeviceContextSpecGTK_h___

#include "nsIDeviceContextSpec.h"
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h" 
#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/Attributes.h"

#include "nsCRT.h" 

#include <gtk/gtk.h>
#include <gtk/gtkunixprint.h>

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

  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface) MOZ_OVERRIDE;

  NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS,
                  bool aIsPrintPreview) MOZ_OVERRIDE;
  NS_IMETHOD BeginDocument(const nsAString& aTitle, char16_t * aPrintToFileName,
                           int32_t aStartPage, int32_t aEndPage) MOZ_OVERRIDE;
  NS_IMETHOD EndDocument() MOZ_OVERRIDE;
  NS_IMETHOD BeginPage() MOZ_OVERRIDE { return NS_OK; }
  NS_IMETHOD EndPage() MOZ_OVERRIDE { return NS_OK; }

  static nsresult GetPrintMethod(const char *aPrinter, PrintMethod &aMethod);
  
protected:
  virtual ~nsDeviceContextSpecGTK();
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  bool mToPrinter : 1;      
  bool mIsPPreview : 1;     
  char   mPath[PATH_MAX];     
  char   mPrinter[256];       
  GtkPrintJob*      mPrintJob;
  GtkPrinter*       mGtkPrinter;
  GtkPrintSettings* mGtkPrintSettings;
  GtkPageSetup*     mGtkPageSetup;

  nsCString         mSpoolName;
  nsCOMPtr<nsIFile> mSpoolFile;

};




class nsPrinterEnumeratorGTK MOZ_FINAL : public nsIPrinterEnumerator
{
  ~nsPrinterEnumeratorGTK() {}
public:
  nsPrinterEnumeratorGTK();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR
};

#endif
