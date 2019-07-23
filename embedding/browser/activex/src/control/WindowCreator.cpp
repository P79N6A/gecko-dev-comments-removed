





































#include "stdafx.h"

#include "WindowCreator.h"

NS_IMPL_ISUPPORTS1(CWindowCreator, nsIWindowCreator)

CWindowCreator::CWindowCreator(void)
{
}

CWindowCreator::~CWindowCreator()
{
}

NS_IMETHODIMP
CWindowCreator::CreateChromeWindow(nsIWebBrowserChrome *aParent, PRUint32 aChromeFlags, nsIWebBrowserChrome **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = nsnull;

    
    
    
    
    
    

    if (aParent == nsnull)
    {
        return NS_ERROR_FAILURE;
    }

    CWebBrowserContainer *pContainer = NS_STATIC_CAST(CWebBrowserContainer *, aParent);
    
    CComQIPtr<IDispatch> dispNew;
    VARIANT_BOOL bCancel = VARIANT_FALSE;

    
    if (pContainer->mEvents2)
    {
        pContainer->mEvents2->Fire_NewWindow2(&dispNew, &bCancel);
        if ((bCancel == VARIANT_FALSE) && dispNew)
        {
            CComQIPtr<IMozControlBridge> cpBridge = dispNew;
            if (cpBridge)
            {
                nsIWebBrowser *browser = nsnull;
                cpBridge->GetWebBrowser((void **) &browser);
                if (browser)
                {
                    nsresult rv = browser->GetContainerWindow(_retval);
                    NS_RELEASE(browser);
                    return rv;
                }
            }
        }
    }

    return NS_ERROR_FAILURE;
}

