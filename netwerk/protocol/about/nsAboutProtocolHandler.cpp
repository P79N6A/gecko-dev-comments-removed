




#include "base/basictypes.h"

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
#include "nsError.h"
#include "nsNetUtil.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsAutoPtr.h"
#include "nsIWritablePropertyBag2.h"

static NS_DEFINE_CID(kSimpleURICID,     NS_SIMPLEURI_CID);
static NS_DEFINE_CID(kNestedAboutURICID, NS_NESTEDABOUTURI_CID);



NS_IMPL_ISUPPORTS1(nsAboutProtocolHandler, nsIProtocolHandler)




NS_IMETHODIMP
nsAboutProtocolHandler::GetScheme(nsACString &result)
{
    result.AssignLiteral("about");
    return NS_OK;
}

NS_IMETHODIMP
nsAboutProtocolHandler::GetDefaultPort(int32_t *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsAboutProtocolHandler::GetProtocolFlags(uint32_t *result)
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
    *result = nullptr;
    nsresult rv;

    
    nsCOMPtr<nsIURI> url = do_CreateInstance(kSimpleURICID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = url->SetSpec(aSpec);
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    
    
    bool isSafe = false;
    
    nsCOMPtr<nsIAboutModule> aboutMod;
    rv = NS_GetAboutModule(url, getter_AddRefs(aboutMod));
    if (NS_SUCCEEDED(rv)) {
        
        uint32_t flags;
        rv = aboutMod->GetURIFlags(url, &flags);
        NS_ENSURE_SUCCESS(rv, rv);

        isSafe =
            ((flags & nsIAboutModule::URI_SAFE_FOR_UNTRUSTED_CONTENT) != 0);
    }

    if (isSafe) {
        
        
        
        
        nsAutoCString spec;
        rv = url->GetPath(spec);
        NS_ENSURE_SUCCESS(rv, rv);
        
        spec.Insert("moz-safe-about:", 0);

        nsCOMPtr<nsIURI> inner;
        rv = NS_NewURI(getter_AddRefs(inner), spec);
        NS_ENSURE_SUCCESS(rv, rv);

        nsSimpleNestedURI* outer = new nsNestedAboutURI(inner, aBaseURI);
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
        
        rv = aboutMod->NewChannel(uri, result);
        if (NS_SUCCEEDED(rv)) {
            nsRefPtr<nsNestedAboutURI> aboutURI;
            nsresult rv2 = uri->QueryInterface(kNestedAboutURICID,
                                               getter_AddRefs(aboutURI));
            if (NS_SUCCEEDED(rv2) && aboutURI->GetBaseURI()) {
                nsCOMPtr<nsIWritablePropertyBag2> writableBag =
                    do_QueryInterface(*result);
                if (writableBag) {
                    writableBag->
                        SetPropertyAsInterface(NS_LITERAL_STRING("baseURI"),
                                               aboutURI->GetBaseURI());
                }
            }
        }
        return rv;
    }

    

    if (rv == NS_ERROR_FACTORY_NOT_REGISTERED) {
        
        
        rv = NS_ERROR_MALFORMED_URI;
    }

    return rv;
}

NS_IMETHODIMP 
nsAboutProtocolHandler::AllowPort(int32_t port, const char *scheme, bool *_retval)
{
    
    *_retval = false;
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
nsSafeAboutProtocolHandler::GetDefaultPort(int32_t *result)
{
    *result = -1;        
    return NS_OK;
}

NS_IMETHODIMP
nsSafeAboutProtocolHandler::GetProtocolFlags(uint32_t *result)
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
    
    *result = nullptr;
    url.swap(*result);
    return rv;
}

NS_IMETHODIMP
nsSafeAboutProtocolHandler::NewChannel(nsIURI* uri, nsIChannel* *result)
{
    *result = nullptr;
    return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP 
nsSafeAboutProtocolHandler::AllowPort(int32_t port, const char *scheme, bool *_retval)
{
    
    *_retval = false;
    return NS_OK;
}



NS_INTERFACE_MAP_BEGIN(nsNestedAboutURI)
  if (aIID.Equals(kNestedAboutURICID))
      foundInterface = static_cast<nsIURI*>(this);
  else
NS_INTERFACE_MAP_END_INHERITING(nsSimpleNestedURI)


NS_IMETHODIMP
nsNestedAboutURI::Read(nsIObjectInputStream* aStream)
{
    nsresult rv = nsSimpleNestedURI::Read(aStream);
    if (NS_FAILED(rv)) return rv;

    bool haveBase;
    rv = aStream->ReadBoolean(&haveBase);
    if (NS_FAILED(rv)) return rv;

    if (haveBase) {
        nsCOMPtr<nsISupports> supports;
        rv = aStream->ReadObject(true, getter_AddRefs(supports));
        if (NS_FAILED(rv)) return rv;

        mBaseURI = do_QueryInterface(supports, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsNestedAboutURI::Write(nsIObjectOutputStream* aStream)
{
    nsresult rv = nsSimpleNestedURI::Write(aStream);
    if (NS_FAILED(rv)) return rv;

    rv = aStream->WriteBoolean(mBaseURI != nullptr);
    if (NS_FAILED(rv)) return rv;

    if (mBaseURI) {
        
        
        
        
        
        
        
        
        rv = aStream->WriteCompoundObject(mBaseURI, NS_GET_IID(nsISupports),
                                          true);
        if (NS_FAILED(rv)) return rv;
    }

    return NS_OK;
}


 nsSimpleURI*
nsNestedAboutURI::StartClone(nsSimpleURI::RefHandlingEnum aRefHandlingMode)
{
    
    
    
    NS_ENSURE_TRUE(mInnerURI, nullptr);

    nsCOMPtr<nsIURI> innerClone;
    nsresult rv = aRefHandlingMode == eHonorRef ?
        mInnerURI->Clone(getter_AddRefs(innerClone)) :
        mInnerURI->CloneIgnoringRef(getter_AddRefs(innerClone));

    if (NS_FAILED(rv)) {
        return nullptr;
    }

    nsNestedAboutURI* url = new nsNestedAboutURI(innerClone, mBaseURI);
    url->SetMutable(false);

    return url;
}


NS_IMETHODIMP
nsNestedAboutURI::GetClassIDNoAlloc(nsCID *aClassIDNoAlloc)
{
    *aClassIDNoAlloc = kNestedAboutURICID;
    return NS_OK;
}
