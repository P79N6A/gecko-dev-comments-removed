





#include "ChromeObjectWrapper.h"
#include "WrapperFactory.h"
#include "AccessCheck.h"
#include "JavaScriptParent.h"
#include "xpcprivate.h"
#include "jsapi.h"
#include "jswrapper.h"
#include "nsXULAppAPI.h"

using namespace JS;

namespace xpc {











const ChromeObjectWrapper ChromeObjectWrapper::singleton;

using js::assertEnteredPolicy;

static bool
AllowedByBase(JSContext *cx, HandleObject wrapper, HandleId id,
              js::Wrapper::Action act)
{
    MOZ_ASSERT(js::Wrapper::wrapperHandler(wrapper) ==
               &ChromeObjectWrapper::singleton);
    bool bp;
    const ChromeObjectWrapper *handler = &ChromeObjectWrapper::singleton;
    return handler->ChromeObjectWrapperBase::enter(cx, wrapper, id, act, &bp);
}

static bool
PropIsFromStandardPrototype(JSContext *cx, JS::MutableHandle<JSPropertyDescriptor> desc)
{
    MOZ_ASSERT(desc.object());
    RootedObject unwrapped(cx, js::UncheckedUnwrap(desc.object()));
    JSAutoCompartment ac(cx, unwrapped);
    return IdentifyStandardPrototype(unwrapped) != JSProto_Null;
}






static bool
PropIsFromStandardPrototype(JSContext *cx, HandleObject wrapper,
                            HandleId id)
{
    MOZ_ASSERT(js::Wrapper::wrapperHandler(wrapper) ==
               &ChromeObjectWrapper::singleton);
    Rooted<JSPropertyDescriptor> desc(cx);
    const ChromeObjectWrapper *handler = &ChromeObjectWrapper::singleton;
    if (!handler->js::CrossCompartmentSecurityWrapper::getPropertyDescriptor(cx, wrapper, id,
                                                                             &desc) ||
        !desc.object())
    {
        return false;
    }
    return PropIsFromStandardPrototype(cx, &desc);
}

bool
ChromeObjectWrapper::getPropertyDescriptor(JSContext *cx,
                                           HandleObject wrapper,
                                           HandleId id,
                                           JS::MutableHandle<JSPropertyDescriptor> desc) const
{
    assertEnteredPolicy(cx, wrapper, id, GET | SET | GET_PROPERTY_DESCRIPTOR);
    
    desc.object().set(nullptr);
    if (AllowedByBase(cx, wrapper, id, Wrapper::GET) &&
        !ChromeObjectWrapperBase::getPropertyDescriptor(cx, wrapper, id,
                                                        desc)) {
        return false;
    }

    
    
    
    if (desc.object() && PropIsFromStandardPrototype(cx, desc))
        desc.object().set(nullptr);

    
    RootedObject wrapperProto(cx);
    if (!JS_GetPrototype(cx, wrapper, &wrapperProto))
      return false;
    if (desc.object() || !wrapperProto)
        return true;

    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    return JS_GetPropertyDescriptorById(cx, wrapperProto, id, desc);
}

static bool
CheckPassToChrome(JSContext *cx, HandleObject wrapper, HandleValue v)
{
    
    if (!v.isObject())
        return true;
    RootedObject obj(cx, &v.toObject());

    
    if (!js::IsWrapper(obj))
        return true;

    
    
    
    if (mozilla::jsipc::IsWrappedCPOW(obj) &&
        js::GetObjectCompartment(wrapper) == js::GetObjectCompartment(xpc::UnprivilegedJunkScope()) &&
        XRE_GetProcessType() == GeckoProcessType_Default)
    {
        return true;
    }

    
    
    
    if (WrapperFactory::IsCOW(obj)) {
        RootedObject target(cx, js::UncheckedUnwrap(obj));
        JSAutoCompartment ac(cx, target);
        RootedId id(cx, GetRTIdByIndex(cx, XPCJSRuntime::IDX_EXPOSEDPROPS));
        bool found = false;
        if (!JS_HasPropertyById(cx, target, id, &found))
            return false;
        if (found)
            return true;
    }

    
    if (AccessCheck::wrapperSubsumes(obj))
        return true;

    
    JS_ReportError(cx, "Permission denied to pass object to chrome");
    return false;
}

static bool
CheckPassToChrome(JSContext *cx, HandleObject wrapper, const CallArgs &args)
{
    if (!CheckPassToChrome(cx, wrapper, args.thisv()))
        return false;
    for (size_t i = 0; i < args.length(); ++i) {
        if (!CheckPassToChrome(cx, wrapper, args[i]))
            return false;
    }
    return true;
}

bool
ChromeObjectWrapper::defineProperty(JSContext *cx, HandleObject wrapper,
                                    HandleId id,
                                    MutableHandle<JSPropertyDescriptor> desc) const
{
    if (!CheckPassToChrome(cx, wrapper, desc.value()))
        return false;
    return ChromeObjectWrapperBase::defineProperty(cx, wrapper, id, desc);
}

bool
ChromeObjectWrapper::set(JSContext *cx, HandleObject wrapper,
                         HandleObject receiver, HandleId id,
                         bool strict, MutableHandleValue vp) const
{
    if (!CheckPassToChrome(cx, wrapper, vp))
        return false;
    return ChromeObjectWrapperBase::set(cx, wrapper, receiver, id, strict, vp);
}



bool
ChromeObjectWrapper::has(JSContext *cx, HandleObject wrapper,
                         HandleId id, bool *bp) const
{
    assertEnteredPolicy(cx, wrapper, id, GET);
    
    if (AllowedByBase(cx, wrapper, id, js::Wrapper::GET) &&
        !ChromeObjectWrapperBase::has(cx, wrapper, id, bp))
    {
        return false;
    }

    
    RootedObject wrapperProto(cx);
    if (!JS_GetPrototype(cx, wrapper, &wrapperProto))
        return false;
    if (*bp || !wrapperProto)
        return true;

    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    Rooted<JSPropertyDescriptor> desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, wrapperProto, id, &desc))
        return false;
    *bp = !!desc.object();
    return true;
}

