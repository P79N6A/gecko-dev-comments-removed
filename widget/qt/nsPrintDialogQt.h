





#ifndef nsPrintDialogQt_h__
#define nsPrintDialogQt_h__

#include "nsIPrintDialogService.h"

class nsIPrintSettings;

class nsPrintDialogServiceQt : public nsIPrintDialogService
{
public:
    nsPrintDialogServiceQt();

    NS_DECL_ISUPPORTS

    NS_IMETHODIMP Init();
    NS_IMETHODIMP Show(nsIDOMWindow* aParent, 
                       nsIPrintSettings* aSettings,
                       nsIWebBrowserPrint* aWebBrowserPrint);
    NS_IMETHODIMP ShowPageSetup(nsIDOMWindow* aParent,
                                nsIPrintSettings* aSettings);

protected:
    virtual ~nsPrintDialogServiceQt();
};

#endif
