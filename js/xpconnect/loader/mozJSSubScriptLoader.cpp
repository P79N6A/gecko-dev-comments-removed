









































#include "mozJSSubScriptLoader.h"
#include "mozJSLoaderUtils.h"

#include "nsIServiceManager.h"
#include "nsIXPConnect.h"

#include "nsIURI.h"
#include "nsIIOService.h"
#include "nsIChannel.h"
#include "nsIInputStream.h"
#include "nsNetCID.h"
#include "nsDependentString.h"
#include "nsAutoPtr.h"
#include "nsNetUtil.h"
#include "nsIProtocolHandler.h"
#include "nsIFileURL.h"
#include "nsScriptLoader.h"

#include "jsapi.h"
#include "jsdbgapi.h"
#include "jsfriendapi.h"

#include "mozilla/FunctionTimer.h"
#include "mozilla/scache/StartupCache.h"
#include "mozilla/scache/StartupCacheUtils.h"

using namespace mozilla::scache;


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


extern void
mozJSLoaderErrorReporter(JSContext *cx, const char *message, JSErrorReport *rep);

mozJSSubScriptLoader::mozJSSubScriptLoader() : mSystemPrincipal(nsnull)
{
}

mozJSSubScriptLoader::~mozJSSubScriptLoader()
{
    
}

NS_IMPL_THREADSAFE_ISUPPORTS1(mozJSSubScriptLoader, mozIJSSubScriptLoader)

static nsresult
ReportError(JSContext *cx, const char *msg)
{
    JS_SetPendingException(cx, STRING_TO_JSVAL(JS_NewStringCopyZ(cx, msg)));
    return NS_OK;
}

nsresult
mozJSSubScriptLoader::ReadScript(nsIURI *uri, JSContext *cx, JSObject *target_obj,
                                 jschar *charset, const char *uriStr,
                                 nsIIOService *serv, nsIPrincipal *principal,
                                 JSScript **scriptp)
{
    nsCOMPtr<nsIChannel>     chan;
    nsCOMPtr<nsIInputStream> instream;
    JSPrincipals    *jsPrincipals;
    JSErrorReporter  er;

    nsresult rv;
    
    
    rv = NS_NewChannel(getter_AddRefs(chan), uri, serv,
                       nsnull, nsnull, nsIRequest::LOAD_NORMAL);
    if (NS_SUCCEEDED(rv)) {
        chan->SetContentType(NS_LITERAL_CSTRING("application/javascript"));
        rv = chan->Open(getter_AddRefs(instream));
    }

    if (NS_FAILED(rv)) {
        return ReportError(cx, LOAD_ERROR_NOSTREAM);
    }

    PRInt32 len = -1;

    rv = chan->GetContentLength(&len);
    if (NS_FAILED(rv) || len == -1) {
        return ReportError(cx, LOAD_ERROR_NOCONTENT);
    }

    nsCString buf;
    rv = NS_ReadInputStreamToString(instream, buf, len);
    if (NS_FAILED(rv))
        return rv;

    


    rv = principal->GetJSPrincipals(cx, &jsPrincipals);
    if (NS_FAILED(rv) || !jsPrincipals) {
        return ReportError(cx, LOAD_ERROR_NOPRINCIPALS);
    }

    

    er = JS_SetErrorReporter(cx, mozJSLoaderErrorReporter);

    if (charset) {
        nsString script;
        rv = nsScriptLoader::ConvertToUTF16(nsnull, reinterpret_cast<const PRUint8*>(buf.get()), len,
                                            nsDependentString(reinterpret_cast<PRUnichar*>(charset)), nsnull, script);

        if (NS_FAILED(rv)) {
            JSPRINCIPALS_DROP(cx, jsPrincipals);
            return ReportError(cx, LOAD_ERROR_BADCHARSET);
        }

        *scriptp =
            JS_CompileUCScriptForPrincipals(cx, target_obj, jsPrincipals,
                                            reinterpret_cast<const jschar*>(script.get()),
                                            script.Length(), uriStr, 1);
    } else {
        *scriptp = JS_CompileScriptForPrincipals(cx, target_obj, jsPrincipals, buf.get(),
                                                 len, uriStr, 1);
    }

    JSPRINCIPALS_DROP(cx, jsPrincipals);

    
    JS_SetErrorReporter(cx, er);

    return NS_OK;
}

