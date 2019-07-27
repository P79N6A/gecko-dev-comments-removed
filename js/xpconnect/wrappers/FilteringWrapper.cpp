





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
        if (Policy::check(cx, wrapper, id, Wrapper::GET) || Policy::check(cx, wrapper, id, Wrapper::SET))
            props[w++].set(id);
        else if (JS_IsExceptionPending(cx))
            return false;
    }
    props.resize(w);
    return true;
}

template <typename Policy>
static bool
FilterPropertyDescriptor(JSContext *cx, HandleObject wrapper, HandleId id, JS::MutableHandle<JSPropertyDescriptor> desc)
{
    MOZ_ASSERT(!JS_IsExceptionPending(cx));
    bool getAllowed = Policy::check(cx, wrapper, id, Wrapper::GET);
    if (JS_IsExceptionPending(cx))
        return false;
    bool setAllowed = Policy::check(cx, wrapper, id, Wrapper::SET);
    if (JS_IsExceptionPending(cx))
        return false;

    MOZ_ASSERT(getAllowed || setAllowed,
               "Filtering policy should not allow GET_PROPERTY_DESCRIPTOR in this case");

    if (!desc.hasGetterOrSetter()) {
        
        if (!getAllowed)
            desc.value().setUndefined();
    } else {
        
        MOZ_ASSERT(desc.value().isUndefined());
        if (!getAllowed)
            desc.setGetter(nullptr);
        if (!setAllowed)
            desc.setSetter(nullptr);
    }

    return true;
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::getPropertyDescriptor(JSContext *cx, HandleObject wrapper,
                                                      HandleId id,
                                                      JS::MutableHandle<JSPropertyDescriptor> desc) const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::GET | BaseProxyHandler::SET |
                                         BaseProxyHandler::GET_PROPERTY_DESCRIPTOR);
    if (!Base::getPropertyDescriptor(cx, wrapper, id, desc))
        return false;
    return FilterPropertyDescriptor<Policy>(cx, wrapper, id, desc);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::getOwnPropertyDescriptor(JSContext *cx, HandleObject wrapper,
                                                         HandleId id,
                                                         JS::MutableHandle<JSPropertyDescriptor> desc) const
{
    assertEnteredPolicy(cx, wrapper, id, BaseProxyHandler::GET | BaseProxyHandler::SET |
                                         BaseProxyHandler::GET_PROPERTY_DESCRIPTOR);
    if (!Base::getOwnPropertyDescriptor(cx, wrapper, id, desc))
        return false;
    return FilterPropertyDescriptor<Policy>(cx, wrapper, id, desc);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::getOwnPropertyNames(JSContext *cx, HandleObject wrapper,
                                                    AutoIdVector &props) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    return Base::getOwnPropertyNames(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enumerate(JSContext *cx, HandleObject wrapper,
                                          AutoIdVector &props) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    return Base::enumerate(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::keys(JSContext *cx, HandleObject wrapper,
                                     AutoIdVector &props) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    return Base::keys(cx, wrapper, props) &&
           Filter<Policy>(cx, wrapper, props);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::iterate(JSContext *cx, HandleObject wrapper,
                                        unsigned flags, MutableHandleValue vp) const
{
    assertEnteredPolicy(cx, wrapper, JSID_VOID, BaseProxyHandler::ENUMERATE);
    
    
    
    
    return js::BaseProxyHandler::iterate(cx, wrapper, flags, vp);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::nativeCall(JSContext *cx, JS::IsAcceptableThis test,
                                           JS::NativeImpl impl, JS::CallArgs args) const
{
    if (Policy::allowNativeCall(cx, test, impl))
        return Base::Permissive::nativeCall(cx, test, impl, args);
    return Base::Restrictive::nativeCall(cx, test, impl, args);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::defaultValue(JSContext *cx, HandleObject obj,
                                             JSType hint, MutableHandleValue vp) const
{
    return Base::defaultValue(cx, obj, hint, vp);
}

template <typename Base, typename Policy>
bool
FilteringWrapper<Base, Policy>::enter(JSContext *cx, HandleObject wrapper,
                                      HandleId id, Wrapper::Action act, bool *bp) const
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

CrossOriginXrayWrapper::CrossOriginXrayWrapper(unsigned flags) : SecurityXrayDOM(flags)
{
}

CrossOriginXrayWrapper::~CrossOriginXrayWrapper()
{
}

bool
CrossOriginXrayWrapper::getPropertyDescriptor(JSContext *cx,
                                              JS::Handle<JSObject*> wrapper,
                                              JS::Handle<jsid> id,
                                              JS::MutableHandle<JSPropertyDescriptor> desc) const
{
    if (!SecurityXrayDOM::getPropertyDescriptor(cx, wrapper, id, desc))
        return false;
    if (desc.object()) {
        
        desc.object().set(wrapper);

        
        
        desc.attributesRef() &= ~JSPROP_ENUMERATE;
        desc.attributesRef() &= ~JSPROP_PERMANENT;
        if (!desc.getter() && !desc.setter())
            desc.attributesRef() |= JSPROP_READONLY;
    }
    return true;
}

bool
CrossOriginXrayWrapper::getOwnPropertyDescriptor(JSContext *cx,
                                                 JS::Handle<JSObject*> wrapper,
                                                 JS::Handle<jsid> id,
                                                 JS::MutableHandle<JSPropertyDescriptor> desc) const
{
    
    return getPropertyDescriptor(cx, wrapper, id, desc);
}

bool
CrossOriginXrayWrapper::getPrototypeOf(JSContext *cx, JS::HandleObject wrapper,
                                       JS::MutableHandleObject protop) const
{
    
    protop.set(nullptr);
    return true;
}

bool
CrossOriginXrayWrapper::getOwnPropertyNames(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                            JS::AutoIdVector &props) const
{
    
    
    
    return SecurityXrayDOM::enumerate(cx, wrapper, JSITER_HIDDEN, props);
}

bool
CrossOriginXrayWrapper::defineProperty(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                       JS::Handle<jsid> id,
                                       JS::MutableHandle<JSPropertyDescriptor> desc) const
{
    
    
    if (XrayUtils::IsXrayResolving(cx, wrapper, id))
        return SecurityXrayDOM::defineProperty(cx, wrapper, id, desc);

    JS_ReportError(cx, "Permission denied to define property on cross-origin object");
    return false;
}

bool
CrossOriginXrayWrapper::delete_(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                JS::Handle<jsid> id, bool *bp) const
{
    JS_ReportError(cx, "Permission denied to delete property on cross-origin object");
    return false;
}

bool
CrossOriginXrayWrapper::enumerate(JSContext *cx, JS::Handle<JSObject*> wrapper,
                                  JS::AutoIdVector &props) const
{
    
    return true;
}

#define XOW FilteringWrapper<CrossOriginXrayWrapper, CrossOriginAccessiblePropertiesOnly>
#define NNXOW FilteringWrapper<CrossCompartmentSecurityWrapper, Opaque>
#define NNXOWC FilteringWrapper<CrossCompartmentSecurityWrapper, OpaqueWithCall>

template<> const XOW XOW::singleton(0);
template<> const NNXOW NNXOW::singleton(0);
template<> const NNXOWC NNXOWC::singleton(0);

template class XOW;
template class NNXOW;
template class NNXOWC;
template class ChromeObjectWrapperBase;
}
