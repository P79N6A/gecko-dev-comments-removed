





































#ifndef nsDeviceContextSpecX_h___
#define nsDeviceContextSpecX_h___

#include "nsIDeviceContextSpec.h"
#include "nsIPrintingContext.h"
#include "nsDeviceContextMac.h"

#include <PMApplication.h>

class nsDeviceContextSpecX : public nsIDeviceContextSpec
#ifndef MOZ_CAIRO_GFX
, public nsIPrintingContext
#endif
{
public:
    



    nsDeviceContextSpecX();

    NS_DECL_ISUPPORTS
#ifdef MOZ_CAIRO_GFX
    NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);
    NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                             PRUnichar*  aPrintToFileName,
                             PRInt32     aStartPage, 
                             PRInt32     aEndPage) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD EndDocument() { return NS_ERROR_NOT_IMPLEMENTED; }
#endif

    










    NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);
    NS_IMETHOD Init(nsIPrintSettings* aPS, PRBool aIsPrintPreview);

    





    NS_IMETHOD PrintManagerOpen(PRBool* aIsOpen);

    




    NS_IMETHOD ClosePrintManager();

#ifndef MOZ_CAIRO_GFX
    NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                             PRUnichar*  aPrintToFileName,
                             PRInt32     aStartPage, 
                             PRInt32     aEndPage);
    
    NS_IMETHOD EndDocument();
    
    NS_IMETHOD AbortDocument();
    
    NS_IMETHOD BeginPage();
    
    NS_IMETHOD EndPage();
#endif

    NS_IMETHOD GetPrinterResolution(double* aResolution);
    
    NS_IMETHOD GetPageRect(double* aTop, double* aLeft, double* aBottom, double* aRight);
protected:




  virtual ~nsDeviceContextSpecX();

protected:

    PMPrintSession    mPrintSession;              
    PMPageFormat      mPageFormat;                
    PMPrintSettings   mPrintSettings;             
    CGrafPtr          mSavedPort;                 
    PRBool            mBeganPrinting;
};

#endif