NS_IMETHODIMP 
mozJSSubScriptLoader::LoadSubScript (const PRUnichar * aURL
                                     )
{
    










    

    nsresult  rv;
    JSBool    ok;

#ifdef NS_FUNCTION_TIMER
    NS_TIME_FUNCTION_FMT("%s (line %d) (url: %s)", MOZ_FUNCTION_NAME,
                         __LINE__, NS_LossyConvertUTF16toASCII(aURL).get());
#else
    (void)aURL; 
#endif

    
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
    if (!xpc) return NS_ERROR_FAILURE;

    nsAXPCNativeCallContext *cc = nsnull;
    rv = xpc->GetCurrentNativeCallContext(&cc);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    JSContext *cx;
    rv = cc->GetJSContext (&cx);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    PRUint32 argc;
    rv = cc->GetArgc (&argc);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    jsval *argv;
    rv = cc->GetArgvPtr (&argv);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    jsval *rval;
    rv = cc->GetRetValPtr (&rval);
    if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

    
    if (!mSystemPrincipal) {
        nsCOMPtr<nsIScriptSecurityManager> secman =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
        if (!secman)
            return rv;

        rv = secman->GetSystemPrincipal(getter_AddRefs(mSystemPrincipal));
        if (NS_FAILED(rv) || !mSystemPrincipal)
            return rv;
    }

    JSAutoRequest ar(cx);

    JSString *url;
    JSObject *target_obj = nsnull;
    jschar   *charset = nsnull;
    ok = JS_ConvertArguments (cx, argc, argv, "S / o W", &url, &target_obj, &charset);
    if (!ok) {
        
        return NS_OK;
    }

    JSAutoByteString urlbytes(cx, url);
    if (!urlbytes) {
        return NS_OK;
    }

    if (!target_obj) {
        


#ifdef DEBUG_rginda
        JSObject *got_glob = JS_GetGlobalObject (cx);
        fprintf (stderr, "JS_GetGlobalObject says glob is %p.\n", got_glob);
        target_obj = JS_GetPrototype (cx, got_glob);
        fprintf (stderr, "That glob's prototype is %p.\n", target_obj);
        target_obj = JS_GetParent (cx, got_glob);
        fprintf (stderr, "That glob's parent is %p.\n", target_obj);
#endif

        nsCOMPtr<nsIXPConnectWrappedNative> wn;
        rv = cc->GetCalleeWrapper (getter_AddRefs(wn));
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

        rv = wn->GetJSObject (&target_obj);
        if (NS_FAILED(rv)) return NS_ERROR_FAILURE;

#ifdef DEBUG_rginda
        fprintf (stderr, "Parent chain: %p", target_obj);
#endif
        JSObject *maybe_glob = JS_GetParent (cx, target_obj);
        while (maybe_glob != nsnull) {
#ifdef DEBUG_rginda
            fprintf (stderr, ", %p", maybe_glob);
#endif
            target_obj = maybe_glob;
            maybe_glob = JS_GetParent (cx, maybe_glob);
        }
#ifdef DEBUG_rginda
        fprintf (stderr, "\n");
#endif
    }

    
    
    nsCOMPtr<nsIPrincipal> principal = mSystemPrincipal;
    JSObject *result_obj = target_obj;
    target_obj = JS_FindCompilationScope(cx, target_obj);
    if (!target_obj)
        return NS_ERROR_FAILURE;

    if (target_obj != result_obj) {
        nsCOMPtr<nsIScriptSecurityManager> secman =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
        if (!secman)
            return NS_ERROR_FAILURE;

        rv = secman->GetObjectPrincipal(cx, target_obj, getter_AddRefs(principal));
        NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG_rginda
        fprintf (stderr, "Final global: %p\n", target_obj);
#endif
    }

    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, target_obj))
        return NS_ERROR_UNEXPECTED;

    

    nsCOMPtr<nsIURI> uri;
    nsCAutoString uriStr;
    nsCAutoString scheme;

    JSStackFrame* frame = nsnull;
    JSScript* script = nsnull;

    
    do
    {
        frame = JS_FrameIterator(cx, &frame);

        if (frame)
            script = JS_GetFrameScript(cx, frame);
    } while (frame && !script);

    if (!script) {
        

        return NS_ERROR_FAILURE;
    }

    
    StartupCache* cache = (principal == mSystemPrincipal)
                          ? StartupCache::GetSingleton()
                          : nsnull;
    nsCOMPtr<nsIIOService> serv = do_GetService(NS_IOSERVICE_CONTRACTID);
    if (!serv) {
        return ReportError(cx, LOAD_ERROR_NOSERVICE);
    }

    
    
    rv = NS_NewURI(getter_AddRefs(uri), urlbytes.ptr(), nsnull, serv);
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

        
        
        nsCAutoString tmp(JS_GetScriptFilename(cx, script));
        tmp.AppendLiteral(" -> ");
        tmp.Append(uriStr);

        uriStr = tmp;
    }

    bool writeScript = false;
    JSVersion version = JS_GetVersion(cx);
    nsCAutoString cachePath;
    cachePath.AppendPrintf("jssubloader/%d", version);
    PathifyURI(uri, cachePath);

    script = nsnull;
    if (cache)
        rv = ReadCachedScript(cache, cachePath, cx, &script);
    if (!script) {
        rv = ReadScript(uri, cx, target_obj, charset, (char *)uriStr.get(), serv,
                        principal, &script);
        writeScript = true;
    }

    if (NS_FAILED(rv) || !script)
        return rv;

    ok = JS_ExecuteScriptVersion(cx, target_obj, script, rval, version);

    if (ok) {
        JSAutoEnterCompartment rac;
        if (!rac.enter(cx, result_obj) || !JS_WrapValue(cx, rval))
            return NS_ERROR_UNEXPECTED;
    }

    if (cache && ok && writeScript) {
        WriteCachedScript(cache, cachePath, cx, script);
    }

    cc->SetReturnValueWasSet (ok);
    return NS_OK;
}

