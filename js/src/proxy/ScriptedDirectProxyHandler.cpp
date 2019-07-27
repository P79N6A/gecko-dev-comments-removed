





#include "proxy/ScriptedDirectProxyHandler.h"

#include "jsapi.h"

#include "vm/PropDesc.h"

#include "jsobjinlines.h"

using namespace js;
using mozilla::ArrayLength;

static inline bool
IsDataDescriptor(const PropertyDescriptor &desc)
{
    return desc.obj && !(desc.attrs & (JSPROP_GETTER | JSPROP_SETTER));
}

static inline bool
IsAccessorDescriptor(const PropertyDescriptor &desc)
{
    return desc.obj && desc.attrs & (JSPROP_GETTER | JSPROP_SETTER);
}




static bool
ValidatePropertyDescriptor(JSContext *cx, bool extensible, Handle<PropDesc> desc,
                           Handle<PropertyDescriptor> current, bool *bp)
{
    
    if (!current.object()) {
        
        *bp = extensible;
        return true;
    }

    
    if (!desc.hasValue() && !desc.hasWritable() && !desc.hasGet() && !desc.hasSet() &&
        !desc.hasEnumerable() && !desc.hasConfigurable())
    {
        *bp = true;
        return true;
    }

    
    if ((!desc.hasWritable() || desc.writable() == !current.isReadonly()) &&
        (!desc.hasGet() || desc.getter() == current.getter()) &&
        (!desc.hasSet() || desc.setter() == current.setter()) &&
        (!desc.hasEnumerable() || desc.enumerable() == current.isEnumerable()) &&
        (!desc.hasConfigurable() || desc.configurable() == !current.isPermanent()))
    {
        if (!desc.hasValue()) {
            *bp = true;
            return true;
        }
        bool same = false;
        if (!SameValue(cx, desc.value(), current.value(), &same))
            return false;
        if (same) {
            *bp = true;
            return true;
        }
    }

    
    if (current.isPermanent()) {
        if (desc.hasConfigurable() && desc.configurable()) {
            *bp = false;
            return true;
        }

        if (desc.hasEnumerable() &&
            desc.enumerable() != current.isEnumerable())
        {
            *bp = false;
            return true;
        }
    }

    
    if (desc.isGenericDescriptor()) {
        *bp = true;
        return true;
    }

    
    if (IsDataDescriptor(current) != desc.isDataDescriptor()) {
        *bp = !current.isPermanent();
        return true;
    }

    
    if (IsDataDescriptor(current)) {
        JS_ASSERT(desc.isDataDescriptor()); 
        if (current.isPermanent() && current.isReadonly()) {
            if (desc.hasWritable() && desc.writable()) {
                *bp = false;
                return true;
            }

            if (desc.hasValue()) {
                bool same;
                if (!SameValue(cx, desc.value(), current.value(), &same))
                    return false;
                if (!same) {
                    *bp = false;
                    return true;
                }
            }
        }

        *bp = true;
        return true;
    }

    
    JS_ASSERT(IsAccessorDescriptor(current)); 
    JS_ASSERT(desc.isAccessorDescriptor()); 
    *bp = (!current.isPermanent() ||
           ((!desc.hasSet() || desc.setter() == current.setter()) &&
            (!desc.hasGet() || desc.getter() == current.getter())));
    return true;
}


static bool
IsSealed(JSContext* cx, HandleObject obj, HandleId id, bool *bp)
{
    
    Rooted<PropertyDescriptor> desc(cx);
    if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;

    
    *bp = desc.object() && desc.isPermanent();
    return true;
}

static bool
HasOwn(JSContext *cx, HandleObject obj, HandleId id, bool *bp)
{
    Rooted<PropertyDescriptor> desc(cx);
    if (!JS_GetPropertyDescriptorById(cx, obj, id, &desc))
        return false;
    *bp = (desc.object() == obj);
    return true;
}


