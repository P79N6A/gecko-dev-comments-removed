





#ifndef mozJSLoaderUtils_h
#define mozJSLoaderUtils_h

#include "nsString.h"

namespace mozilla {
namespace scache {
class StartupCache;
}
}

nsresult
ReadCachedScript(mozilla::scache::StartupCache* cache, nsACString& uri,
                 JSContext* cx, nsIPrincipal* systemPrincipal,
                 JS::MutableHandleScript scriptp);

nsresult
ReadCachedFunction(mozilla::scache::StartupCache* cache, nsACString& uri,
                   JSContext* cx, nsIPrincipal* systemPrincipal,
                   JSFunction** function);

nsresult
WriteCachedScript(mozilla::scache::StartupCache* cache, nsACString& uri,
                  JSContext* cx, nsIPrincipal* systemPrincipal,
                  JS::HandleScript script);
nsresult
WriteCachedFunction(mozilla::scache::StartupCache* cache, nsACString& uri,
                    JSContext* cx, nsIPrincipal* systemPrincipal,
                    JSFunction* function);

#endif 
