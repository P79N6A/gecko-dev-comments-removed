






































#include "nsJSPrincipals.h"

#include "XPCWrapper.h"

#include "CrossOriginWrapper.h"
#include "WrapperFactory.h"

namespace xpc {

NoWaiverWrapper::NoWaiverWrapper(uintN flags) : JSCrossCompartmentWrapper(flags)
{
}

NoWaiverWrapper::~NoWaiverWrapper()
{
}

CrossOriginWrapper::CrossOriginWrapper(uintN flags) : NoWaiverWrapper(flags)
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
CrossOriginWrapper::getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc)
{
    return JSCrossCompartmentWrapper::getPropertyDescriptor(cx, wrapper, id, set, desc) &&
           WrapperFactory::WaiveXrayAndWrap(cx, js::Jsvalify(&desc->value));
}

bool
CrossOriginWrapper::getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc)
{
    return JSCrossCompartmentWrapper::getOwnPropertyDescriptor(cx, wrapper, id, set, desc) &&
           WrapperFactory::WaiveXrayAndWrap(cx, js::Jsvalify(&desc->value));
}

bool
CrossOriginWrapper::get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                        js::Value *vp)
{
    return JSCrossCompartmentWrapper::get(cx, wrapper, receiver, id, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, js::Jsvalify(vp));
}

bool
CrossOriginWrapper::call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp)
{
    return JSCrossCompartmentWrapper::call(cx, wrapper, argc, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, js::Jsvalify(vp));
}

bool
CrossOriginWrapper::construct(JSContext *cx, JSObject *wrapper,
                              uintN argc, js::Value *argv, js::Value *rval)
{
    return JSCrossCompartmentWrapper::construct(cx, wrapper, argc, argv, rval) &&
           WrapperFactory::WaiveXrayAndWrap(cx, js::Jsvalify(rval));
}

bool
NoWaiverWrapper::enter(JSContext *cx, JSObject *wrapper, jsid id, Action act, bool *bp)
{
    *bp = true; 
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (!ssm) {
        return true;
    }

    
    
    
    JSStackFrame *fp = NULL;
    nsIPrincipal *principal = GetCompartmentPrincipal(wrappedObject(wrapper)->compartment());
    nsresult rv = ssm->PushContextPrincipal(cx, JS_FrameIterator(cx, &fp), principal);
    if (NS_FAILED(rv)) {
        NS_WARNING("Not allowing call because we're out of memory");
        JS_ReportOutOfMemory(cx);
        return false;
    }
    return true;
}

void
NoWaiverWrapper::leave(JSContext *cx, JSObject *wrapper)
{
    nsIScriptSecurityManager *ssm = XPCWrapper::GetSecurityManager();
    if (ssm) {
        ssm->PopContextPrincipal(cx);
    }
}

}
