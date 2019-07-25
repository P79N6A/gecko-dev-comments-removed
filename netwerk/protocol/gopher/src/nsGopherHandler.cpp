








































#include "nsGopherChannel.h"
#include "nsGopherHandler.h"
#include "nsIURL.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIStandardURL.h"
#include "nsStandardURL.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"



NS_IMPL_THREADSAFE_ISUPPORTS2(nsGopherHandler,
                              nsIProxiedProtocolHandler,
                              nsIProtocolHandler)



nsGopherHandler *gGopherHandler = nsnull;

nsGopherHandler::nsGopherHandler()
{
    gGopherHandler = this;
}

nsGopherHandler::~nsGopherHandler()
{
    gGopherHandler = nsnull;
}

PRUint8
nsGopherHandler::GetQoSBits()
{
    nsresult rv;
    nsCOMPtr<nsIPrefBranch2> branch = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    if (NS_SUCCEEDED(rv)) {
	PRInt32 val;
	rv = branch->GetIntPref("network.gopher.qos", &val);
	if (NS_SUCCEEDED(rv))
	    return NS_CLAMP(val, 0, 0xff);
    }
    return 0x00;
}

NS_IMETHODIMP
nsGopherHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("gopher");
    return NS_OK;
}

NS_IMETHODIMP
nsGopherHandler::GetDefaultPort(PRInt32 *result)
{
    *result = GOPHER_PORT;
    return NS_OK;
}

NS_IMETHODIMP
nsGopherHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_NORELATIVE | ALLOWS_PROXY | ALLOWS_PROXY_HTTP |
      URI_LOADABLE_BY_ANYONE;
    return NS_OK;
}

NS_IMETHODIMP
nsGopherHandler::NewURI(const nsACString &spec, const char *originCharset,
                        nsIURI *baseURI, nsIURI **result)
{
    nsStandardURL *url = new nsStandardURL();
    if (!url)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(url);

    nsresult rv = url->Init(nsIStandardURL::URLTYPE_STANDARD, GOPHER_PORT,
                            spec, originCharset, baseURI);
    if (NS_FAILED(rv)) {
        NS_RELEASE(url);
        return rv;
    }

    *result = url;  
    return NS_OK;
}

NS_IMETHODIMP
nsGopherHandler::NewProxiedChannel(nsIURI *uri, nsIProxyInfo *proxyInfo,
                                   nsIChannel **result)
{
    NS_ENSURE_ARG_POINTER(uri);
    nsGopherChannel *chan = new nsGopherChannel(uri, proxyInfo);
    if (!chan)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(chan);

    nsresult rv = chan->Init();
    if (NS_FAILED(rv)) {
        NS_RELEASE(chan);
        return rv;
    }
    
    *result = chan;
    return NS_OK;
}

NS_IMETHODIMP
nsGopherHandler::NewChannel(nsIURI *uri, nsIChannel **result)
{
    return NewProxiedChannel(uri, nsnull, result);
}

NS_IMETHODIMP 
nsGopherHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *result)
{
    *result = (port == GOPHER_PORT);
    return NS_OK;
}
