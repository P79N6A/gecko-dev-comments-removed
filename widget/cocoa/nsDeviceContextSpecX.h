




#ifndef nsDeviceContextSpecX_h_
#define nsDeviceContextSpecX_h_

#include "nsIDeviceContextSpec.h"

#include <ApplicationServices/ApplicationServices.h>

class nsDeviceContextSpecX : public nsIDeviceContextSpec
{
public:
    NS_DECL_ISUPPORTS

    nsDeviceContextSpecX();

    NS_IMETHOD Init(nsIWidget *aWidget, nsIPrintSettings* aPS, bool aIsPrintPreview) MOZ_OVERRIDE;
    NS_IMETHOD GetSurfaceForPrinter(gfxASurface **surface) MOZ_OVERRIDE;
    NS_IMETHOD BeginDocument(const nsAString& aTitle,
                             char16_t*       aPrintToFileName,
                             int32_t          aStartPage,
                             int32_t          aEndPage) MOZ_OVERRIDE;
    NS_IMETHOD EndDocument() MOZ_OVERRIDE;
    NS_IMETHOD BeginPage() MOZ_OVERRIDE;
    NS_IMETHOD EndPage() MOZ_OVERRIDE;

    void GetPaperRect(double* aTop, double* aLeft, double* aBottom, double* aRight);

protected:
    virtual ~nsDeviceContextSpecX();

protected:
    PMPrintSession    mPrintSession;              
    PMPageFormat      mPageFormat;                
    PMPrintSettings   mPrintSettings;             
};

#endif 
