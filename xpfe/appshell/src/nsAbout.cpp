




































#include "nsAbout.h"
#include "nsIIOService.h"
#include "nsIServiceManager.h"
#include "nsIChannel.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsNetCID.h"
#include "nsIScriptSecurityManager.h"
#include "nsLiteralString.h"

NS_IMPL_ISUPPORTS1(nsAbout, nsIAboutModule)

static const char kURI[] = "chrome://global/content/about.xhtml";

NS_IMETHODIMP
nsAbout::NewChannel(nsIURI *aURI, nsIChannel **result)
{
    nsresult rv;
    nsCOMPtr<nsIIOService> ioService(do_GetService(NS_IOSERVICE_CONTRACTID, &rv));
    if ( NS_FAILED(rv) )
        return rv;

    nsCOMPtr<nsIChannel> tempChannel;
   	rv = ioService->NewChannel(NS_LITERAL_CSTRING(kURI), nsnull, nsnull, getter_AddRefs(tempChannel));

    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIPrincipal> principal;
    rv = securityManager->GetCodebasePrincipal(aURI, getter_AddRefs(principal));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsISupports> owner = do_QueryInterface(principal);
    rv = tempChannel->SetOwner(owner);
    *result = tempChannel.get();
    NS_ADDREF(*result);
    return rv;
}

NS_IMETHODIMP
nsAbout::GetURIFlags(nsIURI *aURI, PRUint32 *result)
{
    *result = nsIAboutModule::ALLOW_SCRIPT;
    return NS_OK;
}

NS_METHOD
nsAbout::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsAbout* about = new nsAbout();
    if (about == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(about);
    nsresult rv = about->QueryInterface(aIID, aResult);
    NS_RELEASE(about);
    return rv;
}

