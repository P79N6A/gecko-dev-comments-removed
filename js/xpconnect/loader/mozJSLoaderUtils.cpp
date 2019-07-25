



#include "nsAutoPtr.h"
#include "nsScriptLoader.h"

#include "jsapi.h"
#include "jsdbgapi.h"

#include "nsJSPrincipals.h"

#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"

using namespace mozilla::scache;




nsresult
ReadCachedScript(StartupCache* cache, nsACString &uri, JSContext *cx,
                 nsIPrincipal *systemPrincipal, JSScript **scriptp)
{
    nsAutoArrayPtr<char> buf;
    uint32_t len;
    nsresult rv = cache->GetBuffer(PromiseFlatCString(uri).get(),
                                   getter_Transfers(buf), &len);
    if (NS_FAILED(rv))
        return rv; 

    JSScript *script = JS_DecodeScript(cx, buf, len, nsJSPrincipals::get(systemPrincipal), nullptr);
    if (!script)
        return NS_ERROR_OUT_OF_MEMORY;
    *scriptp = script;
    return NS_OK;
}

nsresult
WriteCachedScript(StartupCache* cache, nsACString &uri, JSContext *cx,
                  nsIPrincipal *systemPrincipal, JSScript *script)
{
    MOZ_ASSERT(JS_GetScriptPrincipals(script) == nsJSPrincipals::get(systemPrincipal));
    MOZ_ASSERT(JS_GetScriptOriginPrincipals(script) == nsJSPrincipals::get(systemPrincipal));

    uint32_t size;
    void *data = JS_EncodeScript(cx, script, &size);
    if (!data)
        return NS_ERROR_OUT_OF_MEMORY;

    MOZ_ASSERT(size);
    nsresult rv = cache->PutBuffer(PromiseFlatCString(uri).get(), static_cast<char *>(data), size);
    js_free(data);
    return rv;
}
