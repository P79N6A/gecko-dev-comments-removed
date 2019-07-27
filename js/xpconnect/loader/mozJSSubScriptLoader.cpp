





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
#include "nsThreadUtils.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "nsJSPrincipals.h"
#include "xpcprivate.h" 
#include "jswrapper.h"

#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"
#include "mozilla/unused.h"
#include "nsContentUtils.h"
#include "nsStringGlue.h"

using namespace mozilla::scache;
using namespace JS;
using namespace xpc;
using namespace mozilla;

class MOZ_STACK_CLASS LoadSubScriptOptions : public OptionsBase {
public:
    explicit LoadSubScriptOptions(JSContext* cx = xpc_GetSafeJSContext(),
                                  JSObject* options = nullptr)
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
}

mozJSSubScriptLoader::~mozJSSubScriptLoader()
{
    
}

NS_IMPL_ISUPPORTS(mozJSSubScriptLoader, mozIJSSubScriptLoader)

static nsresult
ReportError(JSContext* cx, const char* msg)
{
    RootedValue exn(cx, JS::StringValue(JS_NewStringCopyZ(cx, msg)));
    JS_SetPendingException(cx, exn);
    return NS_OK;
}

static nsresult
ReportError(JSContext* cx, const char* origMsg, nsIURI* uri)
{
    if (!uri)
        return ReportError(cx, origMsg);

    nsAutoCString spec;
    nsresult rv = uri->GetSpec(spec);
    if (NS_FAILED(rv))
        spec.Assign("(unknown)");

    nsAutoCString msg(origMsg);
    msg.Append(": ");
    msg.Append(spec);
    return ReportError(cx, msg.get());
}

