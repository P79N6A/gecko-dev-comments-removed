






#include "mozJSSubScriptLoader.h"
#include "mozJSComponentLoader.h"
#include "mozJSLoaderUtils.h"

#include "nsIURI.h"
#include "nsIIOService.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsNetCID.h"
#include "nsNetUtil.h"
#include "nsIFileURL.h"
#include "nsScriptLoader.h"
#include "nsIScriptSecurityManager.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "js/OldDebugAPI.h"
#include "nsJSPrincipals.h"
#include "xpcpublic.h" 

#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"

using namespace mozilla::scache;
using namespace JS;


#define LOAD_ERROR_NOSERVICE "Error creating IO Service."
#define LOAD_ERROR_NOURI "Error creating URI (invalid URL scheme?)"
#define LOAD_ERROR_NOSCHEME "Failed to get URI scheme.  This is bad."
#define LOAD_ERROR_URI_NOT_LOCAL "Trying to load a non-local URI."
#define LOAD_ERROR_NOSTREAM  "Error opening input stream (invalid filename?)"
#define LOAD_ERROR_NOCONTENT "ContentLength not available (not a local URL?)"
#define LOAD_ERROR_BADCHARSET "Error converting to specified charset"
#define LOAD_ERROR_BADREAD   "File Read Error."
#define LOAD_ERROR_READUNDERFLOW "File Read Error (underflow.)"
#define LOAD_ERROR_NOPRINCIPALS "Failed to get principals."
#define LOAD_ERROR_NOSPEC "Failed to get URI spec.  This is bad."
#define LOAD_ERROR_CONTENTTOOBIG "ContentLength is too large"

mozJSSubScriptLoader::mozJSSubScriptLoader() : mSystemPrincipal(nullptr)
{
    
    nsCOMPtr<xpcIJSModuleLoader> componentLoader =
        do_GetService(MOZJSCOMPONENTLOADER_CONTRACTID);
}

mozJSSubScriptLoader::~mozJSSubScriptLoader()
{
    
}

NS_IMPL_ISUPPORTS1(mozJSSubScriptLoader, mozIJSSubScriptLoader)

static nsresult
ReportError(JSContext *cx, const char *msg)
{
    RootedValue exn(cx, JS::StringValue(JS_NewStringCopyZ(cx, msg)));
    JS_SetPendingException(cx, exn);
    return NS_OK;
}

nsresult
mozJSSubScriptLoader::ReadScript(nsIURI *uri, JSContext *cx, JSObject *targetObjArg,
                                 const nsAString& charset, const char *uriStr,
                                 nsIIOService *serv, nsIPrincipal *principal,
                                 bool reuseGlobal, JSScript **scriptp,
                                 JSFunction **functionp)
{
    RootedObject target_obj(cx, targetObjArg);

    nsCOMPtr<nsIChannel>     chan;
    nsCOMPtr<nsIInputStream> instream;
    JSErrorReporter  er;

    *scriptp = nullptr;
    *functionp = nullptr;

    nsresult rv;
    
    
    rv = NS_NewChannel(getter_AddRefs(chan), uri, serv,
                       nullptr, nullptr, nsIRequest::LOAD_NORMAL);
    if (NS_SUCCEEDED(rv)) {
        chan->SetContentType(NS_LITERAL_CSTRING("application/javascript"));
        rv = chan->Open(getter_AddRefs(instream));
    }

    if (NS_FAILED(rv)) {
        return ReportError(cx, LOAD_ERROR_NOSTREAM);
    }

    int64_t len = -1;

    rv = chan->GetContentLength(&len);
    if (NS_FAILED(rv) || len == -1) {
        return ReportError(cx, LOAD_ERROR_NOCONTENT);
    }

    if (len > INT32_MAX) {
        return ReportError(cx, LOAD_ERROR_CONTENTTOOBIG);
    }

    nsCString buf;
    rv = NS_ReadInputStreamToString(instream, buf, len);
    if (NS_FAILED(rv))
        return rv;

    

    er = JS_SetErrorReporter(cx, xpc::SystemErrorReporter);

    JS::CompileOptions options(cx);
    options.setPrincipals(nsJSPrincipals::get(principal))
           .setFileAndLine(uriStr, 1);
    if (!charset.IsVoid()) {
        nsString script;
        rv = nsScriptLoader::ConvertToUTF16(nullptr, reinterpret_cast<const uint8_t*>(buf.get()), len,
                                            charset, nullptr, script);

        if (NS_FAILED(rv)) {
            return ReportError(cx, LOAD_ERROR_BADCHARSET);
        }

        if (!reuseGlobal) {
            *scriptp = JS::Compile(cx, target_obj, options,
                                   reinterpret_cast<const jschar*>(script.get()),
                                   script.Length());
        } else {
            *functionp = JS::CompileFunction(cx, target_obj, options,
                                             nullptr, 0, nullptr,
                                             reinterpret_cast<const jschar*>(script.get()),
                                             script.Length());
        }
    } else {
        
        
        if (!reuseGlobal) {
            options.setSourcePolicy(JS::CompileOptions::LAZY_SOURCE);
            *scriptp = JS::Compile(cx, target_obj, options, buf.get(), len);
        } else {
            *functionp = JS::CompileFunction(cx, target_obj, options,
                                             nullptr, 0, nullptr, buf.get(),
                                             len);
        }
    }

    
    JS_SetErrorReporter(cx, er);

    return NS_OK;
}

