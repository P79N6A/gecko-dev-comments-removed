








































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

#include "jsapi.h"


#define LOAD_ERROR_NOSERVICE "Error creating IO Service."
#define LOAD_ERROR_NOCHANNEL "Error creating channel (invalid URL scheme?)"
#define LOAD_ERROR_NOSTREAM  "Error opening input stream (invalid filename?)"
#define LOAD_ERROR_NOCONTENT "ContentLength not available (not a local URL?)"
#define LOAD_ERROR_BADREAD   "File Read Error."
#define LOAD_ERROR_READUNDERFLOW "File Read Error (underflow.)"


extern void JS_DLL_CALLBACK
mozJSLoaderErrorReporter(JSContext *cx, const char *message, JSErrorReport *rep);

mozJSSubScriptLoader::mozJSSubScriptLoader() : mSystemPrincipal(nsnull)
{
}

mozJSSubScriptLoader::~mozJSSubScriptLoader()    
{
    
}

NS_IMPL_THREADSAFE_ISUPPORTS1(mozJSSubScriptLoader, mozIJSSubScriptLoader)

NS_IMETHODIMP 
mozJSSubScriptLoader::LoadSubScript (const PRUnichar * 
                                     )
{
    









    
    

    nsresult  rv;
    JSBool    ok;

    
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID());
    if (!xpc) return NS_ERROR_FAILURE;

    nsCOMPtr<nsIXPCNativeCallContext> cc;
    rv = xpc->GetCurrentNativeCallContext(getter_AddRefs(cc));
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
    
    char     *url;
    JSObject *target_obj = nsnull;
    ok = JS_ConvertArguments (cx, argc, argv, "s / o", &url, &target_obj);
    if (!ok)
    {
        cc->SetExceptionWasThrown (JS_TRUE);
        
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

    
    
    JSClass *target_class = JS_GET_CLASS(cx, target_obj);
    if (target_class->flags & JSCLASS_IS_EXTENDED)
    {
        JSExtendedClass *extended = (JSExtendedClass*)target_class;
        if (extended->innerObject)
        {
            target_obj = extended->innerObject(cx, target_obj);
            if (!target_obj) return NS_ERROR_FAILURE;
#ifdef DEBUG_rginda
            fprintf (stderr, "Final global: %p\n", target_obj);
#endif
        }
    }

    

    PRInt32   len = -1;
    PRUint32  readcount = 0;  
    PRUint32  lastReadCount = 0;  
    char     *buf = nsnull;
    
    JSString        *errmsg;
    JSErrorReporter  er;
    JSPrincipals    *jsPrincipals;
    
    nsCOMPtr<nsIChannel>     chan;
    nsCOMPtr<nsIInputStream> instream;

    nsCOMPtr<nsIIOService> serv = do_GetService(NS_IOSERVICE_CONTRACTID);
    if (!serv)
    {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOSERVICE);
        goto return_exception;
    }

    rv = serv->NewChannel(nsDependentCString(url), nsnull, static_cast<nsIURI *>(nsnull),
                          getter_AddRefs(chan));
    if (NS_FAILED(rv))
    {
        errmsg = JS_NewStringCopyZ (cx, LOAD_ERROR_NOCHANNEL);
        goto return_exception;
    }

    rv = chan->Open (getter_AddRefs(instream));
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
    if (NS_FAILED(rv) || !jsPrincipals) {
        delete[] buf;
        return rv;
    }

    

    er = JS_SetErrorReporter (cx, mozJSLoaderErrorReporter);

    ok = JS_EvaluateScriptForPrincipals (cx, target_obj, jsPrincipals,
                                         buf, len, url, 1, rval);        
    
    JS_SetErrorReporter (cx, er);

    cc->SetExceptionWasThrown (!ok);
    cc->SetReturnValueWasSet (ok);

    delete[] buf;
    JSPRINCIPALS_DROP(cx, jsPrincipals);
    return NS_OK;

 return_exception:
    if (buf)
        delete[] buf;

    JS_SetPendingException (cx, STRING_TO_JSVAL(errmsg));
    cc->SetExceptionWasThrown (JS_TRUE);
    return NS_OK;

}

#endif 
