





#ifndef NSPROMPTUTILS_H_
#define NSPROMPTUTILS_H_

#include "nsIHttpChannel.h"












inline void
NS_SetAuthInfo(nsIAuthInformation* aAuthInfo, const nsString& aUser,
               const nsString& aPassword)
{
  uint32_t flags;
  aAuthInfo->GetFlags(&flags);
  if (flags & nsIAuthInformation::NEED_DOMAIN) {
    
    int32_t idx = aUser.FindChar(char16_t('\\'));
    if (idx == kNotFound) {
      aAuthInfo->SetUsername(aUser);
    } else {
      aAuthInfo->SetDomain(Substring(aUser, 0, idx));
      aAuthInfo->SetUsername(Substring(aUser, idx + 1));
    }
  } else {
    aAuthInfo->SetUsername(aUser);
  }
  aAuthInfo->SetPassword(aPassword);
}














inline void
NS_GetAuthHostPort(nsIChannel* aChannel, nsIAuthInformation* aAuthInfo,
                   bool aMachineProcessing, nsCString& aHost, int32_t* aPort)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aChannel->GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv)) {
    return;
  }

  
  uint32_t flags;
  aAuthInfo->GetFlags(&flags);
  if (flags & nsIAuthInformation::AUTH_PROXY) {
    nsCOMPtr<nsIProxiedChannel> proxied(do_QueryInterface(aChannel));
    NS_ASSERTION(proxied, "proxy auth needs nsIProxiedChannel");

    nsCOMPtr<nsIProxyInfo> info;
    proxied->GetProxyInfo(getter_AddRefs(info));
    NS_ASSERTION(info, "proxy auth needs nsIProxyInfo");

    nsAutoCString idnhost;
    info->GetHost(idnhost);
    info->GetPort(aPort);

    if (aMachineProcessing) {
      nsCOMPtr<nsIIDNService> idnService =
        do_GetService(NS_IDNSERVICE_CONTRACTID);
      if (idnService) {
        idnService->ConvertUTF8toACE(idnhost, aHost);
      } else {
        
        aHost = idnhost;
      }
    } else {
      aHost = idnhost;
    }
  } else {
    if (aMachineProcessing) {
      uri->GetAsciiHost(aHost);
      *aPort = NS_GetRealPort(uri);
    } else {
      uri->GetHost(aHost);
      uri->GetPort(aPort);
    }
  }
}






inline void
NS_GetAuthKey(nsIChannel* aChannel, nsIAuthInformation* aAuthInfo,
              nsCString& aKey)
{
  
  nsCOMPtr<nsIHttpChannel> http(do_QueryInterface(aChannel));
  if (!http) {
    nsCOMPtr<nsIURI> uri;
    aChannel->GetURI(getter_AddRefs(uri));
    uri->GetPrePath(aKey);
    return;
  }

  
  nsCString host;
  int32_t port = -1;

  NS_GetAuthHostPort(aChannel, aAuthInfo, true, host, &port);

  nsAutoString realm;
  aAuthInfo->GetRealm(realm);

  
  aKey.Append(host);
  aKey.Append(':');
  aKey.AppendInt(port);
  aKey.AppendLiteral(" (");
  AppendUTF16toUTF8(realm, aKey);
  aKey.Append(')');
}

#endif
