#include "ChromeObjectWrapper.h"
#include "jsapi.h"

using namespace JS;

namespace xpc {











ChromeObjectWrapper ChromeObjectWrapper::singleton;

using js::assertEnteredPolicy;

static bool
AllowedByBase(JSContext *cx, HandleObject wrapper, HandleId id,
              js::Wrapper::Action act)
{
    MOZ_ASSERT(js::Wrapper::wrapperHandler(wrapper) ==
               &ChromeObjectWrapper::singleton);
    bool bp;
    ChromeObjectWrapper *handler = &ChromeObjectWrapper::singleton;
    return handler->ChromeObjectWrapperBase::enter(cx, wrapper, id, act, &bp);
}

static bool
PropIsFromStandardPrototype(JSContext *cx, JS::MutableHandle<JSPropertyDescriptor> desc)
{
    MOZ_ASSERT(desc.object());
    RootedObject unwrapped(cx, js::UncheckedUnwrap(desc.object()));
    JSAutoCompartment ac(cx, unwrapped);
    return JS_IdentifyClassPrototype(cx, unwrapped) != JSProto_Null;
}





static bool
PropIsFromStandardPrototype(JSContext *cx, HandleObject wrapper,
                            HandleId id)
{
    MOZ_ASSERT(js::Wrapper::wrapperHandler(wrapper) ==
               &ChromeObjectWrapper::singleton);
    Rooted<JSPropertyDescriptor> desc(cx);
    ChromeObjectWrapper *handler = &ChromeObjectWrapper::singleton;
    if (!handler->ChromeObjectWrapperBase::getPropertyDescriptor(cx, wrapper, id,
                                                                 &desc, 0) ||
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
                                           JS::MutableHandle<JSPropertyDescriptor> desc,
                                           unsigned flags)
{
    assertEnteredPolicy(cx, wrapper, id);
    
    desc.object().set(nullptr);
    if (AllowedByBase(cx, wrapper, id, Wrapper::GET) &&
        !ChromeObjectWrapperBase::getPropertyDescriptor(cx, wrapper, id,
                                                        desc, flags)) {
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
    return JS_GetPropertyDescriptorById(cx, wrapperProto, id, 0, desc);
}

bool
ChromeObjectWrapper::has(JSContext *cx, HandleObject wrapper,
                         HandleId id, bool *bp)
{
    assertEnteredPolicy(cx, wrapper, id);
    
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
    if (!JS_GetPropertyDescriptorById(cx, wrapperProto, id, 0, &desc))
        return false;
    *bp = !!desc.object();
    return true;
}

bool
ChromeObjectWrapper::get(JSContext *cx, HandleObject wrapper,
                         HandleObject receiver, HandleId id,
                         MutableHandleValue vp)
{
    assertEnteredPolicy(cx, wrapper, id);
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
ChromeObjectWrapper::objectClassIs(HandleObject obj, js::ESClassValue classValue,
                                   JSContext *cx)
{
  return CrossCompartmentWrapper::objectClassIs(obj, classValue, cx);
}





bool
ChromeObjectWrapper::enter(JSContext *cx, HandleObject wrapper,
                           HandleId id, js::Wrapper::Action act, bool *bp)
{
    if (AllowedByBase(cx, wrapper, id, act))
        return true;
    
    
    *bp = (act == Wrapper::GET);
    if (!*bp || id == JSID_VOID)
        return false;

    
    
    js::AutoWaivePolicy policy(cx, wrapper, id);
    return PropIsFromStandardPrototype(cx, wrapper, id);
}

}