static JSObject *
GetDirectProxyHandlerObject(JSObject *proxy)
{
    JS_ASSERT(proxy->as<ProxyObject>().handler() == &ScriptedDirectProxyHandler::singleton);
    return proxy->as<ProxyObject>().extra(ScriptedDirectProxyHandler::HANDLER_EXTRA).toObjectOrNull();
}

static inline void
ReportInvalidTrapResult(JSContext *cx, JSObject *proxy, JSAtom *atom)
{
    RootedValue v(cx, ObjectOrNullValue(proxy));
    JSAutoByteString bytes;
    if (!AtomToPrintableString(cx, atom, &bytes))
        return;
    js_ReportValueError2(cx, JSMSG_INVALID_TRAP_RESULT, JSDVG_IGNORE_STACK, v,
                         js::NullPtr(), bytes.ptr());
}


static bool
ArrayToIdVector(JSContext *cx, HandleObject proxy, HandleObject target, HandleValue v,
                AutoIdVector &props, unsigned flags, JSAtom *trapName_)
{
    JS_ASSERT(v.isObject());
    RootedObject array(cx, &v.toObject());
    RootedAtom trapName(cx, trapName_);

    
    uint32_t n;
    if (!GetLengthProperty(cx, array, &n))
        return false;

    
    for (uint32_t i = 0; i < n; ++i) {
        
        RootedValue v(cx);
        if (!JSObject::getElement(cx, array, array, i, &v))
            return false;

        
        RootedId id(cx);
        if (!ValueToId<CanGC>(cx, v, &id))
            return false;

        
        for (uint32_t j = 0; j < i; ++j) {
            if (props[j].get() == id) {
                ReportInvalidTrapResult(cx, proxy, trapName);
                return false;
            }
        }

        
        bool isFixed;
        if (!HasOwn(cx, target, id, &isFixed))
            return false;

        
        bool extensible;
        if (!JSObject::isExtensible(cx, target, &extensible))
            return false;
        if (!extensible && !isFixed) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_NEW);
            return false;
        }

        
        if (!props.append(id))
            return false;
    }

    
    AutoIdVector ownProps(cx);
    if (!GetPropertyNames(cx, target, flags, &ownProps))
        return false;

    
    for (size_t i = 0; i < ownProps.length(); ++i) {
        RootedId id(cx, ownProps[i]);

        bool found = false;
        for (size_t j = 0; j < props.length(); ++j) {
            if (props[j].get() == id) {
                found = true;
               break;
            }
        }
        if (found)
            continue;

        
        bool sealed;
        if (!IsSealed(cx, target, id, &sealed))
            return false;
        if (sealed) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_SKIP_NC);
            return false;
        }

        
        bool isFixed;
        if (!HasOwn(cx, target, id, &isFixed))
            return false;

        
        bool extensible;
        if (!JSObject::isExtensible(cx, target, &extensible))
            return false;
        if (!extensible && isFixed) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_E_AS_NE);
            return false;
        }
    }

    
    return true;
}


bool
ScriptedDirectProxyHandler::preventExtensions(JSContext *cx, HandleObject proxy) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().preventExtensions, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::preventExtensions(cx, proxy);

    
    Value argv[] = {
        ObjectValue(*target)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    bool success = ToBoolean(trapResult);
    if (success) {
        
        bool extensible;
        if (!JSObject::isExtensible(cx, target, &extensible))
            return false;
        if (extensible) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_AS_NON_EXTENSIBLE);
            return false;
        }
        
        return true;
    }

    
    
    
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CHANGE_EXTENSIBILITY);
    return false;
}



bool
ScriptedDirectProxyHandler::getPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                                  MutableHandle<PropertyDescriptor> desc) const
{
    JS_CHECK_RECURSION(cx, return false);

    if (!GetOwnPropertyDescriptor(cx, proxy, id, desc))
        return false;
    if (desc.object())
        return true;
    RootedObject proto(cx);
    if (!JSObject::getProto(cx, proxy, &proto))
        return false;
    if (!proto) {
        JS_ASSERT(!desc.object());
        return true;
    }
    return JS_GetPropertyDescriptorById(cx, proto, id, desc);
}


