






































#include "nsAboutRedirector.h"
#include "nsNetUtil.h"
#include "plstr.h"
#include "nsIScriptSecurityManager.h"
#include "nsAboutProtocolUtils.h"

NS_IMPL_ISUPPORTS1(nsAboutRedirector, nsIAboutModule)

struct RedirEntry {
    const char* id;
    const char* url;
    PRUint32 flags;  
                     
                     
};











static RedirEntry kRedirMap[] = {
    { "credits", "http://www.mozilla.org/credits/",
      nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT },
    { "mozilla", "chrome://global/content/mozilla.xhtml",
      nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT },
    { "plugins", "chrome://global/content/plugins.html", 0 },
    { "config", "chrome://global/content/config.xul", 0 },
#ifdef MOZ_CRASHREPORTER
    { "crashes", "chrome://global/content/crashes.xhtml", 0 },
#endif
    { "logo", "chrome://branding/content/about.png",
      nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT},
    { "buildconfig", "chrome://global/content/buildconfig.html",
      nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT },
    { "license", "chrome://global/content/license.html",
      nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT },
    { "licence", "chrome://global/content/license.html",
      nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT },
    { "neterror", "chrome://global/content/netError.xhtml",
      nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT |
      nsIAboutModule::ALLOW_SCRIPT },
    { "memory", "chrome://global/content/aboutMemory.xhtml",
      nsIAboutModule::ALLOW_SCRIPT }
};
static const int kRedirTotal = NS_ARRAY_LENGTH(kRedirMap);

NS_IMETHODIMP
nsAboutRedirector::NewChannel(nsIURI *aURI, nsIChannel **result)
{
    NS_ENSURE_ARG_POINTER(aURI);
    NS_ASSERTION(result, "must not be null");

    nsresult rv;

    nsCAutoString path;
    rv = NS_GetAboutModuleName(aURI, path);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIIOService> ioService = do_GetIOService(&rv);
    if (NS_FAILED(rv))
        return rv;

    for (int i=0; i<kRedirTotal; i++) 
    {
        if (!strcmp(path.get(), kRedirMap[i].id))
        {
            nsCOMPtr<nsIChannel> tempChannel;
            rv = ioService->NewChannel(nsDependentCString(kRedirMap[i].url),
                                       nsnull, nsnull, getter_AddRefs(tempChannel));
            if (NS_FAILED(rv))
                return rv;

            tempChannel->SetOriginalURI(aURI);

            
            if (kRedirMap[i].flags &
                nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT)
            {
                nsCOMPtr<nsIScriptSecurityManager> securityManager = 
                         do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
                if (NS_FAILED(rv))
                    return rv;
            
                nsCOMPtr<nsIPrincipal> principal;
                rv = securityManager->GetCodebasePrincipal(aURI, getter_AddRefs(principal));
                if (NS_FAILED(rv))
                    return rv;
            
                rv = tempChannel->SetOwner(principal);
                if (NS_FAILED(rv))
                    return rv;
            }

            NS_ADDREF(*result = tempChannel);
            return rv;
        }
    }

    NS_ERROR("nsAboutRedirector called for unknown case");
    return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
nsAboutRedirector::GetURIFlags(nsIURI *aURI, PRUint32 *result)
{
    NS_ENSURE_ARG_POINTER(aURI);

    nsCAutoString name;
    nsresult rv = NS_GetAboutModuleName(aURI, name);
    NS_ENSURE_SUCCESS(rv, rv);

    for (int i=0; i < kRedirTotal; i++) 
    {
        if (name.EqualsASCII(kRedirMap[i].id))
        {
            *result = kRedirMap[i].flags;
            return NS_OK;
        }
    }

    NS_ERROR("nsAboutRedirector called for unknown case");
    return NS_ERROR_ILLEGAL_VALUE;
}

NS_METHOD
nsAboutRedirector::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsAboutRedirector* about = new nsAboutRedirector();
    if (about == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
    NS_ADDREF(about);
    nsresult rv = about->QueryInterface(aIID, aResult);
    NS_RELEASE(about);
    return rv;
}
