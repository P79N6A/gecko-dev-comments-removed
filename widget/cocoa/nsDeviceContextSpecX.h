




#ifndef nsDeviceContextSpecX_h_
#define nsDeviceContextSpecX_h_

#include "nsIDeviceContextSpec.h"

#include <ApplicationServices/ApplicationServices.h>

class nsDeviceContextSpecX : public nsIDeviceContextSpec
{
public:
    NS_DECL_ISUPPORTS

    nsDeviceContextSpecX();

    NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, bool aIsPrintPreview) override;
    NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface) override;
    NS_IMETHOD BeginDocument(const nsAString& aTitle,
                             char16_t*       aPrintToFileName,
                             int32_t          aStartPage,
                             int32_t          aEndPage) override;
    NS_IMETHOD EndDocument() override;
    NS_IMETHOD BeginPage() override;
    NS_IMETHOD EndPage() override;

    void GetPaperRect(double* aTop, double* aLeft, double* aBottom, double* aRight);

protected:
    virtual ~nsDeviceContextSpecX();

protected:
    PMPrintSession    mPrintSession;              
    PMPageFormat      mPageFormat;                
    PMPrintSettings   mPrintSettings;             
};

#endif 