nsresult
mozJSSubScriptLoader::ReadScript(nsIURI* uri, JSContext* cx, JSObject* targetObjArg,
                                 const nsAString& charset, const char* uriStr,
                                 nsIIOService* serv, nsIPrincipal* principal,
                                 bool reuseGlobal, JS::MutableHandleScript script,
                                 JS::MutableHandleFunction function)
{
    RootedObject target_obj(cx, targetObjArg);

    script.set(nullptr);
    function.set(nullptr);

    
    
    nsCOMPtr<nsIChannel> chan;
    nsCOMPtr<nsIInputStream> instream;
    nsresult rv;
    rv = NS_NewChannel(getter_AddRefs(chan),
                       uri,
                       nsContentUtils::GetSystemPrincipal(),
                       nsILoadInfo::SEC_NORMAL,
                       nsIContentPolicy::TYPE_OTHER,
                       nullptr,  
                       nullptr,  
                       nsIRequest::LOAD_NORMAL,
                       serv);

    if (NS_SUCCEEDED(rv)) {
        chan->SetContentType(NS_LITERAL_CSTRING("application/javascript"));
        rv = chan->Open(getter_AddRefs(instream));
    }

    if (NS_FAILED(rv)) {
        return ReportError(cx, LOAD_ERROR_NOSTREAM, uri);
    }

    int64_t len = -1;

    rv = chan->GetContentLength(&len);
    if (NS_FAILED(rv) || len == -1) {
        return ReportError(cx, LOAD_ERROR_NOCONTENT, uri);
    }

    if (len > INT32_MAX) {
        return ReportError(cx, LOAD_ERROR_CONTENTTOOBIG, uri);
    }

    nsCString buf;
    rv = NS_ReadInputStreamToString(instream, buf, len);
    if (NS_FAILED(rv))
        return rv;

    JS::CompileOptions options(cx);
    options.setFileAndLine(uriStr, 1);
    if (!charset.IsVoid()) {
        char16_t* scriptBuf = nullptr;
        size_t scriptLength = 0;

        rv = nsScriptLoader::ConvertToUTF16(nullptr, reinterpret_cast<const uint8_t*>(buf.get()), len,
                                            charset, nullptr, scriptBuf, scriptLength);

        JS::SourceBufferHolder srcBuf(scriptBuf, scriptLength,
                                      JS::SourceBufferHolder::GiveOwnership);

        if (NS_FAILED(rv)) {
            return ReportError(cx, LOAD_ERROR_BADCHARSET, uri);
        }

        if (!reuseGlobal) {
            options.setHasPollutedScope(!JS_IsGlobalObject(target_obj));
            JS::Compile(cx, options, srcBuf, script);
        } else {
            AutoObjectVector scopeChain(cx);
            if (!JS_IsGlobalObject(target_obj) &&
                !scopeChain.append(target_obj)) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
            
            JS::CompileFunction(cx, scopeChain, options, nullptr, 0, nullptr,
                                srcBuf, function);
        }
    } else {
        
        
        if (!reuseGlobal) {
            options.setSourceIsLazy(true)
                   .setHasPollutedScope(!JS_IsGlobalObject(target_obj));
            JS::Compile(cx, options, buf.get(), len, script);
        } else {
            AutoObjectVector scopeChain(cx);
            if (!JS_IsGlobalObject(target_obj) &&
                !scopeChain.append(target_obj)) {
                return NS_ERROR_OUT_OF_MEMORY;
            }
            
            JS::CompileFunction(cx, scopeChain, options, nullptr, 0, nullptr,
                                buf.get(), len, function);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
mozJSSubScriptLoader::LoadSubScript(const nsAString& url,
                                    HandleValue target,
                                    const nsAString& charset,
                                    JSContext* cx,
                                    MutableHandleValue retval)
{
    










    LoadSubScriptOptions options(cx);
    options.charset = charset;
    options.target = target.isObject() ? &target.toObject() : nullptr;
    return DoLoadSubScriptWithOptions(url, options, cx, retval);
}


NS_IMETHODIMP
mozJSSubScriptLoader::LoadSubScriptWithOptions(const nsAString& url,
                                               HandleValue optionsVal,
                                               JSContext* cx,
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
mozJSSubScriptLoader::DoLoadSubScriptWithOptions(const nsAString& url,
                                                 LoadSubScriptOptions& options,
                                                 JSContext* cx,
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
    mozJSComponentLoader* loader = mozJSComponentLoader::Get();
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
        return ReportError(cx, LOAD_ERROR_NOSCHEME, uri);
    }

    if (!scheme.EqualsLiteral("chrome") && !scheme.EqualsLiteral("app")) {
        
        nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(uri);
        nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(innerURI);
        if (!fileURL) {
            return ReportError(cx, LOAD_ERROR_URI_NOT_LOCAL, uri);
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
                        principal, reusingGlobal, &script, &function);
        writeScript = !!script;
    }

    if (NS_FAILED(rv) || (!script && !function))
        return rv;

    if (function) {
        script = JS_GetFunctionScript(cx, function);
    }

    bool ok = false;
    if (function) {
        ok = JS_CallFunction(cx, targetObj, function, JS::HandleValueArray::empty(),
                             retval);
    } else {
        if (JS_IsGlobalObject(targetObj)) {
            ok = JS_ExecuteScript(cx, script, retval);
        } else {
            JS::AutoObjectVector scopeChain(cx);
            ok = scopeChain.append(targetObj) &&
                 JS_ExecuteScript(cx, scopeChain, script, retval);
        }
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





class ScriptPrecompiler : public nsIStreamLoaderObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMLOADEROBSERVER

    ScriptPrecompiler(nsIObserver* aObserver,
                      nsIPrincipal* aPrincipal,
                      nsIChannel* aChannel)
        : mObserver(aObserver)
        , mPrincipal(aPrincipal)
        , mChannel(aChannel)
        , mScriptBuf(nullptr)
        , mScriptLength(0)
    {}

    static void OffThreadCallback(void* aToken, void* aData);

    
    void SendObserverNotification();

private:
    virtual ~ScriptPrecompiler()
    {
      if (mScriptBuf) {
        js_free(mScriptBuf);
      }
    }

    nsRefPtr<nsIObserver> mObserver;
    nsRefPtr<nsIPrincipal> mPrincipal;
    nsRefPtr<nsIChannel> mChannel;
    char16_t* mScriptBuf;
    size_t mScriptLength;
};

NS_IMPL_ISUPPORTS(ScriptPrecompiler, nsIStreamLoaderObserver);

class NotifyPrecompilationCompleteRunnable : public nsRunnable
{
public:
    NS_DECL_NSIRUNNABLE

    explicit NotifyPrecompilationCompleteRunnable(ScriptPrecompiler* aPrecompiler)
        : mPrecompiler(aPrecompiler)
        , mToken(nullptr)
    {}

    void SetToken(void* aToken) {
        MOZ_ASSERT(aToken && !mToken);
        mToken = aToken;
    }

protected:
    nsRefPtr<ScriptPrecompiler> mPrecompiler;
    void* mToken;
};


class AutoSendObserverNotification {
public:
    explicit AutoSendObserverNotification(ScriptPrecompiler* aPrecompiler)
        : mPrecompiler(aPrecompiler)
    {}

    ~AutoSendObserverNotification() {
        if (mPrecompiler) {
            mPrecompiler->SendObserverNotification();
        }
    }

    void Disarm() {
        mPrecompiler = nullptr;
    }

private:
    ScriptPrecompiler* mPrecompiler;
};

NS_IMETHODIMP
NotifyPrecompilationCompleteRunnable::Run(void)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mPrecompiler);

    AutoSendObserverNotification notifier(mPrecompiler);

    if (mToken) {
        JSRuntime* rt = XPCJSRuntime::Get()->Runtime();
        NS_ENSURE_TRUE(rt, NS_ERROR_FAILURE);
        JS::FinishOffThreadScript(nullptr, rt, mToken);
    }

    return NS_OK;
}

NS_IMETHODIMP
ScriptPrecompiler::OnStreamComplete(nsIStreamLoader* aLoader,
                                    nsISupports* aContext,
                                    nsresult aStatus,
                                    uint32_t aLength,
                                    const uint8_t* aString)
{
    AutoSendObserverNotification notifier(this);

    
    NS_ENSURE_SUCCESS(aStatus, NS_OK);

    
    nsAutoString hintCharset;
    nsresult rv =
        nsScriptLoader::ConvertToUTF16(mChannel, aString, aLength,
                                       hintCharset, nullptr,
                                       mScriptBuf, mScriptLength);

    NS_ENSURE_SUCCESS(rv, NS_OK);

    
    
    
    
    AutoSafeJSContext cx;
    RootedValue v(cx);
    SandboxOptions sandboxOptions;
    sandboxOptions.sandboxName.AssignASCII("asm.js precompilation");
    sandboxOptions.invisibleToDebugger = true;
    sandboxOptions.discardSource = true;
    rv = CreateSandboxObject(cx, &v, mPrincipal, sandboxOptions);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    JSAutoCompartment ac(cx, js::UncheckedUnwrap(&v.toObject()));

    JS::CompileOptions options(cx, JSVERSION_DEFAULT);
    options.forceAsync = true;
    options.installedFile = true;

    nsCOMPtr<nsIURI> uri;
    mChannel->GetURI(getter_AddRefs(uri));
    nsAutoCString spec;
    uri->GetSpec(spec);
    options.setFile(spec.get());

    if (!JS::CanCompileOffThread(cx, options, mScriptLength)) {
        NS_WARNING("Can't compile script off thread!");
        return NS_OK;
    }

    nsRefPtr<NotifyPrecompilationCompleteRunnable> runnable =
        new NotifyPrecompilationCompleteRunnable(this);

    if (!JS::CompileOffThread(cx, options,
                              mScriptBuf, mScriptLength,
                              OffThreadCallback,
                              static_cast<void*>(runnable))) {
        NS_WARNING("Failed to compile script off thread!");
        return NS_OK;
    }

    unused << runnable.forget();
    notifier.Disarm();

    return NS_OK;
}


void
ScriptPrecompiler::OffThreadCallback(void* aToken, void* aData)
{
    nsRefPtr<NotifyPrecompilationCompleteRunnable> runnable =
        dont_AddRef(static_cast<NotifyPrecompilationCompleteRunnable*>(aData));
    runnable->SetToken(aToken);

    NS_DispatchToMainThread(runnable);
}

void
ScriptPrecompiler::SendObserverNotification()
{
    MOZ_ASSERT(mChannel && mObserver);
    MOZ_ASSERT(NS_IsMainThread());

    nsCOMPtr<nsIURI> uri;
    mChannel->GetURI(getter_AddRefs(uri));
    mObserver->Observe(uri, "script-precompiled", nullptr);
}

NS_IMETHODIMP
mozJSSubScriptLoader::PrecompileScript(nsIURI* aURI,
                                       nsIPrincipal* aPrincipal,
                                       nsIObserver* aObserver)
{
    nsCOMPtr<nsIChannel> channel;
    nsresult rv = NS_NewChannel(getter_AddRefs(channel),
                                aURI,
                                nsContentUtils::GetSystemPrincipal(),
                                nsILoadInfo::SEC_NORMAL,
                                nsIContentPolicy::TYPE_OTHER);

    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<ScriptPrecompiler> loadObserver =
        new ScriptPrecompiler(aObserver, aPrincipal, channel);

    nsCOMPtr<nsIStreamLoader> loader;
    rv = NS_NewStreamLoader(getter_AddRefs(loader), loadObserver);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStreamListener> listener = loader.get();
    rv = channel->AsyncOpen(listener, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}
