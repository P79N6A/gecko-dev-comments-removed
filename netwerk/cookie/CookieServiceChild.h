




#ifndef mozilla_net_CookieServiceChild_h__
#define mozilla_net_CookieServiceChild_h__

#include "mozilla/net/PCookieServiceChild.h"
#include "nsICookieService.h"
#include "nsIObserver.h"
#include "nsIPrefBranch.h"
#include "mozIThirdPartyUtil.h"
#include "nsWeakReference.h"

namespace mozilla {
namespace net {

class CookieServiceChild : public PCookieServiceChild
                         , public nsICookieService
                         , public nsIObserver
                         , public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICOOKIESERVICE
  NS_DECL_NSIOBSERVER

  CookieServiceChild();

  static CookieServiceChild* GetSingleton();

protected:
  virtual ~CookieServiceChild();

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

  void PrefChanged(nsIPrefBranch *aPrefBranch);

  bool RequireThirdPartyCheck();

  nsCOMPtr<mozIThirdPartyUtil> mThirdPartyUtil;
  uint8_t mCookieBehavior;
  bool mThirdPartySession;
};

} 
} 

#endif 

