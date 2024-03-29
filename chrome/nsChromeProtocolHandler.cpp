











#include "nsChromeProtocolHandler.h"
#include "nsChromeRegistry.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsIChannel.h"
#include "nsIChromeRegistry.h"
#include "nsIFile.h"
#include "nsIFileChannel.h"
#include "nsIIOService.h"
#include "nsILoadGroup.h"
#include "nsIScriptSecurityManager.h"
#include "nsIStandardURL.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsIURL.h"
#include "nsString.h"
#include "nsStandardURL.h"



NS_IMPL_ISUPPORTS(nsChromeProtocolHandler,
                  nsIProtocolHandler,
                  nsISupportsWeakReference)




NS_IMETHODIMP
nsChromeProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("chrome");
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::GetDefaultPort(int32_t *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::AllowPort(int32_t port, const char *scheme, bool *_retval)
{
    
    *_retval = false;
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::GetProtocolFlags(uint32_t *result)
{
    *result = URI_STD | URI_IS_UI_RESOURCE | URI_IS_LOCAL_RESOURCE;
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::NewURI(const nsACString &aSpec,
                                const char *aCharset,
                                nsIURI *aBaseURI,
                                nsIURI **result)
{

    
    

    nsRefPtr<nsStandardURL> surl = new nsStandardURL();

    nsresult rv = surl->Init(nsIStandardURL::URLTYPE_STANDARD, -1, aSpec,
                             aCharset, aBaseURI);
    if (NS_FAILED(rv))
        return rv;

    
    
    

    rv = nsChromeRegistry::Canonify(surl);
    if (NS_FAILED(rv))
        return rv;

    surl->SetMutable(false);

    surl.forget(result);
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::NewChannel2(nsIURI* aURI,
                                     nsILoadInfo* aLoadInfo,
                                     nsIChannel** aResult)
{
    nsresult rv;

    NS_ENSURE_ARG_POINTER(aURI);
    NS_PRECONDITION(aResult, "Null out param");

#ifdef DEBUG
    
    nsresult debug_rv;
    nsCOMPtr<nsIURI> debugClone;
    debug_rv = aURI->Clone(getter_AddRefs(debugClone));
    if (NS_SUCCEEDED(debug_rv)) {
        nsCOMPtr<nsIURL> debugURL (do_QueryInterface(debugClone));
        debug_rv = nsChromeRegistry::Canonify(debugURL);
        if (NS_SUCCEEDED(debug_rv)) {
            bool same;
            debug_rv = aURI->Equals(debugURL, &same);
            if (NS_SUCCEEDED(debug_rv)) {
                NS_ASSERTION(same, "Non-canonified chrome uri passed to nsChromeProtocolHandler::NewChannel!");
            }
        }
    }
#endif

    nsCOMPtr<nsIChannel> result;

    if (!nsChromeRegistry::gChromeRegistry) {
        
        
        nsCOMPtr<nsIChromeRegistry> reg =
            mozilla::services::GetChromeRegistryService();
        NS_ENSURE_TRUE(nsChromeRegistry::gChromeRegistry, NS_ERROR_FAILURE);
    }

    nsCOMPtr<nsIURI> resolvedURI;
    rv = nsChromeRegistry::gChromeRegistry->ConvertChromeURL(aURI, getter_AddRefs(resolvedURI));
    if (NS_FAILED(rv)) {
#ifdef DEBUG
        nsAutoCString spec;
        aURI->GetSpec(spec);
        printf("Couldn't convert chrome URL: %s\n", spec.get());
#endif
        return rv;
    }

    rv = NS_NewChannelInternal(getter_AddRefs(result),
                               resolvedURI,
                               aLoadInfo);
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
    nsCOMPtr<nsIFileChannel> fileChan(do_QueryInterface(result));
    if (fileChan) {
        nsCOMPtr<nsIFile> file;
        fileChan->GetFile(getter_AddRefs(file));

        bool exists = false;
        file->Exists(&exists);
        if (!exists) {
            nsAutoCString path;
            file->GetNativePath(path);
            printf("Chrome file doesn't exist: %s\n", path.get());
        }
    }
#endif

    
    
    nsLoadFlags loadFlags = 0;
    result->GetLoadFlags(&loadFlags);
    result->SetLoadFlags(loadFlags & ~nsIChannel::LOAD_REPLACE);
    rv = result->SetOriginalURI(aURI);
    if (NS_FAILED(rv)) return rv;

    
    
    nsCOMPtr<nsIURL> url = do_QueryInterface(aURI);
    nsAutoCString path;
    rv = url->GetPath(path);
    if (StringBeginsWith(path, NS_LITERAL_CSTRING("/content/")))
    {
        nsCOMPtr<nsIScriptSecurityManager> securityManager =
                 do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIPrincipal> principal;
        rv = securityManager->GetSystemPrincipal(getter_AddRefs(principal));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsISupports> owner = do_QueryInterface(principal);
        result->SetOwner(owner);
    }

    
    
    
    
    
    result->SetContentCharset(NS_LITERAL_CSTRING("UTF-8"));

    *aResult = result;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsChromeProtocolHandler::NewChannel(nsIURI* aURI,
                                    nsIChannel* *aResult)
{
  return NewChannel2(aURI, nullptr, aResult);
}


