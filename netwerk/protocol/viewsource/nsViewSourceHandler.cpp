







































#include "nsViewSourceHandler.h"
#include "nsViewSourceChannel.h"
#include "nsNetUtil.h"
#include "nsSimpleNestedURI.h"

#define VIEW_SOURCE "view-source"



NS_IMPL_ISUPPORTS1(nsViewSourceHandler, nsIProtocolHandler)




NS_IMETHODIMP
nsViewSourceHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral(VIEW_SOURCE);
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_NORELATIVE | URI_NOAUTH | URI_LOADABLE_BY_ANYONE |
        URI_NON_PERSISTABLE;
    return NS_OK;
}

NS_IMETHODIMP
nsViewSourceHandler::NewURI(const nsACString &aSpec,
                            const char *aCharset,
                            nsIURI *aBaseURI,
                            nsIURI **aResult)
{
    *aResult = nsnull;

    
    

    PRInt32 colon = aSpec.FindChar(':');
    if (colon == kNotFound)
        return NS_ERROR_MALFORMED_URI;

    nsCOMPtr<nsIURI> innerURI;
    nsresult rv = NS_NewURI(getter_AddRefs(innerURI),
                            Substring(aSpec, colon + 1), aCharset, aBaseURI);
    if (NS_FAILED(rv))
        return rv;

    nsCAutoString asciiSpec;
    rv = innerURI->GetAsciiSpec(asciiSpec);
    if (NS_FAILED(rv))
        return rv;

    

    asciiSpec.Insert(VIEW_SOURCE ":", 0);

    
    
    nsSimpleNestedURI* ourURI = new nsSimpleNestedURI(innerURI);
    nsCOMPtr<nsIURI> uri = ourURI;
    if (!uri)
        return NS_ERROR_OUT_OF_MEMORY;

    rv = ourURI->SetSpec(asciiSpec);
    if (NS_FAILED(rv))
        return rv;

    
    
    ourURI->SetMutable(PR_FALSE);

    uri.swap(*aResult);
    return rv;
}

NS_IMETHODIMP
nsViewSourceHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    NS_ENSURE_ARG_POINTER(uri);
    nsViewSourceChannel *channel = new nsViewSourceChannel();
    if (!channel)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(channel);

    nsresult rv = channel->Init(uri);
    if (NS_FAILED(rv)) {
        NS_RELEASE(channel);
        return rv;
    }

    *result = static_cast<nsIViewSourceChannel*>(channel);
    return NS_OK;
}

NS_IMETHODIMP 
nsViewSourceHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}
