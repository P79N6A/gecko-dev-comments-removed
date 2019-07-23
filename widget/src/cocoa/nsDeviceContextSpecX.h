





































#ifndef nsDeviceContextSpecX_h_
#define nsDeviceContextSpecX_h_

#include "nsIDeviceContextSpec.h"

#include <ApplicationServices/ApplicationServices.h>
#ifdef MOZ_COCOA_PRINTING
#include <Cocoa/Cocoa.h>
#endif

class nsDeviceContextSpecX : public nsIDeviceContextSpec
{
public:
    NS_DECL_ISUPPORTS

    nsDeviceContextSpecX();

    NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, PRBool aIsPrintPreview);
    NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface);
    NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                             PRUnichar*  aPrintToFileName,
                             PRInt32     aStartPage, 
                             PRInt32     aEndPage);
    NS_IMETHOD EndDocument();
    NS_IMETHOD BeginPage();
    NS_IMETHOD EndPage();

    void GetPaperRect(double* aTop, double* aLeft, double* aBottom, double* aRight);

protected:
    virtual ~nsDeviceContextSpecX();

protected:
    PMPrintSession    mPrintSession;              
    PMPageFormat      mPageFormat;                
    PMPrintSettings   mPrintSettings;             
#ifdef MOZ_COCOA_PRINTING
    NSPrintInfo*      mPrintInfo;                 
#endif
};

#endif 
