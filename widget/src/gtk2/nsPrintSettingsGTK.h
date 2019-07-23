





































#ifndef nsPrintSettingsGTK_h_
#define nsPrintSettingsGTK_h_

#include "nsPrintSettingsImpl.h"

extern "C" {
#include <gtk/gtk.h>
#include <gtk/gtkprinter.h>
#include <gtk/gtkprintjob.h>
}

#define NS_PRINTSETTINGSGTK_IID \
{ 0x758df520, 0xc7c3, 0x11dc, { 0x95, 0xff, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }
 





class nsPrintSettingsGTK : public nsPrintSettings
{
public:
  using nsPrintSettings::operator=;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PRINTSETTINGSGTK_IID)

  nsPrintSettingsGTK();
  virtual ~nsPrintSettingsGTK();

  
  
  

  GtkPageSetup* GetGtkPageSetup() { return mPageSetup; };
  void SetGtkPageSetup(GtkPageSetup *aPageSetup);

  GtkPrintSettings* GetGtkPrintSettings() { return mPrintSettings; };
  void SetGtkPrintSettings(GtkPrintSettings *aPrintSettings);

  GtkPrinter* GetGtkPrinter() { return mGTKPrinter; };
  void SetGtkPrinter(GtkPrinter *aPrinter);

  PRBool GetForcePrintSelectionOnly() { return mPrintSelectionOnly; };
  void SetForcePrintSelectionOnly(PRBool aPrintSelectionOnly) { mPrintSelectionOnly = aPrintSelectionOnly; };

  
  
  NS_IMETHOD GetPrintRange(PRInt16 *aPrintRange);
  NS_IMETHOD SetPrintRange(PRInt16 aPrintRange);

  
  NS_IMETHOD GetStartPageRange(PRInt32 *aStartPageRange);
  NS_IMETHOD SetStartPageRange(PRInt32 aStartPageRange);
  NS_IMETHOD GetEndPageRange(PRInt32 *aEndPageRange);
  NS_IMETHOD SetEndPageRange(PRInt32 aEndPageRange);

  
  
  NS_IMETHOD GetPrintReversed(PRBool *aPrintReversed);
  NS_IMETHOD SetPrintReversed(PRBool aPrintReversed);

  NS_IMETHOD GetPrintInColor(PRBool *aPrintInColor);
  NS_IMETHOD SetPrintInColor(PRBool aPrintInColor);

  NS_IMETHOD GetOrientation(PRInt32 *aOrientation);
  NS_IMETHOD SetOrientation(PRInt32 aOrientation);

  NS_IMETHOD GetToFileName(PRUnichar * *aToFileName);
  NS_IMETHOD SetToFileName(const PRUnichar * aToFileName);

  
  
  NS_IMETHOD GetPrinterName(PRUnichar * *aPrinter);
  NS_IMETHOD SetPrinterName(const PRUnichar * aPrinter);

  
  NS_IMETHOD GetNumCopies(PRInt32 *aNumCopies);
  NS_IMETHOD SetNumCopies(PRInt32 aNumCopies);

  NS_IMETHOD GetScaling(double *aScaling);
  NS_IMETHOD SetScaling(double aScaling);

  
  NS_IMETHOD GetPaperName(PRUnichar * *aPaperName);
  NS_IMETHOD SetPaperName(const PRUnichar * aPaperName);

  NS_IMETHOD SetUnwriteableMarginInTwips(nsIntMargin& aUnwriteableMargin);
  NS_IMETHOD SetUnwriteableMarginTop(double aUnwriteableMarginTop);
  NS_IMETHOD SetUnwriteableMarginLeft(double aUnwriteableMarginLeft);
  NS_IMETHOD SetUnwriteableMarginBottom(double aUnwriteableMarginBottom);
  NS_IMETHOD SetUnwriteableMarginRight(double aUnwriteableMarginRight);

  NS_IMETHOD GetPaperWidth(double *aPaperWidth);
  NS_IMETHOD SetPaperWidth(double aPaperWidth);

  NS_IMETHOD GetPaperHeight(double *aPaperHeight);
  NS_IMETHOD SetPaperHeight(double aPaperHeight);

  NS_IMETHOD SetPaperSizeUnit(PRInt16 aPaperSizeUnit);

  NS_IMETHOD GetEffectivePageSize(double *aWidth, double *aHeight);

  NS_IMETHOD SetupSilentPrinting();

protected:
  nsPrintSettingsGTK(const nsPrintSettingsGTK& src);
  nsPrintSettingsGTK& operator=(const nsPrintSettingsGTK& rhs);

  virtual nsresult _Clone(nsIPrintSettings **_retval);
  virtual nsresult _Assign(nsIPrintSettings *aPS);

  GtkUnit GetGTKUnit(PRInt16 aGeckoUnit);
  void SaveNewPageSize();

  



  void InitUnwriteableMargin();

  




  GtkPageSetup* mPageSetup;
  GtkPrintSettings* mPrintSettings;
  GtkPrinter* mGTKPrinter;
  GtkPaperSize* mPaperSize;

  PRBool mPrintSelectionOnly;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPrintSettingsGTK, NS_PRINTSETTINGSGTK_IID)


#endif 
