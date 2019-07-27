




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

class nsPrintSettingsGTK;

class nsDeviceContextSpecGTK : public nsIDeviceContextSpec
{
public:
  nsDeviceContextSpecGTK();

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface) override;

  NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS,
                  bool aIsPrintPreview) override;
  NS_IMETHOD BeginDocument(const nsAString& aTitle, char16_t * aPrintToFileName,
                           int32_t aStartPage, int32_t aEndPage) override;
  NS_IMETHOD EndDocument() override;
  NS_IMETHOD BeginPage() override { return NS_OK; }
  NS_IMETHOD EndPage() override { return NS_OK; }

protected:
  virtual ~nsDeviceContextSpecGTK();
  nsCOMPtr<nsPrintSettingsGTK> mPrintSettings;
  bool mToPrinter : 1;      
  bool mIsPPreview : 1;     
  char   mPath[PATH_MAX];     
  char   mPrinter[256];       
  GtkPrintSettings* mGtkPrintSettings;
  GtkPageSetup*     mGtkPageSetup;

  nsCString         mSpoolName;
  nsCOMPtr<nsIFile> mSpoolFile;
  nsCString         mTitle;

private:
  void EnumeratePrinters();
  static gboolean PrinterEnumerator(GtkPrinter *aPrinter, gpointer aData);
  static void StartPrintJob(nsDeviceContextSpecGTK *spec,
                            GtkPrinter *printer);
};




class nsPrinterEnumeratorGTK final : public nsIPrinterEnumerator
{
  ~nsPrinterEnumeratorGTK() {}
public:
  nsPrinterEnumeratorGTK();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR
};

#endif
