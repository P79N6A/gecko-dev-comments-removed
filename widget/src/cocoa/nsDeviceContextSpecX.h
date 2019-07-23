





































#ifndef nsDeviceContextSpecX_h_
#define nsDeviceContextSpecX_h_

#include "nsIDeviceContextSpec.h"

#include <Carbon/Carbon.h>

class nsDeviceContextSpecX : public nsIDeviceContextSpec
{
public:
    nsDeviceContextSpecX();

    NS_DECL_ISUPPORTS
    NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);
    NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                             PRUnichar*  aPrintToFileName,
                             PRInt32     aStartPage, 
                             PRInt32     aEndPage);
    NS_IMETHOD EndDocument();
    NS_IMETHOD BeginPage();
    NS_IMETHOD EndPage();

    






    NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);
    
    void GetPaperRect(double* aTop, double* aLeft, double* aBottom, double* aRight);

protected:
  virtual ~nsDeviceContextSpecX();

protected:
    PMPrintSession    mPrintSession;              
    PMPageFormat      mPageFormat;                
    PMPrintSettings   mPrintSettings;             
};

#endif 
