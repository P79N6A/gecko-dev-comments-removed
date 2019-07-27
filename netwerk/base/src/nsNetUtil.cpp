





#include "mozilla/LoadContext.h"
#include "nsNetUtil.h"
#include "nsHttp.h"

bool NS_IsReasonableHTTPHeaderValue(const nsACString& aValue)
{
  return mozilla::net::nsHttp::IsReasonableHeaderValue(aValue);
}

bool NS_IsValidHTTPToken(const nsACString& aToken)
{
  return mozilla::net::nsHttp::IsValidToken(aToken);
}

nsresult
NS_NewLoadGroup(nsILoadGroup** aResult, nsIPrincipal* aPrincipal,
                nsILoadGroup* aOptionalBase)
{
    using mozilla::LoadContext;
    nsresult rv;

    nsCOMPtr<nsILoadContext> baseLoadContext;
    if (aOptionalBase) {
      nsCOMPtr<nsIInterfaceRequestor> cb;
      rv = aOptionalBase->GetNotificationCallbacks(getter_AddRefs(cb));
      NS_ENSURE_SUCCESS(rv, rv);

      baseLoadContext = do_QueryInterface(cb);
    }

    nsCOMPtr<nsILoadGroup> group =
        do_CreateInstance(NS_LOADGROUP_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<LoadContext> loadContext = new LoadContext(aPrincipal,
                                                        baseLoadContext);
    rv = group->SetNotificationCallbacks(loadContext);
    NS_ENSURE_SUCCESS(rv, rv);

    group.forget(aResult);
    return rv;
}

bool
NS_LoadGroupMatchesPrincipal(nsILoadGroup* aLoadGroup,
                             nsIPrincipal* aPrincipal)
{
    if (!aPrincipal) {
      return false;
    }

    
    
    
    bool isNullPrincipal;
    nsresult rv = aPrincipal->GetIsNullPrincipal(&isNullPrincipal);
    NS_ENSURE_SUCCESS(rv, false);
    if (isNullPrincipal) {
      return true;
    }

    if (!aLoadGroup) {
        return false;
    }

    nsCOMPtr<nsILoadContext> loadContext;
    NS_QueryNotificationCallbacks(nullptr, aLoadGroup, NS_GET_IID(nsILoadContext),
                                  getter_AddRefs(loadContext));
    NS_ENSURE_TRUE(loadContext, false);

    
    uint32_t contextAppId;
    bool contextInBrowserElement;
    rv = loadContext->GetAppId(&contextAppId);
    NS_ENSURE_SUCCESS(rv, false);
    rv = loadContext->GetIsInBrowserElement(&contextInBrowserElement);
    NS_ENSURE_SUCCESS(rv, false);

    uint32_t principalAppId;
    bool principalInBrowserElement;
    rv = aPrincipal->GetAppId(&principalAppId);
    NS_ENSURE_SUCCESS(rv, false);
    rv = aPrincipal->GetIsInBrowserElement(&principalInBrowserElement);
    NS_ENSURE_SUCCESS(rv, false);

    return contextAppId == principalAppId &&
           contextInBrowserElement == principalInBrowserElement;
}
