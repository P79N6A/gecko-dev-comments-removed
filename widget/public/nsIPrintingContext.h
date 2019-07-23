





































#ifndef nsIPrintingContextMac_h___
#define nsIPrintingContextMac_h___

#include "nsISupports.h"

class nsIPrintSettings;


#define NS_IPRINTING_CONTEXT_IID    \
{ 0xD9853908, 0xA34D, 0x4D8B,       \
{ 0xB4, 0xD6, 0x5D, 0xC3, 0x6E, 0x21, 0x1B, 0xDD } }

class nsIPrintingContext : public nsISupports {
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRINTING_CONTEXT_IID)
    




    NS_IMETHOD Init(nsIPrintSettings* aPS, PRBool aIsPrintPreview) = 0;

    





    NS_IMETHOD PrintManagerOpen(PRBool* aIsOpen) = 0;

    




    NS_IMETHOD ClosePrintManager() = 0;
    
    NS_IMETHOD BeginDocument(PRUnichar*  aTitle, 
                             PRUnichar*  aPrintToFileName,
                             PRInt32     aStartPage, 
                             PRInt32     aEndPage) = 0;
    
    NS_IMETHOD EndDocument() = 0;
    
    NS_IMETHOD BeginPage() = 0;
    
    NS_IMETHOD EndPage() = 0;
    
    NS_IMETHOD GetPrinterResolution(double* aResolution) = 0;
    
    NS_IMETHOD GetPageRect(double* aTop, double* aLeft, double* aBottom, double* aRight) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrintingContext, NS_IPRINTING_CONTEXT_IID)

#endif 
