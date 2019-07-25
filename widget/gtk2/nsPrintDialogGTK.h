




#ifndef nsPrintDialog_h__
#define nsPrintDialog_h__

#include "nsIPrintDialogService.h"

class nsIPrintSettings;



typedef enum
{
  _GTK_PRINT_PAGES_ALL,
  _GTK_PRINT_PAGES_CURRENT,
  _GTK_PRINT_PAGES_RANGES,
  _GTK_PRINT_PAGES_SELECTION
} _GtkPrintPages;

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
