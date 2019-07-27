





#include "jsapi.h"
#include "jswrapper.h"

#include "jsatominlines.h"

using namespace js;

template <class Base>
bool
SecurityWrapper<Base>::isExtensible(JSContext *cx, HandleObject wrapper, bool *extensible) const
{
    
    
    
    *extensible = true;
    return true;
}

template <class Base>
bool
SecurityWrapper<Base>::preventExtensions(JSContext *cx, HandleObject wrapper) const
{
    
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::enter(JSContext *cx, HandleObject wrapper, HandleId id,
                             Wrapper::Action act, bool *bp) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    *bp = false;
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::nativeCall(JSContext *cx, IsAcceptableThis test, NativeImpl impl,
                                  CallArgs args) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::setPrototypeOf(JSContext *cx, HandleObject wrapper,
                                      HandleObject proto, bool *bp) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}




template <class Base>
bool
SecurityWrapper<Base>::defaultValue(JSContext *cx, HandleObject wrapper,
                                    JSType hint, MutableHandleValue vp) const
{
    return DefaultValue(cx, wrapper, hint, vp);
}

template <class Base>
bool
SecurityWrapper<Base>::objectClassIs(HandleObject obj, ESClassValue classValue, JSContext *cx) const
{
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::regexp_toShared(JSContext *cx, HandleObject obj, RegExpGuard *g) const
{
    return Base::regexp_toShared(cx, obj, g);
}

template <class Base>
bool
SecurityWrapper<Base>::boxedValue_unbox(JSContext *cx, HandleObject obj, MutableHandleValue vp) const
{
    vp.setUndefined();
    return true;
}

template <class Base>
bool
SecurityWrapper<Base>::defineProperty(JSContext *cx, HandleObject wrapper,
                                      HandleId id, MutableHandle<PropertyDescriptor> desc) const
{
    if (desc.getter() || desc.setter()) {
        JSString *str = IdToString(cx, id);
        AutoStableStringChars chars(cx);
        const char16_t *prop = nullptr;
        if (str->ensureFlat(cx) && chars.initTwoByte(cx, str))
            prop = chars.twoByteChars();
        JS_ReportErrorNumberUC(cx, js_GetErrorMessage, nullptr,
                               JSMSG_ACCESSOR_DEF_DENIED, prop);
        return false;
    }

    return Base::defineProperty(cx, wrapper, id, desc);
}

template <class Base>
bool
SecurityWrapper<Base>::watch(JSContext *cx, HandleObject proxy,
                             HandleId id, HandleObject callable) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}

template <class Base>
bool
SecurityWrapper<Base>::unwatch(JSContext *cx, HandleObject proxy,
                               HandleId id) const
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNWRAP_DENIED);
    return false;
}


template class js::SecurityWrapper<Wrapper>;
template class js::SecurityWrapper<CrossCompartmentWrapper>;
