





































#include <strings.h>
#include <nsXPIDLString.h>

#include "nsIURI.h"

#include "EmbedContentListener.h"
#include "qgeckoembed.h"

#include "nsServiceManagerUtils.h"
#include "nsIWebNavigationInfo.h"
#include "nsDocShellCID.h"

EmbedContentListener::EmbedContentListener(QGeckoEmbed *aOwner)
{
    mOwner = aOwner;
}

EmbedContentListener::~EmbedContentListener()
{
}

NS_IMPL_ISUPPORTS2(EmbedContentListener,
                   nsIURIContentListener,
                   nsISupportsWeakReference)

NS_IMETHODIMP
EmbedContentListener::OnStartURIOpen(nsIURI     *aURI,
                                     PRBool     *aAbortOpen)
{
    nsresult rv;

    nsCAutoString specString;
    rv = aURI->GetSpec(specString);

    if (NS_FAILED(rv))
        return rv;

    
    
    bool abort = false;
    mOwner->startURIOpen(specString.get(), abort);
    *aAbortOpen = abort;

    return NS_OK;
}

NS_IMETHODIMP
EmbedContentListener::DoContent(const char         *aContentType,
                                PRBool             aIsContentPreferred,
                                nsIRequest         *aRequest,
                                nsIStreamListener **aContentHandler,
                                PRBool             *aAbortProcess)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::IsPreferred(const char        *aContentType,
                                  char             **aDesiredContentType,
                                  PRBool            *aCanHandleContent)
{
    return CanHandleContent(aContentType, PR_TRUE, aDesiredContentType,
                            aCanHandleContent);
}

NS_IMETHODIMP
EmbedContentListener::CanHandleContent(const char        *aContentType,
                                       PRBool           aIsContentPreferred,
                                       char             **aDesiredContentType,
                                       PRBool            *_retval)
{
    *_retval = PR_FALSE;
    *aDesiredContentType = nsnull;
    qDebug("HANDLING:");

    if (aContentType) {
        nsCOMPtr<nsIWebNavigationInfo> webNavInfo(
           do_GetService(NS_WEBNAVIGATION_INFO_CONTRACTID));
        if (webNavInfo) {
            PRUint32 canHandle;
            nsresult rv =
                webNavInfo->IsTypeSupported(nsDependentCString(aContentType),
                                            mOwner ?
                                              mOwner->d->navigation.get() :
                                              nsnull,
                                            &canHandle);
            NS_ENSURE_SUCCESS(rv, rv);
            *_retval = (canHandle != nsIWebNavigationInfo::UNSUPPORTED);
        }
    }

    qDebug("\tCan handle content %s: %d", aContentType, *_retval);
    return NS_OK;
}

NS_IMETHODIMP
EmbedContentListener::GetLoadCookie(nsISupports **aLoadCookie)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::SetLoadCookie(nsISupports *aLoadCookie)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::GetParentContentListener(nsIURIContentListener **aParent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
EmbedContentListener::SetParentContentListener(nsIURIContentListener *aParent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

