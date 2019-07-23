




































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
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIChannel> tempChannel;
    rv = ioService->NewChannel(NS_LITERAL_CSTRING(kURI), nsnull, nsnull, 
                               getter_AddRefs(tempChannel));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIScriptSecurityManager> securityManager = 
             do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrincipal> principal;
    rv = securityManager->GetCodebasePrincipal(aURI, getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

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
