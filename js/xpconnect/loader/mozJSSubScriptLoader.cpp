






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
#include "xpcprivate.h" 

#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"

using namespace mozilla::scache;
using namespace JS;
using namespace xpc;

class MOZ_STACK_CLASS LoadSubScriptOptions : public OptionsBase {
public:
    LoadSubScriptOptions(JSContext *cx = xpc_GetSafeJSContext(),
                         JSObject *options = nullptr)
        : OptionsBase(cx, options)
        , target(cx)
        , charset(NullString())
        , ignoreCache(false)
    { }

    virtual bool Parse() {
      return ParseObject("target", &target) &&
             ParseString("charset", charset) &&
             ParseBoolean("ignoreCache", &ignoreCache);
    }

    RootedObject target;
    nsString charset;
    bool ignoreCache;
};



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
                                 const nsAString &charset, const char *uriStr,
                                 nsIIOService *serv, nsIPrincipal *principal,
                                 bool reuseGlobal, JSScript **scriptp,
                                 JSFunction **functionp)
{
    RootedObject target_obj(cx, targetObjArg);

    *scriptp = nullptr;
    *functionp = nullptr;

    
    
    nsCOMPtr<nsIChannel> chan;
    nsCOMPtr<nsIInputStream> instream;
    nsresult rv = NS_NewChannel(getter_AddRefs(chan), uri, serv,
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

    

    JSErrorReporter er = JS_SetErrorReporter(cx, xpc::SystemErrorReporter);

    JS::CompileOptions options(cx);
    options.setFileAndLine(uriStr, 1);
    if (!charset.IsVoid()) {
        nsString script;
        rv = nsScriptLoader::ConvertToUTF16(nullptr, reinterpret_cast<const uint8_t*>(buf.get()), len,
                                            charset, nullptr, script);

        if (NS_FAILED(rv)) {
            return ReportError(cx, LOAD_ERROR_BADCHARSET);
        }

        if (!reuseGlobal) {
            *scriptp = JS::Compile(cx, target_obj, options,
                                   script.get(),
                                   script.Length());
        } else {
            *functionp = JS::CompileFunction(cx, target_obj, options,
                                             nullptr, 0, nullptr,
                                             script.get(),
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
mozJSSubScriptLoader::LoadSubScript(const nsAString &url,
                                    HandleValue target,
                                    const nsAString &charset,
                                    JSContext *cx,
                                    MutableHandleValue retval)
{
    










    LoadSubScriptOptions options(cx);
    options.charset = charset;
    options.target = target.isObject() ? &target.toObject() : nullptr;
    return DoLoadSubScriptWithOptions(url, options, cx, retval);
}


NS_IMETHODIMP
mozJSSubScriptLoader::LoadSubScriptWithOptions(const nsAString &url,
                                               HandleValue optionsVal,
                                               JSContext *cx,
                                               MutableHandleValue retval)
{
    if (!optionsVal.isObject())
        return NS_ERROR_INVALID_ARG;
    LoadSubScriptOptions options(cx, &optionsVal.toObject());
    if (!options.Parse())
        return NS_ERROR_INVALID_ARG;
    return DoLoadSubScriptWithOptions(url, options, cx, retval);
}

nsresult
mozJSSubScriptLoader::DoLoadSubScriptWithOptions(const nsAString &url,
                                                 LoadSubScriptOptions &options,
                                                 JSContext *cx,
                                                 MutableHandleValue retval)
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
    mozJSComponentLoader *loader = mozJSComponentLoader::Get();
    rv = loader->FindTargetObject(cx, &targetObj);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    bool reusingGlobal = !JS_IsGlobalObject(targetObj);

    if (options.target)
        targetObj = options.target;

    
    
    nsCOMPtr<nsIPrincipal> principal = mSystemPrincipal;
    RootedObject result_obj(cx, targetObj);
    targetObj = JS_FindCompilationScope(cx, targetObj);
    if (!targetObj)
        return NS_ERROR_FAILURE;

    if (targetObj != result_obj)
        principal = GetObjectPrincipal(targetObj);

    JSAutoCompartment ac(cx, targetObj);

    

    nsCOMPtr<nsIURI> uri;
    nsAutoCString uriStr;
    nsAutoCString scheme;

    
    JS::AutoFilename filename;
    if (!JS::DescribeScriptedCaller(cx, &filename)) {
        
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

        
        
        nsAutoCString tmp(filename.get());
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
    RootedScript script(cx);
    if (cache && !options.ignoreCache)
        rv = ReadCachedScript(cache, cachePath, cx, mSystemPrincipal, &script);
    if (!script) {
        rv = ReadScript(uri, cx, targetObj, options.charset,
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


    bool ok = false;
    if (function) {
        ok = JS_CallFunction(cx, targetObj, function, JS::HandleValueArray::empty(),
                             retval);
    } else {
        ok = JS_ExecuteScriptVersion(cx, targetObj, script, retval.address(), version);
    }

    if (ok) {
        JSAutoCompartment rac(cx, result_obj);
        if (!JS_WrapValue(cx, retval))
            return NS_ERROR_UNEXPECTED;
    }

    if (cache && ok && writeScript) {
        WriteCachedScript(cache, cachePath, cx, mSystemPrincipal, script);
    }

    return NS_OK;
}
