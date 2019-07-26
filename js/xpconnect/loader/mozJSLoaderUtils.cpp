



#include "nsAutoPtr.h"

#include "jsapi.h"
#include "js/OldDebugAPI.h"

#include "nsJSPrincipals.h"

#include "mozilla/scache/StartupCache.h"

using namespace JS;
using namespace mozilla::scache;




nsresult
ReadCachedScript(StartupCache* cache, nsACString &uri, JSContext *cx,
                 nsIPrincipal *systemPrincipal, MutableHandleScript scriptp)
{
    nsAutoArrayPtr<char> buf;
    uint32_t len;
    nsresult rv = cache->GetBuffer(PromiseFlatCString(uri).get(),
                                   getter_Transfers(buf), &len);
    if (NS_FAILED(rv))
        return rv; 

    scriptp.set(JS_DecodeScript(cx, buf, len, nsJSPrincipals::get(systemPrincipal), nullptr));
    if (!scriptp)
        return NS_ERROR_OUT_OF_MEMORY;
    return NS_OK;
}

nsresult
ReadCachedFunction(StartupCache* cache, nsACString &uri, JSContext *cx,
                   nsIPrincipal *systemPrincipal, JSFunction **functionp)
{
    return NS_ERROR_FAILURE;














}

nsresult
WriteCachedScript(StartupCache* cache, nsACString &uri, JSContext *cx,
                  nsIPrincipal *systemPrincipal, HandleScript script)
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

nsresult
WriteCachedFunction(StartupCache* cache, nsACString &uri, JSContext *cx,
                    nsIPrincipal *systemPrincipal, JSFunction *function)
{
    return NS_ERROR_FAILURE;











}
