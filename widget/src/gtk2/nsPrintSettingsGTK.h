





































#ifndef nsPrintSettingsGTK_h_
#define nsPrintSettingsGTK_h_

#include "nsPrintSettingsImpl.h"  
#include "nsIPrintSettingsGTK.h"





class nsPrintSettingsGTK : public nsPrintSettings,
                           public nsIPrintSettingsGTK
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIPRINTSETTINGSGTK

  nsPrintSettingsGTK();
  virtual ~nsPrintSettingsGTK();

  
  
  

  
  
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

  NS_IMETHOD GetEdgeTop(double *aEdgeTop);
  NS_IMETHOD SetEdgeTop(double aEdgeTop);

  NS_IMETHOD GetEdgeLeft(double *aEdgeLeft);
  NS_IMETHOD SetEdgeLeft(double aEdgeLeft);

  NS_IMETHOD GetEdgeBottom(double *aEdgeBottom);
  NS_IMETHOD SetEdgeBottom(double aEdgeBottom);

  NS_IMETHOD GetEdgeRight(double *aEdgeRight);
  NS_IMETHOD SetEdgeRight(double aEdgeRight);

  NS_IMETHOD GetScaling(double *aScaling);
  NS_IMETHOD SetScaling(double aScaling);

  
  NS_IMETHOD GetPaperName(PRUnichar * *aPaperName);
  NS_IMETHOD SetPaperName(const PRUnichar * aPaperName);

  NS_IMETHOD GetPaperWidth(double *aPaperWidth);
  NS_IMETHOD SetPaperWidth(double aPaperWidth);

  NS_IMETHOD GetPaperHeight(double *aPaperHeight);
  NS_IMETHOD SetPaperHeight(double aPaperHeight);

  NS_IMETHOD SetPaperSizeUnit(PRInt16 aPaperSizeUnit);

  NS_IMETHOD SetEdgeInTwips(nsMargin& aEdge);
  NS_IMETHOD GetEdgeInTwips(nsMargin& aEdge);

  NS_IMETHOD GetEffectivePageSize(double *aWidth, double *aHeight);

protected:
  nsPrintSettingsGTK(const nsPrintSettingsGTK& src);
  nsPrintSettingsGTK& operator=(const nsPrintSettingsGTK& rhs);

  virtual nsresult _Clone(nsIPrintSettings **_retval);
  virtual nsresult _Assign(nsIPrintSettings *aPS);

  GtkUnit GetGTKUnit(PRInt16 aGeckoUnit);
  void SaveNewPageSize();

  





  GtkPageSetup* mPageSetup;
  GtkPrintSettings* mPrintSettings;
  GtkPrinter* mGTKPrinter;
  GtkPaperSize* mPaperSize;

  PRBool mPrintSelectionOnly;
};

#endif 
