





#ifndef nsPrintSettingsGTK_h_
#define nsPrintSettingsGTK_h_

#include "nsPrintSettingsImpl.h"

extern "C" {
#include <gtk/gtk.h>
#include <gtk/gtkunixprint.h>
}

#define NS_PRINTSETTINGSGTK_IID \
{ 0x758df520, 0xc7c3, 0x11dc, { 0x95, 0xff, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 } }
 





class nsPrintSettingsGTK : public nsPrintSettings
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_PRINTSETTINGSGTK_IID)

  nsPrintSettingsGTK();

  
  
  

  GtkPageSetup* GetGtkPageSetup() { return mPageSetup; };
  void SetGtkPageSetup(GtkPageSetup *aPageSetup);

  GtkPrintSettings* GetGtkPrintSettings() { return mPrintSettings; };
  void SetGtkPrintSettings(GtkPrintSettings *aPrintSettings);

  GtkPrinter* GetGtkPrinter() { return mGTKPrinter; };
  void SetGtkPrinter(GtkPrinter *aPrinter);

  bool GetForcePrintSelectionOnly() { return mPrintSelectionOnly; };
  void SetForcePrintSelectionOnly(bool aPrintSelectionOnly) { mPrintSelectionOnly = aPrintSelectionOnly; };

  
  
  NS_IMETHOD GetPrintRange(int16_t *aPrintRange);
  NS_IMETHOD SetPrintRange(int16_t aPrintRange);

  
  NS_IMETHOD GetStartPageRange(int32_t *aStartPageRange);
  NS_IMETHOD SetStartPageRange(int32_t aStartPageRange);
  NS_IMETHOD GetEndPageRange(int32_t *aEndPageRange);
  NS_IMETHOD SetEndPageRange(int32_t aEndPageRange);

  
  
  NS_IMETHOD GetPrintReversed(bool *aPrintReversed);
  NS_IMETHOD SetPrintReversed(bool aPrintReversed);

  NS_IMETHOD GetPrintInColor(bool *aPrintInColor);
  NS_IMETHOD SetPrintInColor(bool aPrintInColor);

  NS_IMETHOD GetOrientation(int32_t *aOrientation);
  NS_IMETHOD SetOrientation(int32_t aOrientation);

  NS_IMETHOD GetToFileName(char16_t * *aToFileName);
  NS_IMETHOD SetToFileName(const char16_t * aToFileName);

  
  
  NS_IMETHOD GetPrinterName(char16_t * *aPrinter);
  NS_IMETHOD SetPrinterName(const char16_t * aPrinter);

  
  NS_IMETHOD GetNumCopies(int32_t *aNumCopies);
  NS_IMETHOD SetNumCopies(int32_t aNumCopies);

  NS_IMETHOD GetScaling(double *aScaling);
  NS_IMETHOD SetScaling(double aScaling);

  
  NS_IMETHOD GetPaperName(char16_t * *aPaperName);
  NS_IMETHOD SetPaperName(const char16_t * aPaperName);

  NS_IMETHOD SetUnwriteableMarginInTwips(nsIntMargin& aUnwriteableMargin);
  NS_IMETHOD SetUnwriteableMarginTop(double aUnwriteableMarginTop);
  NS_IMETHOD SetUnwriteableMarginLeft(double aUnwriteableMarginLeft);
  NS_IMETHOD SetUnwriteableMarginBottom(double aUnwriteableMarginBottom);
  NS_IMETHOD SetUnwriteableMarginRight(double aUnwriteableMarginRight);

  NS_IMETHOD GetPaperWidth(double *aPaperWidth);
  NS_IMETHOD SetPaperWidth(double aPaperWidth);

  NS_IMETHOD GetPaperHeight(double *aPaperHeight);
  NS_IMETHOD SetPaperHeight(double aPaperHeight);

  NS_IMETHOD SetPaperSizeUnit(int16_t aPaperSizeUnit);

  NS_IMETHOD GetEffectivePageSize(double *aWidth, double *aHeight);

  NS_IMETHOD SetupSilentPrinting();

  NS_IMETHOD GetPageRanges(nsTArray<int32_t> &aPages);

  NS_IMETHOD GetResolution(int32_t *aResolution);
  NS_IMETHOD SetResolution(int32_t aResolution);

  NS_IMETHOD GetDuplex(int32_t *aDuplex);
  NS_IMETHOD SetDuplex(int32_t aDuplex);

protected:
  virtual ~nsPrintSettingsGTK();

  nsPrintSettingsGTK(const nsPrintSettingsGTK& src);
  nsPrintSettingsGTK& operator=(const nsPrintSettingsGTK& rhs);

  virtual nsresult _Clone(nsIPrintSettings **_retval);
  virtual nsresult _Assign(nsIPrintSettings *aPS);

  GtkUnit GetGTKUnit(int16_t aGeckoUnit);
  void SaveNewPageSize();

  



  void InitUnwriteableMargin();

  




  GtkPageSetup* mPageSetup;
  GtkPrintSettings* mPrintSettings;
  GtkPrinter* mGTKPrinter;
  GtkPaperSize* mPaperSize;

  bool mPrintSelectionOnly;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsPrintSettingsGTK, NS_PRINTSETTINGSGTK_IID)


#endif 
