





































#ifndef mozilla_net_CookieServiceChild_h__
#define mozilla_net_CookieServiceChild_h__

#include "mozilla/net/PCookieServiceChild.h"
#include "nsICookieService.h"
#include "nsICookiePermission.h"

namespace mozilla {
namespace net {

class CookieServiceChild : public PCookieServiceChild
                         , public nsICookieService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOOKIESERVICE

  CookieServiceChild();
  virtual ~CookieServiceChild();

  static CookieServiceChild* GetSingleton();

protected:
  void SerializeURIs(nsIURI *aHostURI,
                     nsIChannel *aChannel,
                     nsCString &aHostSpec,
                     nsCString &aHostCharset,
                     nsCString &aOriginatingSpec,
                     nsCString &aOriginatingCharset);

  nsresult GetCookieStringInternal(nsIURI *aHostURI,
                                   nsIChannel *aChannel,
                                   char **aCookieString,
                                   bool aFromHttp);

  nsresult SetCookieStringInternal(nsIURI *aHostURI,
                                   nsIChannel *aChannel,
                                   const char *aCookieString,
                                   const char *aServerTime,
                                   bool aFromHttp);

  nsCOMPtr<nsICookiePermission> mPermissionService;
};

}
}

#endif 

