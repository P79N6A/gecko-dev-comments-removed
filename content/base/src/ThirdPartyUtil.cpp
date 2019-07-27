



#include "ThirdPartyUtil.h"
#include "nsNetUtil.h"
#include "nsIServiceManager.h"
#include "nsIHttpChannelInternal.h"
#include "nsIDOMWindow.h"
#include "nsILoadContext.h"
#include "nsIPrincipal.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsThreadUtils.h"

NS_IMPL_ISUPPORTS(ThirdPartyUtil, mozIThirdPartyUtil)

nsresult
ThirdPartyUtil::Init()
{
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_NOT_AVAILABLE);

  nsresult rv;
  mTLDService = do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID, &rv);
  return rv;
}




nsresult
ThirdPartyUtil::IsThirdPartyInternal(const nsCString& aFirstDomain,
                                     nsIURI* aSecondURI,
                                     bool* aResult)
{
  NS_ENSURE_ARG(aSecondURI);

  
  nsCString secondDomain;
  nsresult rv = GetBaseDomain(aSecondURI, secondDomain);
  if (NS_FAILED(rv))
    return rv;

  
  *aResult = aFirstDomain != secondDomain;
  return NS_OK;
}


NS_IMETHODIMP
ThirdPartyUtil::GetURIFromWindow(nsIDOMWindow* aWin, nsIURI** result)
{
  nsresult rv;
  nsCOMPtr<nsIScriptObjectPrincipal> scriptObjPrin = do_QueryInterface(aWin);
  if (!scriptObjPrin) {
    return NS_ERROR_INVALID_ARG;
  }

  nsIPrincipal* prin = scriptObjPrin->GetPrincipal();
  if (!prin) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIURI> uri;
  rv = prin->GetURI(result);
  return rv;
}



NS_IMETHODIMP
ThirdPartyUtil::IsThirdPartyURI(nsIURI* aFirstURI,
                                nsIURI* aSecondURI,
                                bool* aResult)
{
  NS_ENSURE_ARG(aFirstURI);
  NS_ENSURE_ARG(aSecondURI);
  NS_ASSERTION(aResult, "null outparam pointer");

  nsCString firstHost;
  nsresult rv = GetBaseDomain(aFirstURI, firstHost);
  if (NS_FAILED(rv))
    return rv;

  return IsThirdPartyInternal(firstHost, aSecondURI, aResult);
}



