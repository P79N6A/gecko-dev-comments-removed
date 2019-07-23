





































#include "nsAboutProtocolHandler.h"
#include "nsIURI.h"
#include "nsIIOService.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIAboutModule.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsNetCID.h"
#include "nsAboutProtocolUtils.h"
#include "nsNetError.h"
#include "nsNetUtil.h"
#include "nsSimpleNestedURI.h"

static NS_DEFINE_CID(kSimpleURICID,     NS_SIMPLEURI_CID);



NS_IMPL_ISUPPORTS1(nsAboutProtocolHandler, nsIProtocolHandler)




NS_IMETHODIMP
nsAboutProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("about");
    return NS_OK;
}

NS_IMETHODIMP
nsAboutProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsAboutProtocolHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_NORELATIVE | URI_NOAUTH | URI_DANGEROUS_TO_LOAD;
    return NS_OK;
}

NS_IMETHODIMP
nsAboutProtocolHandler::NewURI(const nsACString &aSpec,
                               const char *aCharset, 
                               nsIURI *aBaseURI,
                               nsIURI **result)
{
    *result = nsnull;
    nsresult rv;

    
    nsCOMPtr<nsIURI> url = do_CreateInstance(kSimpleURICID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = url->SetSpec(aSpec);
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    
    
    PRBool isSafe = PR_FALSE;
    
    nsCOMPtr<nsIAboutModule> aboutMod;
    rv = NS_GetAboutModule(url, getter_AddRefs(aboutMod));
    if (NS_SUCCEEDED(rv)) {
        
        PRUint32 flags;
        rv = aboutMod->GetURIFlags(url, &flags);
        NS_ENSURE_SUCCESS(rv, rv);

        isSafe =
            ((flags & nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT) != 0);
    }

    if (isSafe) {
        
        
        
        
        nsCAutoString spec;
        rv = url->GetPath(spec);
        NS_ENSURE_SUCCESS(rv, rv);
        
        spec.Insert("moz-safe-about:", 0);

        nsCOMPtr<nsIURI> inner;
        rv = NS_NewURI(getter_AddRefs(inner), spec);
        NS_ENSURE_SUCCESS(rv, rv);

        nsSimpleNestedURI* outer = new nsSimpleNestedURI(inner);
        NS_ENSURE_TRUE(outer, NS_ERROR_OUT_OF_MEMORY);

        
        url = outer;

        rv = outer->SetSpec(aSpec);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    
    NS_TryToSetImmutable(url);
    url.swap(*result);
    return NS_OK;
}

NS_IMETHODIMP
nsAboutProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    NS_ENSURE_ARG_POINTER(uri);

    
    nsCOMPtr<nsIAboutModule> aboutMod;
    nsresult rv = NS_GetAboutModule(uri, getter_AddRefs(aboutMod));
    if (NS_SUCCEEDED(rv)) {
        
        return aboutMod->NewChannel(uri, result);
    }

    

    if (rv == NS_ERROR_FACTORY_NOT_REGISTERED) {
        
        
        rv = NS_ERROR_MALFORMED_URI;
    }

    return rv;
}

NS_IMETHODIMP 
nsAboutProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}




NS_IMPL_ISUPPORTS1(nsSafeAboutProtocolHandler, nsIProtocolHandler)



NS_IMETHODIMP
nsSafeAboutProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("moz-safe-about");
    return NS_OK;
}

NS_IMETHODIMP
nsSafeAboutProtocolHandler::GetDefaultPort(PRInt32 *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsSafeAboutProtocolHandler::GetProtocolFlags(PRUint32 *result)
{
    *result = URI_NORELATIVE | URI_NOAUTH | URI_LOADABLE_BY_ANYONE;
    return NS_OK;
}

NS_IMETHODIMP
nsSafeAboutProtocolHandler::NewURI(const nsACString &aSpec,
                                   const char *aCharset, 
                                   nsIURI *aBaseURI,
                                   nsIURI **result)
{
    nsresult rv;

    nsCOMPtr<nsIURI> url = do_CreateInstance(kSimpleURICID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = url->SetSpec(aSpec);
    if (NS_FAILED(rv)) {
        return rv;
    }

    NS_TryToSetImmutable(url);
    
    *result = nsnull;
    url.swap(*result);
    return rv;
}

NS_IMETHODIMP
nsSafeAboutProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    *result = nsnull;
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP 
nsSafeAboutProtocolHandler::AllowPort(PRInt32 port, const char *scheme, PRBool *_retval)
{
    
    *_retval = PR_FALSE;
    return NS_OK;
}