bool
ScriptedDirectProxyHandler::getOwnPropertyDescriptor(JSContext *cx, HandleObject proxy, HandleId id,
                                                     MutableHandle<PropertyDescriptor> desc) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().getOwnPropertyDescriptor, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::getOwnPropertyDescriptor(cx, proxy, id, desc);

    
    RootedValue propKey(cx);
    if (!IdToStringOrSymbol(cx, id, &propKey))
        return false;

    Value argv[] = {
        ObjectValue(*target),
        propKey
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    if (!trapResult.isUndefined() && !trapResult.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_GETOWN_OBJORUNDEF);
        return false;
    }

    
    Rooted<PropertyDescriptor> targetDesc(cx);
    if (!GetOwnPropertyDescriptor(cx, target, id, &targetDesc))
        return false;

    
    if (trapResult.isUndefined()) {
        
        if (!targetDesc.object()) {
            desc.object().set(nullptr);
            return true;
        }

        
        if (targetDesc.isPermanent()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_NC_AS_NE);
            return false;
        }

        
        bool extensibleTarget;
        if (!JSObject::isExtensible(cx, target, &extensibleTarget))
            return false;
        if (!extensibleTarget) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_E_AS_NE);
            return false;
        }

        
        desc.object().set(nullptr);
        return true;
    }

    
    bool extensibleTarget;
    if (!JSObject::isExtensible(cx, target, &extensibleTarget))
        return false;

    
    Rooted<PropDesc> resultDesc(cx);
    if (!resultDesc.initialize(cx, trapResult))
        return false;

    
    resultDesc.complete();

    
    bool valid;
    if (!ValidatePropertyDescriptor(cx, extensibleTarget, resultDesc, targetDesc, &valid))
        return false;

    
    if (!valid) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_INVALID);
        return false;
    }

    
    if (!resultDesc.configurable()) {
        if (!targetDesc.object()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_NE_AS_NC);
            return false;
        }

        if (!targetDesc.isPermanent()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_C_AS_NC);
            return false;
        }
    }

    
    resultDesc.populatePropertyDescriptor(proxy, desc);
    return true;
}


bool
ScriptedDirectProxyHandler::defineProperty(JSContext *cx, HandleObject proxy, HandleId id,
                                           MutableHandle<PropertyDescriptor> desc) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().defineProperty, &trap))
         return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::defineProperty(cx, proxy, id, desc);

    
    RootedValue descObj(cx);
    if (!NewPropertyDescriptorObject(cx, desc, &descObj))
        return false;

    
    RootedValue propKey(cx);
    if (!IdToStringOrSymbol(cx, id, &propKey))
        return false;

    Value argv[] = {
        ObjectValue(*target),
        propKey,
        descObj
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    if (ToBoolean(trapResult)) {
        
        Rooted<PropertyDescriptor> targetDesc(cx);
        if (!GetOwnPropertyDescriptor(cx, target, id, &targetDesc))
            return false;

        
        bool extensibleTarget;
        if (!JSObject::isExtensible(cx, target, &extensibleTarget))
            return false;

        
        bool settingConfigFalse = desc.isPermanent();
        if (!targetDesc.object()) {
            
            if (!extensibleTarget) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_DEFINE_NEW);
                return false;
            }
            
            if (settingConfigFalse) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_DEFINE_NE_AS_NC);
                return false;
            }
        } else {
            
            bool valid;
            Rooted<PropDesc> pd(cx);
            pd.initFromPropertyDescriptor(desc);
            if (!ValidatePropertyDescriptor(cx, extensibleTarget, pd, targetDesc, &valid))
                return false;
            if (!valid || (settingConfigFalse && !targetDesc.isPermanent())) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_DEFINE_INVALID);
                return false;
            }
        }
    }

    
    
    return true;
}



