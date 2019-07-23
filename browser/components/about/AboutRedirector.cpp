








































#include "AboutRedirector.h"
#include "nsNetUtil.h"
#include "nsIScriptSecurityManager.h"

namespace mozilla {
namespace browser {

NS_IMPL_ISUPPORTS1(AboutRedirector, nsIAboutModule)

struct RedirEntry {
  const char* id;
  const char* url;
  PRUint32 flags;  
                   
                   
};











static RedirEntry kRedirMap[] = {
#ifdef MOZ_SAFE_BROWSING
  { "blocked", "chrome://browser/content/safebrowsing/blockedSite.xhtml",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
    nsIAboutModule::ALLOW_SCRIPT },
#endif
  { "certerror", "chrome://browser/content/certerror/aboutCertError.xhtml",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
    nsIAboutModule::ALLOW_SCRIPT },
  { "feeds", "chrome://browser/content/feeds/subscribe.xhtml",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
    nsIAboutModule::ALLOW_SCRIPT },
  { "privatebrowsing", "chrome://browser/content/aboutPrivateBrowsing.xhtml",
    nsIAboutModule::ALLOW_SCRIPT },
  { "rights",
#ifdef OFFICIAL_BUILD
    "chrome://global/content/aboutRights.xhtml",
#else
    "chrome://global/content/aboutRights-unbranded.xhtml",
#endif
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
    nsIAboutModule::ALLOW_SCRIPT },
  { "robots", "chrome://browser/content/aboutRobots.xhtml",
    nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
    nsIAboutModule::ALLOW_SCRIPT },
  { "sessionrestore", "chrome://browser/content/aboutSessionRestore.xhtml",
    nsIAboutModule::ALLOW_SCRIPT }
};
static const int kRedirTotal = NS_ARRAY_LENGTH(kRedirMap);

static nsCAutoString
GetAboutModuleName(nsIURI *aURI)
{
  nsCAutoString path;
  aURI->GetPath(path);

  PRInt32 f = path.FindChar('#');
  if (f >= 0)
    path.SetLength(f);

  f = path.FindChar('?');
  if (f >= 0)
    path.SetLength(f);

  ToLowerCase(path);
  return path;
}

NS_IMETHODIMP
AboutRedirector::NewChannel(nsIURI *aURI, nsIChannel **result) 
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ASSERTION(result, "must not be null");

  nsCAutoString path = GetAboutModuleName(aURI);

  nsresult rv;
  nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (int i = 0; i < kRedirTotal; i++) {
    if (!strcmp(path.get(), kRedirMap[i].id)) {
      nsCOMPtr<nsIChannel> tempChannel;
      rv = ioService->NewChannel(nsDependentCString(kRedirMap[i].url),
                                 nsnull, nsnull, getter_AddRefs(tempChannel));
      NS_ENSURE_SUCCESS(rv, rv);

      tempChannel->SetOriginalURI(aURI);

      
      if (kRedirMap[i].flags & nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT) {
        nsCOMPtr<nsIScriptSecurityManager> securityManager =
          do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIPrincipal> principal;
        rv = securityManager->GetCodebasePrincipal(aURI, getter_AddRefs(principal));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = tempChannel->SetOwner(principal);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      NS_ADDREF(*result = tempChannel);
      return rv;
    }
  }

  return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
AboutRedirector::GetURIFlags(nsIURI *aURI, PRUint32 *result)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsCAutoString name = GetAboutModuleName(aURI);

  for (int i = 0; i < kRedirTotal; i++) {
    if (name.Equals(kRedirMap[i].id)) {
      *result = kRedirMap[i].flags;
      return NS_OK;
    }
  }

  return NS_ERROR_ILLEGAL_VALUE;
}

NS_METHOD
AboutRedirector::Create(nsISupports *aOuter, REFNSIID aIID, void **result)
{
  AboutRedirector* about = new AboutRedirector();
  if (about == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(about);
  nsresult rv = about->QueryInterface(aIID, result);
  NS_RELEASE(about);
  return rv;
}

} 
} 
