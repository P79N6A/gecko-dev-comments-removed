





#include "FilteringWrapper.h"
#include "AccessCheck.h"
#include "ChromeObjectWrapper.h"
#include "XrayWrapper.h"

#include "jsapi.h"

using namespace JS;
using namespace js;

namespace xpc {

template <typename Base, typename Policy>
FilteringWrapper<Base, Policy>::FilteringWrapper(unsigned flags) : Base(flags)
{
}

template <typename Base, typename Policy>
FilteringWrapper<Base, Policy>::~FilteringWrapper()
{
}

template <typename Policy>
static bool
Filter(JSContext *cx, HandleObject wrapper, AutoIdVector &props)
{
    size_t w = 0;
    RootedId id(cx);
    for (size_t n = 0; n < props.length(); ++n) {
        id = props[n];
        if (Policy::check(cx, wrapper, id, Wrapper::GET))
            props[w++].set(id);
        else if (JS_IsExceptionPending(cx))
            return false;
    }
    props.resize(w);
    return true;
}

template <typename Policy>
static bool
FilterSetter(JSContext *cx, JSObject *wrapper, jsid id, JS::MutableHandle<JSPropertyDescriptor> desc)
{
    bool setAllowed = Policy::check(cx, wrapper, id, Wrapper::SET);
    if (!setAllowed) {
        if (JS_IsExceptionPending(cx))
            return false;
        desc.setSetter(nullptr);
    }
    return true;
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::getPropertyDescriptor(JSContext *cx, HandleObject wrapper,
                                                      HandleId id,
                                                      JS::MutableHandle<JSPropertyDescriptor> desc)
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::GET | BaseProxyHandler::SET);
    if (!Base::getPropertyDescriptor(cx, wrapper, id, desc))
        return false;
    return FilterSetter<Policy>(cx, wrapper, id, desc);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper,
                                                         HandleId id,
                                                         JS::MutableHandle<JSPropertyDescriptor> desc)
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::GET | BaseProxyHandler::SET);
    if (!Base::getOwnPropertyDescriptor(cx, wrapper, id, desc))
        return false;
    return FilterSetter<Policy>(cx, wrapper, id, desc);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::getOwnPropertyNames(JSContext *cx, HandleObject wrapper,
                                                    AutoIdVector &props)
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    return Base::getOwnPropertyNames(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enumerate(JSContext *cx, HandleObject wrapper,
                                          AutoIdVector &props)
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    return Base::enumerate(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::keys(JSContext *cx, HandleObject wrapper,
                                     AutoIdVector &props)
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    return Base::keys(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::iterate(JSContext *cx, HandleObject wrapper,
                                        unsigned flags, MutableHandleValue vp)
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    
    
    
    
    return js::BaseProxyHandler::iterate(cx, wrapper, flags, vp);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::nativeCall(JSContext *cx, JS::IsAcceptableThis test,
                                           JS::NativeImpl impl, JS::CallArgs args)
{
    if (Policy::allowNativeCall(cx, test, impl))
        return Base::Permissive::nativeCall(cx, test, impl, args);
    return Base::Restrictive::nativeCall(cx, test, impl, args);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::defaultValue(JSContext *cx, HandleObject obj,
                                             JSType hint, MutableHandleValue vp)
{
    return Base::defaultValue(cx, obj, hint, vp);
}



template<>
bool
FilteringWrapper<CrossCompartmentSecurityWrapper, GentlyOpaque>
                ::defaultValue(JSContext *cx, HandleObject obj,
                               JSType hint, MutableHandleValue vp)
{
    JSString *str = JS_NewStringCopyZ(cx, "[Opaque]");
    if (!str)
        return false;
    vp.set(JS::StringValue(str));
    return true;
}


template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enter(JSContext *cx, HandleObject wrapper,
                                      HandleId id, Wrapper::Action act, bool *bp)
{
    
    
    
    
    
    
    
    
    
    
    
    
    if (XrayUtils::IsXrayResolving(cx, wrapper, id)) {
        *bp = true;
        return true;
    }
    if (!Policy::check(cx, wrapper, id, act)) {
        *bp = JS_IsExceptionPending(cx) ? false : Policy::deny(act, id);
        return false;
    }
    *bp = true;
    return true;
}

#define XOW FilteringWrapper<SecurityXrayXPCWN, CrossOriginAccessiblePropertiesOnly>
#define DXOW   FilteringWrapper<SecurityXrayDOM, CrossOriginAccessiblePropertiesOnly>
#define NNXOW FilteringWrapper<CrossCompartmentSecurityWrapper, Opaque>
#define NNXOWC FilteringWrapper<CrossCompartmentSecurityWrapper, OpaqueWithCall>
#define GO FilteringWrapper<CrossCompartmentSecurityWrapper, GentlyOpaque>
template<> XOW XOW::singleton(0);
template<> DXOW DXOW::singleton(0);
template<> NNXOW NNXOW::singleton(0);
template<> NNXOWC NNXOWC::singleton(0);

template<> GO GO::singleton(0);

template class XOW;
template class DXOW;
template class NNXOW;
template class NNXOWC;
template class ChromeObjectWrapperBase;
template class GO;
}
