




































#include "nsAutoPtr.h"
#include "nsScriptLoader.h"

#include "jsapi.h"
#include "jsdbgapi.h"
#include "jsxdrapi.h"

#include "nsJSPrincipals.h"

#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"

using namespace mozilla::scache;




nsresult
ReadCachedScript(StartupCache* cache, nsACString &uri, JSContext *cx,
                 nsIPrincipal *systemPrincipal, JSScript **script)
{
    nsAutoArrayPtr<char> buf;
    PRUint32 len;
    nsresult rv = cache->GetBuffer(PromiseFlatCString(uri).get(),
                                   getter_Transfers(buf), &len);
    if (NS_FAILED(rv)) {
        return rv; 
    }

    JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_DECODE);
    if (!xdr) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    ::JS_XDRMemSetData(xdr, buf, len);
    ::JS_XDRSetPrincipals(xdr, nsJSPrincipals::get(systemPrincipal), nsnull);

    JSBool ok = ::JS_XDRScript(xdr, script);
    
    
    ::JS_XDRMemSetData(xdr, NULL, 0);
    ::JS_XDRDestroy(xdr);

    return ok ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
WriteCachedScript(StartupCache* cache, nsACString &uri, JSContext *cx,
                  nsIPrincipal *systemPrincipal, JSScript *script)
{
    MOZ_ASSERT(JS_GetScriptPrincipals(script) == nsJSPrincipals::get(systemPrincipal));
    MOZ_ASSERT(JS_GetScriptOriginPrincipals(script) == nsJSPrincipals::get(systemPrincipal));

    JSXDRState *xdr = ::JS_XDRNewMem(cx, JSXDR_ENCODE);
    if (!xdr) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    nsresult rv;
    if (!::JS_XDRScript(xdr, &script)) {
        rv = NS_ERROR_OUT_OF_MEMORY;
    } else {
        uint32_t size;
        char* data = static_cast<char *>(::JS_XDRMemGetData(xdr, &size));
        MOZ_ASSERT(size);
        rv = cache->PutBuffer(PromiseFlatCString(uri).get(), data, size);
    }

    ::JS_XDRDestroy(xdr);
    return rv;
}
