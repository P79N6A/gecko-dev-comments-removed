





#ifndef mozJSLoaderUtils_h
#define mozJSLoaderUtils_h

#include "nsString.h"
#include "jsapi.h"

class nsIURI;
namespace mozilla {
namespace scache {
class StartupCache;
}
}

nsresult
ReadCachedScript(mozilla::scache::StartupCache* cache, nsACString &uri,
                 JSContext *cx, nsIPrincipal *systemPrincipal,
                 JSScript **script);

nsresult
WriteCachedScript(mozilla::scache::StartupCache* cache, nsACString &uri,
                  JSContext *cx, nsIPrincipal *systemPrincipal,
                  JSScript *script);

#endif 
