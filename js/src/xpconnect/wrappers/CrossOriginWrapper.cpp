






































#include "CrossOriginWrapper.h"

#include "nsJSPrincipals.h"

#include "XPCWrapper.h"

namespace xpc {

CrossOriginWrapper::CrossOriginWrapper(uintN flags) : JSCrossCompartmentWrapper(flags)
{
}

CrossOriginWrapper::~CrossOriginWrapper()
{
}

static nsIPrincipal *
GetCompartmentPrincipal(JSCompartment *compartment)
{
    return static_cast<nsJSPrincipals *>(compartment->principals)->nsIPrincipalPtr;
}

bool
CrossOriginWrapper::enter(JSContext *cx, JSObject *wrapper, jsid id, Action act)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm) {
        return true;
    }
    JSStackFrame *fp = NULL;
    nsIPrincipal *principal = GetCompartmentPrincipal(wrappedObject(wrapper)->getCompartment());
    nsresult rv = ssm->PushContextPrincipal(cx, JS_FrameIterator(cx, &fp), principal);
    if (NS_FAILED(rv)) {
        NS_WARNING("Not allowing call because we're out of memory");
        JS_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

void
CrossOriginWrapper::leave(JSContext *cx, JSObject *wrapper)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (ssm) {
        ssm->PopContextPrincipal(cx);
    }
}

}
