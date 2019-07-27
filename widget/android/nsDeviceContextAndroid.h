



#include "nsIDeviceContextSpec.h"
#include "nsCOMPtr.h"

class nsDeviceContextSpecAndroid final : public nsIDeviceContextSpec
{
private:
    ~nsDeviceContextSpecAndroid() {}

public:
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

private:
    nsCOMPtr<nsIPrintSettings> mPrintSettings;
    nsCOMPtr<nsIFile> mTempFile;
};