bool
ChromeObjectWrapper::get(JSContext *cx, HandleObject wrapper,
                         HandleObject receiver, HandleId id,
                         MutableHandleValue vp) const
{
    assertEnteredPolicy(cx, wrapper, id, GET);
    vp.setUndefined();
    
    
    
    if (AllowedByBase(cx, wrapper, id, js::Wrapper::GET) &&
        !PropIsFromStandardPrototype(cx, wrapper, id))
    {
        
        if (!ChromeObjectWrapperBase::get(cx, wrapper, receiver, id, vp))
            return false;
        
        if (!vp.isUndefined())
            return true;
    }

    
    RootedObject wrapperProto(cx);
    if (!JS_GetPrototype(cx, wrapper, &wrapperProto))
        return false;
    if (!wrapperProto)
        return true;

    
    MOZ_ASSERT(js::IsObjectInContextCompartment(wrapper, cx));
    return js::GetGeneric(cx, wrapperProto, receiver, id, vp.address());
}

bool
ChromeObjectWrapper::call(JSContext *cx, HandleObject wrapper,
                      const CallArgs &args) const
{
    if (!CheckPassToChrome(cx, wrapper, args))
        return false;
    return ChromeObjectWrapperBase::call(cx, wrapper, args);
}

bool
ChromeObjectWrapper::construct(JSContext *cx, HandleObject wrapper,
                               const CallArgs &args) const
{
    if (!CheckPassToChrome(cx, wrapper, args))
        return false;
    return ChromeObjectWrapperBase::construct(cx, wrapper, args);
}




bool
ChromeObjectWrapper::objectClassIs(HandleObject obj, js::ESClassValue classValue,
                                   JSContext *cx) const
{
  return CrossCompartmentWrapper::objectClassIs(obj, classValue, cx);
}





bool
ChromeObjectWrapper::enter(JSContext *cx, HandleObject wrapper,
                           HandleId id, js::Wrapper::Action act, bool *bp) const
{
    if (AllowedByBase(cx, wrapper, id, act))
        return true;
    
    
    *bp = act == Wrapper::GET || act == Wrapper::ENUMERATE ||
          act == Wrapper::GET_PROPERTY_DESCRIPTOR;
    if (!*bp || id == JSID_VOID)
        return false;

    
    
    js::AutoWaivePolicy policy(cx, wrapper, id, act);
    return PropIsFromStandardPrototype(cx, wrapper, id);
}

}
