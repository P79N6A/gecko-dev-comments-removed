





































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
  virtual bool RecvGetCookieString(const IPC::URI& aHost,
                                   const bool& aIsForeign,
                                   const bool& aFromHttp,
                                   nsCString* aResult);

  virtual bool RecvSetCookieString(const IPC::URI& aHost,
                                   const bool& aIsForeign,
                                   const nsCString& aCookieString,
                                   const nsCString& aServerTime,
                                   const bool& aFromHttp);

  nsRefPtr<nsCookieService> mCookieService;
};

}
}

#endif 