NS_IMETHODIMP
mozJSSubScriptLoader::LoadSubScript(const nsAString& url,
                                    const Value& targetArg,
                                    const nsAString& charset,
                                    JSContext* cx,
                                    Value* retval)
{
    










    nsresult rv = NS_OK;

    
    if (!mSystemPrincipal) {
        nsCOMPtr<nsIScriptSecurityManager> secman =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
        if (!secman)
            return NS_OK;

        rv = secman->GetSystemPrincipal(getter_AddRefs(mSystemPrincipal));
        if (NS_FAILED(rv) || !mSystemPrincipal)
            return rv;
    }

    RootedObject targetObj(cx);
    mozJSComponentLoader* loader = mozJSComponentLoader::Get();
    rv = loader->FindTargetObject(cx, &targetObj);
    NS_ENSURE_SUCCESS(rv, rv);

    bool reusingGlobal = !JS_IsGlobalObject(targetObj);

    
    
    RootedValue target(cx, targetArg);
    RootedObject passedObj(cx);
    if (!JS_ValueToObject(cx, target, &passedObj))
        return NS_ERROR_ILLEGAL_VALUE;

    if (passedObj)
        targetObj = passedObj;

    
    
    nsCOMPtr<nsIPrincipal> principal = mSystemPrincipal;
    RootedObject result_obj(cx, targetObj);
    targetObj = JS_FindCompilationScope(cx, targetObj);
    if (!targetObj)
        return NS_ERROR_FAILURE;

    if (targetObj != result_obj) {
        nsCOMPtr<nsIScriptSecurityManager> secman =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
        if (!secman)
            return NS_ERROR_FAILURE;

        rv = secman->GetObjectPrincipal(cx, targetObj, getter_AddRefs(principal));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    JSAutoCompartment ac(cx, targetObj);

    

    nsCOMPtr<nsIURI> uri;
    nsAutoCString uriStr;
    nsAutoCString scheme;

    RootedScript script(cx);

    
    if (!JS_DescribeScriptedCaller(cx, &script, nullptr)) {
        
        return NS_ERROR_FAILURE;
    }

    
    StartupCache* cache = (principal == mSystemPrincipal)
                          ? StartupCache::GetSingleton()
                          : nullptr;
    nsCOMPtr<nsIIOService> serv = do_GetService(NS_IOSERVICE_CONTRACTID);
    if (!serv) {
        return ReportError(cx, LOAD_ERROR_NOSERVICE);
    }

    
    
    rv = NS_NewURI(getter_AddRefs(uri), NS_LossyConvertUTF16toASCII(url).get(), nullptr, serv);
    if (NS_FAILED(rv)) {
        return ReportError(cx, LOAD_ERROR_NOURI);
    }

    rv = uri->GetSpec(uriStr);
    if (NS_FAILED(rv)) {
        return ReportError(cx, LOAD_ERROR_NOSPEC);
    }

    rv = uri->GetScheme(scheme);
    if (NS_FAILED(rv)) {
        return ReportError(cx, LOAD_ERROR_NOSCHEME);
    }

    if (!scheme.EqualsLiteral("chrome")) {
        
        nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(uri);
        nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(innerURI);
        if (!fileURL) {
            return ReportError(cx, LOAD_ERROR_URI_NOT_LOCAL);
        }

        
        
        nsAutoCString tmp(JS_GetScriptFilename(cx, script));
        tmp.AppendLiteral(" -> ");
        tmp.Append(uriStr);

        uriStr = tmp;
    }

    bool writeScript = false;
    JSVersion version = JS_GetVersion(cx);
    nsAutoCString cachePath;
    cachePath.AppendPrintf("jssubloader/%d", version);
    PathifyURI(uri, cachePath);

    RootedFunction function(cx);
    script = nullptr;
    if (cache)
        rv = ReadCachedScript(cache, cachePath, cx, mSystemPrincipal, &script);
    if (!script) {
        rv = ReadScript(uri, cx, targetObj, charset,
                        static_cast<const char*>(uriStr.get()), serv,
                        principal, reusingGlobal, script.address(), function.address());
        writeScript = !!script;
    }

    if (NS_FAILED(rv) || (!script && !function))
        return rv;

    if (function) {
        script = JS_GetFunctionScript(cx, function);
    }

    loader->NoteSubScript(script, targetObj);

    RootedValue rval(cx);
    bool ok = false;
    if (function) {
        ok = JS_CallFunction(cx, targetObj, function, 0, nullptr, rval.address());
    } else {
        ok = JS_ExecuteScriptVersion(cx, targetObj, script, rval.address(), version);
    }

    if (ok) {
        JSAutoCompartment rac(cx, result_obj);
        if (!JS_WrapValue(cx, &rval))
            return NS_ERROR_UNEXPECTED;
        *retval = rval;
    }

    if (cache && ok && writeScript) {
        WriteCachedScript(cache, cachePath, cx, mSystemPrincipal, script);
    }

    return NS_OK;
}
