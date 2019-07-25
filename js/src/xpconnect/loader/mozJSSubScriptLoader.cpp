









































#if !defined(XPCONNECT_STANDALONE) && !defined(NO_SUBSCRIPT_LOADER)

#include "mozJSSubScriptLoader.h"

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

    
    if (!mSystemPrincipal)
    {
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
    if (!ok)
    {
        
        return NS_OK;
    }

    JSAutoByteString urlbytes(cx, url);
    if (!urlbytes)
    {
        return NS_OK;
    }

    if (!target_obj)
    {
        


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
        while (maybe_glob != nsnull)
        {
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

    
    
    JSObject *result_obj = target_obj;
    target_obj = JS_FindCompilationScope(cx, target_obj);
    if (!target_obj) return NS_ERROR_FAILURE;
#ifdef DEBUG_rginda
    if (target_obj != result_obj)
        fprintf (stderr, "Final global: %p\n", target_obj);
#endif

    JSAutoEnterCompartment ac;
    if (!ac.enter(cx, target_obj))
        return NS_ERROR_UNEXPECTED;

    

    PRInt32   len = -1;
    PRUint32  readcount = 0;  
    PRUint32  lastReadCount = 0;  
    nsAutoArrayPtr<char> buf;
    
    JSString        *errmsg;
    JSErrorReporter  er;
    JSPrincipals    *jsPrincipals;
    
    nsCOMPtr<nsIChannel>     chan;
    nsCOMPtr<nsIInputStream> instream;
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

    if (!script)
    {
        

        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIIOService> serv = do_GetService(NS_IOSERVICE_CONTRACTID);
    if (!serv)
    {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOSERVICE);
        goto return_exception;
    }

    
    
    rv = NS_NewURI(getter_AddRefs(uri), urlbytes.ptr(), nsnull, serv);
    if (NS_FAILED(rv)) {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOURI);
        goto return_exception;
    }

    rv = uri->GetSpec(uriStr);
    if (NS_FAILED(rv)) {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOSPEC);
        goto return_exception;
    }    

    rv = uri->GetScheme(scheme);
    if (NS_FAILED(rv))
    {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOSCHEME);
        goto return_exception;
    }

    if (!scheme.EqualsLiteral("chrome"))
    {
        
        nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(uri);
        nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(innerURI);
        if (!fileURL)
        {
            errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_URI_NOT_LOCAL);
            goto return_exception;
        }

        
        
        nsCAutoString tmp(JS_GetScriptFilename(cx, script));
        tmp.AppendLiteral(" -> ");
        tmp.Append(uriStr);

        uriStr = tmp;
    }

    
    
    rv = NS_NewChannel(getter_AddRefs(chan), uri, serv,
                       nsnull, nsnull, nsIRequest::LOAD_NORMAL);
    if (NS_SUCCEEDED(rv))
    {
        chan->SetContentType(NS_LITERAL_CSTRING("application/javascript"));
        rv = chan->Open(getter_AddRefs(instream));
    }

    if (NS_FAILED(rv))
    {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOSTREAM);
        goto return_exception;
    }
    
    rv = chan->GetContentLength (&len);
    if (NS_FAILED(rv) || len == -1)
    {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOCONTENT);
        goto return_exception;
    }

    buf = new char[len + 1];
    if (!buf)
        return NS_ERROR_OUT_OF_MEMORY;
    buf[len] = '\0';
    
    do {
        rv = instream->Read (buf + readcount, len - readcount, &lastReadCount);
        if (NS_FAILED(rv))
        {
            errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_BADREAD);
            goto return_exception;
        }
        readcount += lastReadCount;
    } while (lastReadCount && readcount != PRUint32(len));
    
    if (static_cast<PRUint32>(len) != readcount)
    {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_READUNDERFLOW);
        goto return_exception;
    }

    


    rv = mSystemPrincipal->GetJSPrincipals(cx, &jsPrincipals);
    if (NS_FAILED(rv) || !jsPrincipals)
    {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOPRINCIPALS);
        goto return_exception;
    }

    

    er = JS_SetErrorReporter (cx, mozJSLoaderErrorReporter);

    if (charset)
    {
        nsString script;
        rv = nsScriptLoader::ConvertToUTF16(
                nsnull, reinterpret_cast<PRUint8*>(buf.get()), len,
                nsDependentString(reinterpret_cast<PRUnichar*>(charset)), nsnull, script);

        if (NS_FAILED(rv))
        {
            JSPRINCIPALS_DROP(cx, jsPrincipals);
            errmsg = JS_NewStringCopyZ(cx, LOAD_ERROR_BADCHARSET);
            goto return_exception;
        }
        ok = JS_EvaluateUCScriptForPrincipals(cx, target_obj, jsPrincipals,
                                              reinterpret_cast<const jschar*>(script.get()),
                                              script.Length(), uriStr.get(), 1, rval);
    }
    else
    {
        ok = JS_EvaluateScriptForPrincipals(cx, target_obj, jsPrincipals,
                                            buf, len, uriStr.get(), 1, rval);
    }

    JSPRINCIPALS_DROP(cx, jsPrincipals);

    if (ok)
    {
        JSAutoEnterCompartment rac;

        if (!rac.enter(cx, result_obj) || !JS_WrapValue(cx, rval))
            return NS_ERROR_UNEXPECTED; 
    }

    
    JS_SetErrorReporter (cx, er);

    cc->SetReturnValueWasSet (ok);
    return NS_OK;

 return_exception:
    JS_SetPendingException (cx, STRING_TO_JSVAL(errmsg));
    return NS_OK;
}

#endif 
