




































#ifndef NSPROMPTUTILS_H_
#define NSPROMPTUTILS_H_

#include "nsIHttpChannel.h"












inline void
NS_SetAuthInfo(nsIAuthInformation* aAuthInfo, const nsString& user,
               const nsString& password)
{
  PRUint32 flags;
  aAuthInfo->GetFlags(&flags);
  if (flags & nsIAuthInformation::NEED_DOMAIN) {
    
    PRInt32 idx = user.FindChar(PRUnichar('\\'));
    if (idx == kNotFound) {
      aAuthInfo->SetUsername(user);
    } else {
      aAuthInfo->SetDomain(Substring(user, 0, idx));
      aAuthInfo->SetUsername(Substring(user, idx + 1));
    }
  } else {
    aAuthInfo->SetUsername(user);
  }
  aAuthInfo->SetPassword(password);
}














inline void
NS_GetAuthHostPort(nsIChannel* aChannel, nsIAuthInformation* aAuthInfo,
                   PRBool machineProcessing, nsCString& host, PRInt32* port)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = aChannel->GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return;

  
  PRUint32 flags;
  aAuthInfo->GetFlags(&flags);
  if (flags & nsIAuthInformation::AUTH_PROXY) {
    nsCOMPtr<nsIProxiedChannel> proxied(do_QueryInterface(aChannel));
    NS_ASSERTION(proxied, "proxy auth needs nsIProxiedChannel");

    nsCOMPtr<nsIProxyInfo> info;
    proxied->GetProxyInfo(getter_AddRefs(info));
    NS_ASSERTION(info, "proxy auth needs nsIProxyInfo");

    nsCAutoString idnhost;
    info->GetHost(idnhost);
    info->GetPort(port);

    if (machineProcessing) {
      nsCOMPtr<nsIIDNService> idnService =
        do_GetService(NS_IDNSERVICE_CONTRACTID);
      if (idnService) {
        idnService->ConvertUTF8toACE(idnhost, host);
      } else {
        
        host = idnhost;
      }
    } else {
      host = idnhost;
    }
  } else {
    if (machineProcessing) {
      uri->GetAsciiHost(host);
      *port = NS_GetRealPort(uri);
    } else {
      uri->GetHost(host);
      uri->GetPort(port);
    }
  }
}






inline void
NS_GetAuthKey(nsIChannel* aChannel, nsIAuthInformation* aAuthInfo,
              nsCString& key)
{
  
  nsCOMPtr<nsIHttpChannel> http(do_QueryInterface(aChannel));
  if (!http) {
    nsCOMPtr<nsIURI> uri;
    aChannel->GetURI(getter_AddRefs(uri));
    uri->GetPrePath(key);
    return;
  }

  
  nsCString host;
  PRInt32 port = -1;

  NS_GetAuthHostPort(aChannel, aAuthInfo, PR_TRUE, host, &port);

  nsAutoString realm;
  aAuthInfo->GetRealm(realm);
  
  
  key.Append(host);
  key.Append(':');
  key.AppendInt(port);
  key.AppendLiteral(" (");
  AppendUTF16toUTF8(realm, key);
  key.Append(')');
}

#endif

