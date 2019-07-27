




#include "jsapi.h"
#include "nsIXPConnect.h"
#include "nsIJSRuntimeService.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsString.h"
#include "nsIPrefService.h"
#include "nspr.h"
#include "mozilla/Attributes.h"
#include "mozilla/Maybe.h"
#include "nsContentUtils.h"
#include "nsIScriptSecurityManager.h"
#include "nsJSPrincipals.h"
#include "jswrapper.h"

extern PRLogModuleInfo *MCD;
using mozilla::AutoSafeJSContext;



static JS::PersistentRooted<JSObject *> autoconfigSb;

nsresult CentralizedAdminPrefManagerInit()
{
    nsresult rv;

    
    if (autoconfigSb.initialized())
        return NS_OK;

    
    nsCOMPtr<nsIXPConnect> xpc = do_GetService(nsIXPConnect::GetCID(), &rv);
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    nsCOMPtr<nsIPrincipal> principal;
    nsContentUtils::GetSecurityManager()->GetSystemPrincipal(getter_AddRefs(principal));


    
    AutoSafeJSContext cx;
    nsCOMPtr<nsIXPConnectJSObjectHolder> sandbox;
    rv = xpc->CreateSandbox(cx, principal, getter_AddRefs(sandbox));
    NS_ENSURE_SUCCESS(rv, rv);

    
    NS_ENSURE_STATE(sandbox->GetJSObject());
    autoconfigSb.init(cx, js::UncheckedUnwrap(sandbox->GetJSObject()));

    return NS_OK;
}

nsresult CentralizedAdminPrefManagerFinish()
{
    if (autoconfigSb.initialized()) {
        AutoSafeJSContext cx;
        autoconfigSb.reset();
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

    AutoSafeJSContext cx;
    JSAutoCompartment ac(cx, autoconfigSb);

    nsAutoCString script(js_buffer, length);
    JS::RootedValue v(cx);
    rv = xpc->EvalInSandboxObject(NS_ConvertUTF8toUTF16(script), filename, cx,
                                  autoconfigSb, &v);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

