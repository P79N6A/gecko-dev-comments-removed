







































#ifndef nsDeviceContextSpecQt_h___
#define nsDeviceContextSpecQt_h___

#include "nsCOMPtr.h"
#include "nsIDeviceContextSpec.h"
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h"
#include "nsVoidArray.h"
#include <limits.h>
#ifdef USE_POSTSCRIPT
#include "nsIDeviceContextSpecPS.h"
#endif 
#ifdef USE_XPRINT
#include "nsIDeviceContextSpecXPrint.h"
#endif 

#define NS_PORTRAIT  0
#define NS_LANDSCAPE 1

typedef enum
{
  pmInvalid = 0,
  pmXprint,
  pmPostScript
} PrintMethod;

class nsDeviceContextSpecQt : public nsIDeviceContextSpec
#ifdef USE_POSTSCRIPT
#warning "postscript hardcore disabled"
#if 0

                            , public nsIDeviceContextSpecPS
#endif
#endif 
#ifdef USE_XPRINT
                              , public nsIDeviceContextSpecXp
#endif 
{
public:
  nsDeviceContextSpecQt();

  NS_DECL_ISUPPORTS

  NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);
  NS_IMETHOD ClosePrintManager();

  NS_IMETHOD GetToPrinter(PRBool &aToPrinter);
  NS_IMETHOD GetPrinterName ( const char **aPrinter );
  NS_IMETHOD GetCopies ( int &aCopies );
  NS_IMETHOD GetFirstPageFirst(PRBool &aFpf);
  NS_IMETHOD GetGrayscale(PRBool &aGrayscale);
  NS_IMETHOD GetTopMargin(float &value);
  NS_IMETHOD GetBottomMargin(float &value);
  NS_IMETHOD GetLeftMargin(float &value);
  NS_IMETHOD GetRightMargin(float &value);
  NS_IMETHOD GetCommand(const char **aCommand);
  NS_IMETHOD GetPath (const char **aPath);
  NS_IMETHOD GetLandscape (PRBool &aLandscape);
  NS_IMETHOD GetUserCancelled(PRBool &aCancel);
  NS_IMETHOD GetPrintMethod(PrintMethod &aMethod);
  static nsresult GetPrintMethod(const char *aPrinter, PrintMethod &aMethod);
  NS_IMETHOD GetPageSizeInTwips(PRInt32 *aWidth, PRInt32 *aHeight);
  NS_IMETHOD GetPaperName(const char **aPaperName);
  virtual ~nsDeviceContextSpecQt();

protected:
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  PRBool mToPrinter;          
  PRBool mFpf;                
  PRBool mGrayscale;          
  int    mOrientation;        
  char   mCommand[PATH_MAX];  
  char   mPath[PATH_MAX];     
  char   mPrinter[256];       
  char   mPaperName[256];     
  int    mCopies;             
  PRBool mCancel;             
  float  mLeft;               
  float  mRight;              
  float  mTop;                
  float  mBottom;             
};




class nsPrinterEnumeratorQt : public nsIPrinterEnumerator
{
public:
  nsPrinterEnumeratorQt();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR
};

#endif
