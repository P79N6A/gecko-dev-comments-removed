





























#include "nsIWebBrowserChrome.h"
#include "WindowCreator.h"
#include "winEmbed.h"

WindowCreator::WindowCreator()
{
}

WindowCreator::~WindowCreator()
{
}

NS_IMPL_ISUPPORTS1(WindowCreator, nsIWindowCreator)

NS_IMETHODIMP
WindowCreator::CreateChromeWindow(nsIWebBrowserChrome *parent,
                                  uint32_t chromeFlags,
                                  nsIWebBrowserChrome **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    AppCallbacks::CreateBrowserWindow(int32_t(chromeFlags), parent, _retval);
    return *_retval ? NS_OK : NS_ERROR_FAILURE;
}
