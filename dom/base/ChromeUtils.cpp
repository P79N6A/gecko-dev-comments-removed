




#include "ChromeUtils.h"

#include "mozilla/BasePrincipal.h"

namespace mozilla {
namespace dom {

 void
ChromeUtils::OriginAttributesToCookieJar(GlobalObject& aGlobal,
                                         const OriginAttributesDictionary& aAttrs,
                                         nsCString& aCookieJar)
{
  OriginAttributes attrs(aAttrs);
  attrs.CookieJar(aCookieJar);
}

} 
} 
