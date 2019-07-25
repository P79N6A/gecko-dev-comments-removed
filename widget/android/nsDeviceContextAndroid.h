




































#include "nsIDeviceContextSpec.h"
#include "nsCOMPtr.h"

class nsDeviceContextSpecAndroid : public nsIDeviceContextSpec
{
public:
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
private:
    nsCOMPtr<nsIPrintSettings> mPrintSettings;
    nsCOMPtr<nsIFile> mTempFile;
};
