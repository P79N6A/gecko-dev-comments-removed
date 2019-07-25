





































#ifndef mozilla_net_CookieServiceParent_h
#define mozilla_net_CookieServiceParent_h

#include "mozilla/net/PCookieServiceParent.h"

class nsCookieService;
class nsIIOService;

namespace mozilla {
namespace net {

class CookieServiceParent : public PCookieServiceParent
{
public:
  CookieServiceParent();
  virtual ~CookieServiceParent();

protected:
  virtual bool RecvGetCookieString(const nsCString& aHostSpec,
                                   const nsCString& aHostCharset,
                                   const nsCString& aOriginatingSpec,
                                   const nsCString& aOriginatingCharset,
                                   const bool& aFromHttp,
                                   nsCString* aResult);

  virtual bool RecvSetCookieString(const nsCString& aHostSpec,
                                   const nsCString& aHostCharset,
                                   const nsCString& aOriginatingSpec,
                                   const nsCString& aOriginatingCharset,
                                   const nsCString& aCookieString,
                                   const nsCString& aServerTime,
                                   const bool& aFromHttp);

  nsRefPtr<nsCookieService> mCookieService;
  nsCOMPtr<nsIIOService> mIOService;
};

}
}

#endif 

