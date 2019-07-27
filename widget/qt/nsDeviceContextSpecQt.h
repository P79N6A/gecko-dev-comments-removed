





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

    NS_DECL_ISUPPORTS

    NS_IMETHOD GetSurfaceForPrinter(gfxASurface** surface);

    NS_IMETHOD Init(nsIWidget* aWidget,
                    nsIPrintSettings* aPS,
                    bool aIsPrintPreview);
    NS_IMETHOD BeginDocument(const nsAString& aTitle,
                             char16_t* aPrintToFileName,
                             int32_t aStartPage,
                             int32_t aEndPage);
    NS_IMETHOD EndDocument();
    NS_IMETHOD BeginPage() { return NS_OK; }
    NS_IMETHOD EndPage() { return NS_OK; }

    NS_IMETHOD GetPath (const char** aPath);

protected:
    virtual ~nsDeviceContextSpecQt();

    nsCOMPtr<nsIPrintSettings> mPrintSettings;
    bool mToPrinter : 1;      
    bool mIsPPreview : 1;     
    char   mPath[PATH_MAX];     
    char   mPrinter[256];       
    nsCString         mSpoolName;
    nsCOMPtr<nsIFile> mSpoolFile;
};

class nsPrinterEnumeratorQt : public nsIPrinterEnumerator
{
public:
    nsPrinterEnumeratorQt();
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPRINTERENUMERATOR

protected:
    virtual ~nsPrinterEnumeratorQt();

};

#endif 