NS_IMETHODIMP
ThirdPartyUtil::IsThirdPartyWindow(nsIDOMWindow* aWindow,
                                   nsIURI* aURI,
                                   bool* aResult)
{
  NS_ENSURE_ARG(aWindow);
  NS_ASSERTION(aResult, "null outparam pointer");

  bool result;

  
  nsresult rv;
  nsCOMPtr<nsIURI> currentURI;
  rv = GetURIFromWindow(aWindow, getter_AddRefs(currentURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString bottomDomain;
  rv = GetBaseDomain(currentURI, bottomDomain);
  if (NS_FAILED(rv))
    return rv;

  if (aURI) {
    
    rv = IsThirdPartyInternal(bottomDomain, aURI, &result);
    if (NS_FAILED(rv))
      return rv;

    if (result) {
      *aResult = true;
      return NS_OK;
    }
  }

  nsCOMPtr<nsIDOMWindow> current = aWindow, parent;
  nsCOMPtr<nsIURI> parentURI;
  do {
    
    
    rv = current->GetScriptableParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);

    if (SameCOMIdentity(parent, current)) {
      
      *aResult = false;
      return NS_OK;
    }

    rv = GetURIFromWindow(parent, getter_AddRefs(parentURI));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = IsThirdPartyInternal(bottomDomain, parentURI, &result);
    if (NS_FAILED(rv))
      return rv;

    if (result) {
      *aResult = true;
      return NS_OK;
    }

    current = parent;
    currentURI = parentURI;
  } while (1);

  NS_NOTREACHED("should've returned");
  return NS_ERROR_UNEXPECTED;
}




NS_IMETHODIMP 
ThirdPartyUtil::IsThirdPartyChannel(nsIChannel* aChannel,
                                    nsIURI* aURI,
                                    bool* aResult)
{
  NS_ENSURE_ARG(aChannel);
  NS_ASSERTION(aResult, "null outparam pointer");

  nsresult rv;
  bool doForce = false;
  nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal =
    do_QueryInterface(aChannel);
  if (httpChannelInternal) {
    rv = httpChannelInternal->GetForceAllowThirdPartyCookie(&doForce);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    if (doForce && !aURI) {
      *aResult = false;
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIURI> channelURI;
  aChannel->GetURI(getter_AddRefs(channelURI));
  NS_ENSURE_TRUE(channelURI, NS_ERROR_INVALID_ARG);

  nsCString channelDomain;
  rv = GetBaseDomain(channelURI, channelDomain);
  if (NS_FAILED(rv))
    return rv;

  if (aURI) {
    
    bool result;
    rv = IsThirdPartyInternal(channelDomain, aURI, &result);
    if (NS_FAILED(rv))
     return rv;

    
    if (result || doForce) {
      *aResult = result;
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsILoadContext> ctx;
  NS_QueryNotificationCallbacks(aChannel, ctx);
  if (!ctx) return NS_ERROR_INVALID_ARG;

  
  
  
  
  nsCOMPtr<nsIDOMWindow> ourWin, parentWin;
  ctx->GetAssociatedWindow(getter_AddRefs(ourWin));
  if (!ourWin) return NS_ERROR_INVALID_ARG;

  
  
  ourWin->GetScriptableParent(getter_AddRefs(parentWin));
  NS_ENSURE_TRUE(parentWin, NS_ERROR_INVALID_ARG);

  
  
  
  
  
  
  
  
  
  nsLoadFlags flags;
  rv = aChannel->GetLoadFlags(&flags);
  NS_ENSURE_SUCCESS(rv, rv);

  if (flags & nsIChannel::LOAD_DOCUMENT_URI) {
    if (SameCOMIdentity(ourWin, parentWin)) {
      
      
      *aResult = false;
      return NS_OK;
    }

    
    ourWin = parentWin;
  }

  
  
  return IsThirdPartyWindow(ourWin, channelURI, aResult);
}

NS_IMETHODIMP
ThirdPartyUtil::GetTopWindowForChannel(nsIChannel* aChannel, nsIDOMWindow** aWin)
{
  NS_ENSURE_ARG(aWin);

  nsresult rv;
  
  nsCOMPtr<nsILoadContext> ctx;
  NS_QueryNotificationCallbacks(aChannel, ctx);
  if (!ctx) {
    return NS_ERROR_INVALID_ARG;
  }

  nsCOMPtr<nsIDOMWindow> window;
  rv = ctx->GetAssociatedWindow(getter_AddRefs(window));
  if (!window) {
    return NS_ERROR_INVALID_ARG;
  }

  rv = window->GetTop(aWin);
  return rv;
}








NS_IMETHODIMP
ThirdPartyUtil::GetBaseDomain(nsIURI* aHostURI,
                              nsACString& aBaseDomain)
{
  
  
  nsresult rv = mTLDService->GetBaseDomain(aHostURI, 0, aBaseDomain);
  if (rv == NS_ERROR_HOST_IS_IP_ADDRESS ||
      rv == NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS) {
    
    
    
    rv = aHostURI->GetAsciiHost(aBaseDomain);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (aBaseDomain.Length() == 1 && aBaseDomain.Last() == '.')
    return NS_ERROR_INVALID_ARG;

  
  
  
  
  if (aBaseDomain.IsEmpty()) {
    bool isFileURI = false;
    aHostURI->SchemeIs("file", &isFileURI);
    NS_ENSURE_TRUE(isFileURI, NS_ERROR_INVALID_ARG);
  }

  return NS_OK;
}
