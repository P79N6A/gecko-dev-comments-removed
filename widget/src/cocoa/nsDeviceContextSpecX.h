





































#ifndef nsDeviceContextSpecX_h_
#define nsDeviceContextSpecX_h_

#include "nsIDeviceContextSpec.h"
#include "nsIPrintingContext.h"

#include <PMApplication.h>

class nsDeviceContextSpecX : public nsIDeviceContextSpec
{
public:
    



    nsDeviceContextSpecX();

    NS_DECL_ISUPPORTS
#ifdef MOZ_CAIRO_GFX
    NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);
    NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                             PRUnichar*  aPrintToFileName,
                             PRInt32     aStartPage, 
                             PRInt32     aEndPage);
    NS_IMETHOD EndDocument();
    NS_IMETHOD BeginPage();
    NS_IMETHOD EndPage();
#endif

    










    NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);
    NS_IMETHOD Init(nsIPrintSettings* aPS, PRBool aIsPrintPreview);

    





    NS_IMETHOD PrintManagerOpen(PRBool* aIsOpen);

    




    NS_IMETHOD ClosePrintManager();

    NS_IMETHOD GetPrinterResolution(double* aResolution);
    
    NS_IMETHOD GetPageRect(double* aTop, double* aLeft, double* aBottom, double* aRight);
protected:




  virtual ~nsDeviceContextSpecX();

protected:

    PMPrintSession    mPrintSession;              
    PMPageFormat      mPageFormat;                
    PMPrintSettings   mPrintSettings;             
    PRBool            mBeganPrinting;
};

#endif 
