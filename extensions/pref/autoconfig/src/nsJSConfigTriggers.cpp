





































#ifdef MOZ_LOGGING

#define FORCE_PR_LOG
#endif
#include "jsapi.h"
#include "nsIXPCSecurityManager.h"
#include "nsIXPConnect.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"
#include "nsIGenericFactory.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsString.h"
#include "nsIPrefService.h"
#include "nsIJSContextStack.h"
#include "nspr.h"

extern PRLogModuleInfo *MCD;




class AutoConfigSecMan : public nsIXPCSecurityManager
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIXPCSECURITYMANAGER
    AutoConfigSecMan();
};

NS_IMPL_ISUPPORTS1(AutoConfigSecMan, nsIXPCSecurityManager)

AutoConfigSecMan::AutoConfigSecMan()
{
}

NS_IMETHODIMP
AutoConfigSecMan::CanCreateWrapper(JSContext *aJSContext, const nsIID & aIID, 
                                  nsISupports *aObj, nsIClassInfo *aClassInfo, 
                                  void **aPolicy)
{
    return NS_OK;
}

NS_IMETHODIMP
AutoConfigSecMan::CanCreateInstance(JSContext *aJSContext, const nsCID & aCID)
{
    return NS_OK;
}

NS_IMETHODIMP
AutoConfigSecMan::CanGetService(JSContext *aJSContext, const nsCID & aCID)
{
    return NS_OK;
}

NS_IMETHODIMP 
AutoConfigSecMan::CanAccess(PRUint32 aAction, 
                            nsIXPCNativeCallContext *aCallContext, 
                            JSContext *aJSContext, JSObject *aJSObject, 
                            nsISupports *aObj, nsIClassInfo *aClassInfo, 
                            jsval aName, void **aPolicy)
{
    return NS_OK;
}



static  JSContext *autoconfig_cx = nsnull;
static  JSObject *autoconfig_glob;

static JSClass global_class = {
    "autoconfig_global", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub
};

JS_STATIC_DLL_CALLBACK(void)
autoConfigErrorReporter(JSContext *cx, const char *message, 
                        JSErrorReport *report)
{
    NS_ERROR(message);
    PR_LOG(MCD, PR_LOG_DEBUG, ("JS error in js from MCD server: %s\n", message));
} 

nsresult CentralizedAdminPrefManagerInit()
{
    nsresult rv;
    JSRuntime *rt;

    
    if (autoconfig_cx) 
        return NS_OK;

    
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rv);
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    nsCOMPtr<nsIJSRuntimeService> rtsvc = 
        do_GetService("@mozilla.org/js/xpc/RuntimeService;1", &rv);
    if (NS_SUCCEEDED(rv))
        rv = rtsvc->GetRuntime(&rt);

    if (NS_FAILED(rv)) {
        NS_ERROR("Couldn't get JS RunTime");
        return rv;
    }

    
    autoconfig_cx = JS_NewContext(rt, 1024);
    if (!autoconfig_cx)
        return NS_ERROR_OUT_OF_MEMORY;

    JS_SetErrorReporter(autoconfig_cx, autoConfigErrorReporter);

    
    nsCOMPtr<nsIXPCSecurityManager> secman =
        static_cast<nsIXPCSecurityManager*>(new AutoConfigSecMan());
    xpc->SetSecurityManagerForJSContext(autoconfig_cx, secman, 0);

    autoconfig_glob = JS_NewObject(autoconfig_cx, &global_class, NULL, NULL);
    if (autoconfig_glob) {
        if (JS_InitStandardClasses(autoconfig_cx, autoconfig_glob)) {
            
            rv = xpc->InitClasses(autoconfig_cx, autoconfig_glob);
            if (NS_SUCCEEDED(rv)) 
                return NS_OK;
        }
    }

    
    JS_DestroyContext(autoconfig_cx);
    autoconfig_cx = nsnull;
    return NS_ERROR_FAILURE;
}

nsresult CentralizedAdminPrefManagerFinish()
{
    if (autoconfig_cx)
        JS_DestroyContext(autoconfig_cx);
    return NS_OK;
}

nsresult EvaluateAdminConfigScript(const char *js_buffer, size_t length,
                                   const char *filename, PRBool bGlobalContext, 
                                   PRBool bCallbacks, PRBool skipFirstLine)
{
    JSBool ok;
    jsval result;

    if (skipFirstLine) {
        




        unsigned int i = 0;
        while (i < length) {
            char c = js_buffer[i++];
            if (c == '\r') {
                if (js_buffer[i] == '\n')
                    i++;
                break;
            }
            if (c == '\n')
                break;
        }

        length -= i;
        js_buffer += i;
    }

    nsresult rv;
    nsCOMPtr<nsIJSContextStack> cxstack = 
        do_GetService("@mozilla.org/js/xpc/ContextStack;1");
    rv = cxstack->Push(autoconfig_cx);
    if (NS_FAILED(rv)) {
        NS_ERROR("coudn't push the context on the stack");
        return rv;
    }

    JS_BeginRequest(autoconfig_cx);
    ok = JS_EvaluateScript(autoconfig_cx, autoconfig_glob,
                           js_buffer, length, filename, 0, &result);
    JS_EndRequest(autoconfig_cx);

    JS_MaybeGC(autoconfig_cx);

    JSContext *cx;
    cxstack->Pop(&cx);
    NS_ASSERTION(cx == autoconfig_cx, "AutoConfig JS contexts didn't match");

    if (ok)
        return NS_OK;
    return NS_ERROR_FAILURE;
}