bool
ScriptedDirectProxyHandler::getOwnPropertyNames(JSContext *cx, HandleObject proxy,
                                                AutoIdVector &props) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().ownKeys, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::getOwnPropertyNames(cx, proxy, props);

    
    Value argv[] = {
        ObjectValue(*target)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    if (trapResult.isPrimitive()) {
        ReportInvalidTrapResult(cx, proxy, cx->names().ownKeys);
        return false;
    }

    
    
    return ArrayToIdVector(cx, proxy, target, trapResult, props,
                           JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS,
                           cx->names().getOwnPropertyNames);
}


bool
ScriptedDirectProxyHandler::delete_(JSContext *cx, HandleObject proxy, HandleId id, bool *bp) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().deleteProperty, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::delete_(cx, proxy, id, bp);

    
    RootedValue value(cx);
    if (!IdToStringOrSymbol(cx, id, &value))
        return false;
    Value argv[] = {
        ObjectValue(*target),
        value
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    if (ToBoolean(trapResult)) {
        
        Rooted<PropertyDescriptor> desc(cx);
        if (!GetOwnPropertyDescriptor(cx, target, id, &desc))
            return false;

        
        if (desc.object() && desc.isPermanent()) {
            RootedValue v(cx, IdToValue(id));
            js_ReportValueError(cx, JSMSG_CANT_DELETE, JSDVG_IGNORE_STACK, v, js::NullPtr());
            return false;
        }

        
        *bp = true;
        return true;
    }

    
    *bp = false;
    return true;
}


bool
ScriptedDirectProxyHandler::enumerate(JSContext *cx, HandleObject proxy, AutoIdVector &props) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().enumerate, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::enumerate(cx, proxy, props);

    
    Value argv[] = {
        ObjectOrNullValue(target)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    if (trapResult.isPrimitive()) {
        JSAutoByteString bytes;
        if (!AtomToPrintableString(cx, cx->names().enumerate, &bytes))
            return false;
        RootedValue v(cx, ObjectOrNullValue(proxy));
        js_ReportValueError2(cx, JSMSG_INVALID_TRAP_RESULT, JSDVG_SEARCH_STACK,
                             v, js::NullPtr(), bytes.ptr());
        return false;
    }

    
    
    
    return ArrayToIdVector(cx, proxy, target, trapResult, props, 0, cx->names().enumerate);
}


bool
ScriptedDirectProxyHandler::has(JSContext *cx, HandleObject proxy, HandleId id, bool *bp) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().has, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::has(cx, proxy, id, bp);

    
    RootedValue value(cx);
    if (!IdToStringOrSymbol(cx, id, &value))
        return false;
    Value argv[] = {
        ObjectOrNullValue(target),
        value
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    bool success = ToBoolean(trapResult);

    
    if (!success) {
        Rooted<PropertyDescriptor> desc(cx);
        if (!GetOwnPropertyDescriptor(cx, target, id, &desc))
            return false;

        if (desc.object()) {
            if (desc.isPermanent()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_NC_AS_NE);
                return false;
            }

            bool extensible;
            if (!JSObject::isExtensible(cx, target, &extensible))
                return false;
            if (!extensible) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_REPORT_E_AS_NE);
                return false;
            }
        }
    }

    
    *bp = success;
    return true;
}


