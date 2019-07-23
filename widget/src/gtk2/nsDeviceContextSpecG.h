





































#ifndef nsDeviceContextSpecGTK_h___
#define nsDeviceContextSpecGTK_h___

#include "nsIDeviceContextSpec.h"
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h" 
#include "nsVoidArray.h"
#include "nsCOMPtr.h"

#ifndef MOZ_CAIRO_GFX
#ifdef USE_POSTSCRIPT
#include "nsIDeviceContextSpecPS.h"
#endif 
#endif

#include "nsCRT.h" 

#include "nsIPrintJobGTK.h"

#define NS_PORTRAIT  0
#define NS_LANDSCAPE 1

typedef enum
{
  pmInvalid = 0,
  pmPostScript
} PrintMethod;

class nsDeviceContextSpecGTK : public nsIDeviceContextSpec
#ifndef MOZ_CAIRO_GFX
#ifdef USE_POSTSCRIPT
                              , public nsIDeviceContextSpecPS
#endif 
#endif
{
public:
  nsDeviceContextSpecGTK();

  NS_DECL_ISUPPORTS

#ifdef MOZ_CAIRO_GFX
  NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);
#endif

  NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);
  NS_IMETHOD ClosePrintManager(); 
  NS_IMETHOD BeginDocument(PRUnichar * aTitle, PRUnichar * aPrintToFileName, PRInt32 aStartPage, PRInt32 aEndPage);
  NS_IMETHOD EndDocument();
  NS_IMETHOD BeginPage() { return NS_ERROR_NOT_IMPLEMENTED; }
  NS_IMETHOD EndPage() { return NS_ERROR_NOT_IMPLEMENTED; }

  NS_IMETHOD GetToPrinter(PRBool &aToPrinter); 
  NS_IMETHOD GetIsPrintPreview(PRBool &aIsPPreview);
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
  NS_IMETHOD GetPlexName(const char **aPlexName);
  NS_IMETHOD GetResolutionName(const char **aResolutionName);
  NS_IMETHOD GetColorspace(const char **aColorspace);
  NS_IMETHOD GetDownloadFonts(PRBool &aDownloadFonts);   
  virtual ~nsDeviceContextSpecGTK();
  
protected:
  nsCOMPtr<nsIPrintSettings> mPrintSettings;
  PRPackedBool mToPrinter : 1;      
  PRPackedBool mIsPPreview : 1;     
  PRPackedBool mFpf : 1;            
  PRPackedBool mGrayscale : 1;      
  PRPackedBool mDownloadFonts : 1;  
  PRPackedBool mCancel : 1;         
  int    mOrientation;        
  char   mCommand[PATH_MAX];  
  char   mPath[PATH_MAX];     
  char   mPrinter[256];       
  char   mPaperName[256];     
  char   mPlexName[256];      
  char   mResolutionName[256];
  char   mColorspace[256];    
  int    mCopies;             
  float  mLeft;               
  float  mRight;              
  float  mTop;                
  float  mBottom;             
  nsIPrintJobGTK * mPrintJob;
};




class nsPrinterEnumeratorGTK : public nsIPrinterEnumerator
{
public:
  nsPrinterEnumeratorGTK();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRINTERENUMERATOR
};

#endif
