




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

 void
ChromeUtils::OriginAttributesToSuffix(dom::GlobalObject& aGlobal,
                                      const dom::OriginAttributesDictionary& aAttrs,
                                      nsCString& aSuffix)

{
  OriginAttributes attrs(aAttrs);
  attrs.CreateSuffix(aSuffix);
}
} 
} 
