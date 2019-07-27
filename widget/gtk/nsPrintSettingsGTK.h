





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

  
  
  NS_IMETHOD GetPrintRange(int16_t *aPrintRange) MOZ_OVERRIDE;
  NS_IMETHOD SetPrintRange(int16_t aPrintRange) MOZ_OVERRIDE;

  
  NS_IMETHOD GetStartPageRange(int32_t *aStartPageRange) MOZ_OVERRIDE;
  NS_IMETHOD SetStartPageRange(int32_t aStartPageRange) MOZ_OVERRIDE;
  NS_IMETHOD GetEndPageRange(int32_t *aEndPageRange) MOZ_OVERRIDE;
  NS_IMETHOD SetEndPageRange(int32_t aEndPageRange) MOZ_OVERRIDE;

  
  
  NS_IMETHOD GetPrintReversed(bool *aPrintReversed) MOZ_OVERRIDE;
  NS_IMETHOD SetPrintReversed(bool aPrintReversed) MOZ_OVERRIDE;

  NS_IMETHOD GetPrintInColor(bool *aPrintInColor) MOZ_OVERRIDE;
  NS_IMETHOD SetPrintInColor(bool aPrintInColor) MOZ_OVERRIDE;

  NS_IMETHOD GetOrientation(int32_t *aOrientation) MOZ_OVERRIDE;
  NS_IMETHOD SetOrientation(int32_t aOrientation) MOZ_OVERRIDE;

  NS_IMETHOD GetToFileName(char16_t * *aToFileName) MOZ_OVERRIDE;
  NS_IMETHOD SetToFileName(const char16_t * aToFileName) MOZ_OVERRIDE;

  
  
  NS_IMETHOD GetPrinterName(char16_t * *aPrinter) MOZ_OVERRIDE;
  NS_IMETHOD SetPrinterName(const char16_t * aPrinter) MOZ_OVERRIDE;

  
  NS_IMETHOD GetNumCopies(int32_t *aNumCopies) MOZ_OVERRIDE;
  NS_IMETHOD SetNumCopies(int32_t aNumCopies) MOZ_OVERRIDE;

  NS_IMETHOD GetScaling(double *aScaling) MOZ_OVERRIDE;
  NS_IMETHOD SetScaling(double aScaling) MOZ_OVERRIDE;

  
  NS_IMETHOD GetPaperName(char16_t * *aPaperName) MOZ_OVERRIDE;
  NS_IMETHOD SetPaperName(const char16_t * aPaperName) MOZ_OVERRIDE;

  NS_IMETHOD SetUnwriteableMarginInTwips(nsIntMargin& aUnwriteableMargin) MOZ_OVERRIDE;
  NS_IMETHOD SetUnwriteableMarginTop(double aUnwriteableMarginTop) MOZ_OVERRIDE;
  NS_IMETHOD SetUnwriteableMarginLeft(double aUnwriteableMarginLeft) MOZ_OVERRIDE;
  NS_IMETHOD SetUnwriteableMarginBottom(double aUnwriteableMarginBottom) MOZ_OVERRIDE;
  NS_IMETHOD SetUnwriteableMarginRight(double aUnwriteableMarginRight) MOZ_OVERRIDE;

  NS_IMETHOD GetPaperWidth(double *aPaperWidth) MOZ_OVERRIDE;
  NS_IMETHOD SetPaperWidth(double aPaperWidth) MOZ_OVERRIDE;

  NS_IMETHOD GetPaperHeight(double *aPaperHeight) MOZ_OVERRIDE;
  NS_IMETHOD SetPaperHeight(double aPaperHeight) MOZ_OVERRIDE;

  NS_IMETHOD SetPaperSizeUnit(int16_t aPaperSizeUnit) MOZ_OVERRIDE;

  NS_IMETHOD GetEffectivePageSize(double *aWidth, double *aHeight) MOZ_OVERRIDE;

  NS_IMETHOD SetupSilentPrinting() MOZ_OVERRIDE;

  NS_IMETHOD GetPageRanges(nsTArray<int32_t> &aPages) MOZ_OVERRIDE;

  NS_IMETHOD GetResolution(int32_t *aResolution) MOZ_OVERRIDE;
  NS_IMETHOD SetResolution(int32_t aResolution) MOZ_OVERRIDE;

  NS_IMETHOD GetDuplex(int32_t *aDuplex) MOZ_OVERRIDE;
  NS_IMETHOD SetDuplex(int32_t aDuplex) MOZ_OVERRIDE;

protected:
  virtual ~nsPrintSettingsGTK();

  nsPrintSettingsGTK(const nsPrintSettingsGTK& src);
  nsPrintSettingsGTK& operator=(const nsPrintSettingsGTK& rhs);

  virtual nsresult _Clone(nsIPrintSettings **_retval) MOZ_OVERRIDE;
  virtual nsresult _Assign(nsIPrintSettings *aPS) MOZ_OVERRIDE;

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
