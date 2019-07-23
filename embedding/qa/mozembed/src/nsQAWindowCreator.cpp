




































#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsIWebBrowserChrome.h"
#include "nsQAWindowCreator.h"
#include "nsIQABrowserUIGlue.h"
#include "nsQABrowserCID.h"

WindowCreator::WindowCreator()
{
}

WindowCreator::~WindowCreator()
{
}

NS_IMPL_ISUPPORTS1(WindowCreator, nsIWindowCreator)

NS_IMETHODIMP
WindowCreator::CreateChromeWindow(nsIWebBrowserChrome *parent,
                                  PRUint32 chromeFlags,
                                  nsIWebBrowserChrome **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    nsresult rv;
    nsCOMPtr<nsIQABrowserUIGlue> glue(do_CreateInstance(NS_QABROWSERUIGLUE_CONTRACTID, &rv));
    if (glue)
      glue->CreateNewBrowserWindow(PRInt32(chromeFlags), parent, _retval);
    
    return *_retval ? NS_OK : NS_ERROR_FAILURE;
}
