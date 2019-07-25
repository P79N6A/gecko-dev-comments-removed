







































#ifndef nsDeviceContextSpecQt_h___
#define nsDeviceContextSpecQt_h___

#include "nsIDeviceContextSpec.h"
#include "nsIPrintSettings.h"
#include "nsIPrintOptions.h"
#include "nsCOMPtr.h"
#include "nsString.h"

#include "nsCRT.h" 

class nsDeviceContextSpecQt : public nsIDeviceContextSpec
{
public:
    nsDeviceContextSpecQt();
    virtual ~nsDeviceContextSpecQt();

    NS_DECL_ISUPPORTS

    NS_IMETHOD GetSurfaceForPrinter(gfxASurface** surface);

    NS_IMETHOD Init(nsIWidget* aWidget,
                    nsIPrintSettings* aPS,
                    bool aIsPrintPreview);
    NS_IMETHOD BeginDocument(PRUnichar* aTitle,
                             PRUnichar* aPrintToFileName,
                             PRInt32 aStartPage,
                             PRInt32 aEndPage);
    NS_IMETHOD EndDocument();
    NS_IMETHOD BeginPage() { return NS_OK; }
    NS_IMETHOD EndPage() { return NS_OK; }

    NS_IMETHOD GetPath (const char** aPath);

protected:
    nsCOMPtr<nsIPrintSettings> mPrintSettings;
    bool mToPrinter : 1;      
    bool mIsPPreview : 1;     
    char   mPath[PATH_MAX];     
    char   mPrinter[256];       
    nsCString              mSpoolName;
    nsCOMPtr<nsILocalFile> mSpoolFile;
};

class nsPrinterEnumeratorQt : public nsIPrinterEnumerator
{
public:
    nsPrinterEnumeratorQt();
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPRINTERENUMERATOR
};

#endif
