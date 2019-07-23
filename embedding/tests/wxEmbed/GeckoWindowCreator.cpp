





























#include "GeckoWindowCreator.h"
#include "GeckoContainer.h"

#include "nsIWebBrowserChrome.h"
#include "nsString.h"

GeckoWindowCreator::GeckoWindowCreator()
{
}

GeckoWindowCreator::~GeckoWindowCreator()
{
}

NS_IMPL_ISUPPORTS1(GeckoWindowCreator, nsIWindowCreator)

NS_IMETHODIMP
GeckoWindowCreator::CreateChromeWindow(nsIWebBrowserChrome *parent,
                                  PRUint32 chromeFlags,
                                  nsIWebBrowserChrome **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = nsnull;

    if (!parent)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIGeckoContainer> geckoContainer = do_QueryInterface(parent);
    if (!geckoContainer)
        return NS_ERROR_FAILURE;

    nsCAutoString role;
    geckoContainer->GetRole(role);
    if (stricmp(role.get(), "browser") == 0)
    {
        GeckoContainerUI *pUI = NULL;
        geckoContainer->GetContainerUI(&pUI);
        if (pUI)
        {
            return pUI->CreateBrowserWindow(chromeFlags, parent, _retval);
        }
    }

    return NS_ERROR_FAILURE;
}
