




#ifdef MOZ_LOGGING

#define FORCE_PR_LOG
#endif
#include "jsapi.h"
#include "nsIXPCSecurityManager.h"
#include "nsIXPConnect.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsString.h"
#include "nsIPrefService.h"
#include "nsIJSContextStack.h"
#include "nspr.h"
#include "mozilla/Attributes.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsJSPrincipals.h"
#include "jswrapper.h"

extern PRLogModuleInfo *MCD;
using mozilla::SafeAutoJSContext;



static JSObject *autoconfigSb = nullptr;

nsresult CentralizedAdminPrefManagerInit()
{
    nsresult rv;

    
    if (autoconfigSb)
        return NS_OK;

    
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rv);
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    nsCOMPtr<nsIPrincipal> principal;
    nsContentUtils::GetSecurityManager()->GetSystemPrincipal(getter_AddRefs(principal));


    
    SafeAutoJSContext cx;
    JSAutoRequest ar(cx);
    nsCOMPtr<nsIXPConnectJSObjectHolder> sandbox;
    rv = xpc->CreateSandbox(cx, principal, getter_AddRefs(sandbox));
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = sandbox->GetJSObject(&autoconfigSb);
    NS_ENSURE_SUCCESS(rv, rv);
    autoconfigSb = js::UnwrapObject(autoconfigSb);
    JSAutoCompartment ac(cx, autoconfigSb);
    if (!JS_AddNamedObjectRoot(cx, &autoconfigSb, "AutoConfig Sandbox"))
        return NS_ERROR_FAILURE;

    return NS_OK;
}

nsresult CentralizedAdminPrefManagerFinish()
{
    if (autoconfigSb) {
        SafeAutoJSContext cx;
        JSAutoRequest ar(cx);
        JSAutoCompartment(cx, autoconfigSb);
        JS_RemoveObjectRoot(cx, &autoconfigSb);
        JS_MaybeGC(cx);
    }
    return NS_OK;
}

nsresult EvaluateAdminConfigScript(const char *js_buffer, size_t length,
                                   const char *filename, bool bGlobalContext, 
                                   bool bCallbacks, bool skipFirstLine)
{
    nsresult rv = NS_OK;

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

    
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rv);
    if (NS_FAILED(rv)) {
        return rv;
    }

    SafeAutoJSContext cx;
    JSAutoRequest ar(cx);
    JSAutoCompartment ac(cx, autoconfigSb);

    nsAutoCString script(js_buffer, length);
    JS::RootedValue v(cx);
    rv = xpc->EvalInSandboxObject(NS_ConvertASCIItoUTF16(script), filename, cx, autoconfigSb,
                                   false, v.address());
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

