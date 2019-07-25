






































#include "jsapi.h"
#include "jswrapper.h"

#include "XPCWrapper.h"

#include "nsJSPrincipals.h"

#include "AccessCheck.h"

namespace xpc {

nsIPrincipal *
GetCompartmentPrincipal(JSCompartment *compartment)
{
    return static_cast<nsJSPrincipals *>(compartment->principals)->nsIPrincipalPtr;
}

bool
AccessCheck::isPrivileged(JSCompartment *compartment)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if(!ssm) {
        return true;
    }

    PRBool privileged;
    if(NS_SUCCEEDED(ssm->IsSystemPrincipal(GetCompartmentPrincipal(compartment), &privileged))
       && privileged) {
        return true;
    }
    if(NS_SUCCEEDED(ssm->IsCapabilityEnabled("UniversalXPConnect", &privileged)) && privileged) {
        return true;
    }
    return false;
}

void
AccessCheck::deny(JSContext *cx, jsid id)
{
    if(id == JSID_VOID) {
        JS_ReportError(cx, "Permission denied to access object");
    } else {
        jsval idval;
        if (!JS_IdToValue(cx, id, &idval))
            return;
        JSString *str = JS_ValueToString(cx, idval);
        if (!str)
            return;
        JS_ReportError(cx, "Permission denied to access property '%hs'", str);
    }
}

bool
AccessCheck::enter(JSContext *cx, JSObject *wrapper, JSObject *wrappedObject, jsid id,
                   JSCrossCompartmentWrapper::Mode mode)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if(!ssm) {
        return true;
    }
    JSStackFrame *fp = NULL;
    nsresult rv = ssm->PushContextPrincipal(cx, JS_FrameIterator(cx, &fp),
                                            GetCompartmentPrincipal(wrappedObject->getCompartment(cx)));
    if(NS_FAILED(rv)) {
        NS_WARNING("Not allowing call because we're out of memory");
        JS_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

void
AccessCheck::leave(JSContext *cx, JSObject *wrapper, JSObject *wrappedObject)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if(ssm) {
        ssm->PopContextPrincipal(cx);
    }
}

}