bool
ScriptedDirectProxyHandler::get(JSContext *cx, HandleObject proxy, HandleObject receiver,
                                HandleId id, MutableHandleValue vp) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().get, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::get(cx, proxy, receiver, id, vp);

    
    RootedValue value(cx);
    if (!IdToStringOrSymbol(cx, id, &value))
        return false;
    Value argv[] = {
        ObjectOrNullValue(target),
        value,
        ObjectOrNullValue(receiver)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    Rooted<PropertyDescriptor> desc(cx);
    if (!GetOwnPropertyDescriptor(cx, target, id, &desc))
        return false;

    
    if (desc.object()) {
        if (IsDataDescriptor(desc) && desc.isPermanent() && desc.isReadonly()) {
            bool same;
            if (!SameValue(cx, trapResult, desc.value(), &same))
                return false;
            if (!same) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MUST_REPORT_SAME_VALUE);
                return false;
            }
        }

        if (IsAccessorDescriptor(desc) && desc.isPermanent() && !desc.hasGetterObject()) {
            if (!trapResult.isUndefined()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MUST_REPORT_UNDEFINED);
                return false;
            }
        }
    }

    
    vp.set(trapResult);
    return true;
}


bool
ScriptedDirectProxyHandler::set(JSContext *cx, HandleObject proxy, HandleObject receiver,
                                HandleId id, bool strict, MutableHandleValue vp) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().set, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::set(cx, proxy, receiver, id, strict, vp);

    
    RootedValue value(cx);
    if (!IdToStringOrSymbol(cx, id, &value))
        return false;
    Value argv[] = {
        ObjectOrNullValue(target),
        value,
        vp.get(),
        ObjectValue(*receiver)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    bool success = ToBoolean(trapResult);

    if (success) {
        
        Rooted<PropertyDescriptor> desc(cx);
        if (!GetOwnPropertyDescriptor(cx, target, id, &desc))
            return false;

        
        if (desc.object()) {
            if (IsDataDescriptor(desc) && desc.isPermanent() && desc.isReadonly()) {
                bool same;
                if (!SameValue(cx, vp, desc.value(), &same))
                    return false;
                if (!same) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_SET_NW_NC);
                    return false;
                }
            }

            if (IsAccessorDescriptor(desc) && desc.isPermanent() && !desc.hasSetterObject()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_SET_WO_SETTER);
                return false;
            }
        }
    }

    
    vp.set(BooleanValue(success));
    return true;
}


bool
ScriptedDirectProxyHandler::isExtensible(JSContext *cx, HandleObject proxy, bool *extensible) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().isExtensible, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::isExtensible(cx, proxy, extensible);

    
    Value argv[] = {
        ObjectValue(*target)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    bool booleanTrapResult = ToBoolean(trapResult);

    
    bool targetResult;
    if (!JSObject::isExtensible(cx, target, &targetResult))
        return false;

    
    if (targetResult != booleanTrapResult) {
       JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_EXTENSIBILITY);
       return false;
    }

    
    *extensible = booleanTrapResult;
    return true;
}

bool
ScriptedDirectProxyHandler::iterate(JSContext *cx, HandleObject proxy, unsigned flags,
                                    MutableHandleValue vp) const
{
    
    return DirectProxyHandler::iterate(cx, proxy, flags, vp);
}


bool
ScriptedDirectProxyHandler::call(JSContext *cx, HandleObject proxy, const CallArgs &args) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    




    
    RootedObject argsArray(cx, NewDenseCopiedArray(cx, args.length(), args.array()));
    if (!argsArray)
        return false;

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().apply, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::call(cx, proxy, args);

    
    Value argv[] = {
        ObjectValue(*target),
        args.thisv(),
        ObjectValue(*argsArray)
    };
    RootedValue thisValue(cx, ObjectValue(*handler));
    return Invoke(cx, thisValue, trap, ArrayLength(argv), argv, args.rval());
}


bool
ScriptedDirectProxyHandler::construct(JSContext *cx, HandleObject proxy, const CallArgs &args) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    




    
    RootedObject argsArray(cx, NewDenseCopiedArray(cx, args.length(), args.array()));
    if (!argsArray)
        return false;

    
    RootedValue trap(cx);
    if (!JSObject::getProperty(cx, handler, handler, cx->names().construct, &trap))
        return false;

    
    if (trap.isUndefined())
        return DirectProxyHandler::construct(cx, proxy, args);

    
    Value constructArgv[] = {
        ObjectValue(*target),
        ObjectValue(*argsArray)
    };
    RootedValue thisValue(cx, ObjectValue(*handler));
    if (!Invoke(cx, thisValue, trap, ArrayLength(constructArgv), constructArgv, args.rval()))
        return false;

    
    if (!args.rval().isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_PROXY_CONSTRUCT_OBJECT);
        return false;
    }
    return true;
}

