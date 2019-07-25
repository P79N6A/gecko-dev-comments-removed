






































#include "nsJSPrincipals.h"

#include "XPCWrapper.h"

#include "CrossOriginWrapper.h"
#include "AccessCheck.h"
#include "WrapperFactory.h"

namespace xpc {

NoWaiverWrapper::NoWaiverWrapper(uintN flags) : CrossCompartmentWrapper(flags)
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

bool
CrossOriginWrapper::getPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc)
{
    return CrossCompartmentWrapper::getPropertyDescriptor(cx, wrapper, id, set, desc) &&
           WrapperFactory::WaiveXrayAndWrap(cx, &desc->value);
}

bool
CrossOriginWrapper::getOwnPropertyDescriptor(JSContext *cx, JSObject *wrapper, jsid id,
                                          bool set, js::PropertyDescriptor *desc)
{
    return CrossCompartmentWrapper::getOwnPropertyDescriptor(cx, wrapper, id, set, desc) &&
           WrapperFactory::WaiveXrayAndWrap(cx, &desc->value);
}

bool
CrossOriginWrapper::get(JSContext *cx, JSObject *wrapper, JSObject *receiver, jsid id,
                        js::Value *vp)
{
    return CrossCompartmentWrapper::get(cx, wrapper, receiver, id, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, vp);
}

bool
CrossOriginWrapper::call(JSContext *cx, JSObject *wrapper, uintN argc, js::Value *vp)
{
    return CrossCompartmentWrapper::call(cx, wrapper, argc, vp) &&
           WrapperFactory::WaiveXrayAndWrap(cx, vp);
}

bool
CrossOriginWrapper::construct(JSContext *cx, JSObject *wrapper,
                              uintN argc, js::Value *argv, js::Value *rval)
{
    return CrossCompartmentWrapper::construct(cx, wrapper, argc, argv, rval) &&
           WrapperFactory::WaiveXrayAndWrap(cx, rval);
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
    nsIPrincipal *principal = GetCompartmentPrincipal(js::GetObjectCompartment(wrappedObject(wrapper)));
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
