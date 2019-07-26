




#ifndef mozilla_net_CookieServiceParent_h
#define mozilla_net_CookieServiceParent_h

#include "mozilla/net/PCookieServiceParent.h"
#include "SerializedLoadContext.h"

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
  virtual bool RecvGetCookieString(const URIParams& aHost,
                                   const bool& aIsForeign,
                                   const bool& aFromHttp,
                                   const IPC::SerializedLoadContext&
                                         loadContext,
                                   nsCString* aResult);

  virtual bool RecvSetCookieString(const URIParams& aHost,
                                   const bool& aIsForeign,
                                   const nsCString& aCookieString,
                                   const nsCString& aServerTime,
                                   const bool& aFromHttp,
                                   const IPC::SerializedLoadContext&
                                         loadContext);

  nsRefPtr<nsCookieService> mCookieService;
};

}
}

#endif 

