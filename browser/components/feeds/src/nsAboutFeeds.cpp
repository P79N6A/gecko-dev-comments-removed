





































#include "nsAboutFeeds.h"
#include "nsNetUtil.h"
#include "nsIScriptSecurityManager.h"

NS_IMPL_ISUPPORTS1(nsAboutFeeds, nsIAboutModule)

#define FEEDS_PAGE_URI "chrome://browser/content/feeds/subscribe.xhtml"

NS_IMETHODIMP
nsAboutFeeds::NewChannel(nsIURI* uri, nsIChannel** result)
{

  nsresult rv;
  nsCOMPtr<nsIIOService> ios(do_GetIOService(&rv));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIChannel> channel;
  rv = ios->NewChannel(NS_LITERAL_CSTRING(FEEDS_PAGE_URI),
                       nsnull, nsnull, getter_AddRefs(channel));
  if (NS_FAILED(rv))
    return rv;

  channel->SetOriginalURI(uri);

  nsCOMPtr<nsIScriptSecurityManager> ssm =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIPrincipal> principal;
  rv = ssm->GetCodebasePrincipal(uri, getter_AddRefs(principal));
  if (NS_FAILED(rv))
    return rv;

  rv = channel->SetOwner(principal);
  if (NS_FAILED(rv))
    return rv;

  NS_ADDREF(*result = channel);
  return NS_OK;
}

NS_IMETHODIMP
nsAboutFeeds::GetURIFlags(nsIURI* uri, PRUint32* uriFlags)
{
  
  *uriFlags = URI_SAFE_FOR_UNTRUSTED_CONTENT | ALLOW_SCRIPT;
  return NS_OK;
}

NS_METHOD
nsAboutFeeds::Create(nsISupports* outer, REFNSIID iid, void** result)
{
  nsAboutFeeds* aboutFeeds = new nsAboutFeeds();
  if (aboutFeeds == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(aboutFeeds);
  nsresult rv = aboutFeeds->QueryInterface(iid, result);
  NS_RELEASE(aboutFeeds);
  return rv;
}