bool
ScriptedDirectProxyHandler::isCallable(JSObject *obj) const
{
    MOZ_ASSERT(obj->as<ProxyObject>().handler() == &ScriptedDirectProxyHandler::singleton);
    return obj->as<ProxyObject>().extra(IS_CALLABLE_EXTRA).toBoolean();
}

const char ScriptedDirectProxyHandler::family = 0;
const ScriptedDirectProxyHandler ScriptedDirectProxyHandler::singleton;

bool
js::proxy(JSContext *cx, unsigned argc, jsval *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 2) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             "Proxy", "1", "s");
        return false;
    }
    RootedObject target(cx, NonNullObject(cx, args[0]));
    if (!target)
        return false;
    RootedObject handler(cx, NonNullObject(cx, args[1]));
    if (!handler)
        return false;
    RootedValue priv(cx, ObjectValue(*target));
    JSObject *proxy =
        NewProxyObject(cx, &ScriptedDirectProxyHandler::singleton,
                       priv, TaggedProto::LazyProto, cx->global());
    if (!proxy)
        return false;
    proxy->as<ProxyObject>().setExtra(ScriptedDirectProxyHandler::HANDLER_EXTRA, ObjectValue(*handler));
    proxy->as<ProxyObject>().setExtra(ScriptedDirectProxyHandler::IS_CALLABLE_EXTRA,
                                      BooleanValue(target->isCallable()));
    args.rval().setObject(*proxy);
    return true;
}

static bool
RevokeProxy(JSContext *cx, unsigned argc, Value *vp)
{
    CallReceiver rec = CallReceiverFromVp(vp);

    RootedFunction func(cx, &rec.callee().as<JSFunction>());
    RootedObject p(cx, func->getExtendedSlot(ScriptedDirectProxyHandler::REVOKE_SLOT).toObjectOrNull());

    if (p) {
        func->setExtendedSlot(ScriptedDirectProxyHandler::REVOKE_SLOT, NullValue());

        MOZ_ASSERT(p->is<ProxyObject>());

        p->as<ProxyObject>().setSameCompartmentPrivate(NullValue());
        p->as<ProxyObject>().setExtra(ScriptedDirectProxyHandler::HANDLER_EXTRA, NullValue());
    }

    rec.rval().setUndefined();
    return true;
}

bool
js::proxy_revocable(JSContext *cx, unsigned argc, Value *vp)
{
    CallReceiver args = CallReceiverFromVp(vp);

    if (!proxy(cx, argc, vp))
        return false;

    RootedValue proxyVal(cx, args.rval());
    MOZ_ASSERT(proxyVal.toObject().is<ProxyObject>());

    RootedObject revoker(cx, NewFunctionByIdWithReserved(cx, RevokeProxy, 0, 0, cx->global(),
                         AtomToId(cx->names().revoke)));
    if (!revoker)
        return false;

    revoker->as<JSFunction>().initExtendedSlot(ScriptedDirectProxyHandler::REVOKE_SLOT, proxyVal);

    RootedObject result(cx, NewBuiltinClassInstance(cx, &JSObject::class_));
    if (!result)
        return false;

    RootedValue revokeVal(cx, ObjectValue(*revoker));
    if (!JSObject::defineProperty(cx, result, cx->names().proxy, proxyVal) ||
        !JSObject::defineProperty(cx, result, cx->names().revoke, revokeVal))
    {
        return false;
    }

    args.rval().setObject(*result);
    return true;
}
