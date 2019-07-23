






































#include "EmbedWindowCreator.h"

#include "qgeckoembed.h"
#include "qgeckoglobals.h"
#include "EmbedWindow.h"


EmbedWindowCreator::EmbedWindowCreator()
{
}

EmbedWindowCreator::~EmbedWindowCreator()
{
}

NS_IMPL_ISUPPORTS1(EmbedWindowCreator, nsIWindowCreator)

NS_IMETHODIMP
EmbedWindowCreator::CreateChromeWindow(nsIWebBrowserChrome *aParent,
                                       PRUint32 aChromeFlags,
                                       nsIWebBrowserChrome **_retval)
{
    NS_ENSURE_ARG_POINTER(_retval);

    QGeckoEmbed *newEmbed = 0;

    
    if (!aParent) {
        
        qDebug("XXXXXXXXXXXXXXXXXXXXXXXXXXXXX not implemented");
    }
    else {
        
        QGeckoEmbed *qecko = QGeckoGlobals::findPrivateForBrowser(aParent);

        if (!qecko)
            return NS_ERROR_FAILURE;
        newEmbed = qecko;
        emit newEmbed->newWindow(newEmbed, aChromeFlags);
    }

    
    if (!newEmbed)
        return NS_ERROR_FAILURE;

    qDebug("MMMMM here");
    
    if (aChromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME)
        newEmbed->setIsChrome(PR_TRUE);

    *_retval = NS_STATIC_CAST(nsIWebBrowserChrome *,
                              (newEmbed->window()));

    if (*_retval) {
        NS_ADDREF(*_retval);
        return NS_OK;
    }

    return NS_ERROR_FAILURE;
}
