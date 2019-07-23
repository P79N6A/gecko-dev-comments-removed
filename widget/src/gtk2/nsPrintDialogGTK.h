




































#ifndef nsPrintDialog_h__
#define nsPrintDialog_h__

#include "nsIPrintDialogService.h"

class nsIPrintSettings;

class nsPrintDialogServiceGTK : public nsIPrintDialogService
{
public:
  nsPrintDialogServiceGTK();
  virtual ~nsPrintDialogServiceGTK();

  NS_DECL_ISUPPORTS

  NS_IMETHODIMP Init();
  NS_IMETHODIMP Show(nsIDOMWindow *aParent, nsIPrintSettings *aSettings,
                     nsIWebBrowserPrint *aWebBrowserPrint);
  NS_IMETHODIMP ShowPageSetup(nsIDOMWindow *aParent,
                              nsIPrintSettings *aSettings);
};

#endif
