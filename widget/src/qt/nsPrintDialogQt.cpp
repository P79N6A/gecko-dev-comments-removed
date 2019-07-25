







































#include "nsPrintSettingsQt.h"
#include "nsPrintDialogQt.h"





NS_IMPL_ISUPPORTS1(nsPrintDialogServiceQt, nsIPrintDialogService)

nsPrintDialogServiceQt::nsPrintDialogServiceQt()
{
}

nsPrintDialogServiceQt::~nsPrintDialogServiceQt()
{
}

NS_IMETHODIMP
nsPrintDialogServiceQt::Init()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPrintDialogServiceQt::Show(nsIDOMWindow* aParent,
                             nsIPrintSettings* aSettings,
                             nsIWebBrowserPrint* aWebBrowserPrint)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsPrintDialogServiceQt::ShowPageSetup(nsIDOMWindow* aParent,
                                      nsIPrintSettings* aNSSettings)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
