





#include "proxy/ScriptedDirectProxyHandler.h"

#include "jsapi.h"

#include "jsobjinlines.h"

using namespace js;
using mozilla::ArrayLength;

static inline bool
IsDataDescriptor(const PropertyDescriptor& desc)
{
    return desc.obj && !(desc.attrs & (JSPROP_GETTER | JSPROP_SETTER));
}

static inline bool
IsAccessorDescriptor(const PropertyDescriptor& desc)
{
    return desc.obj && desc.attrs & (JSPROP_GETTER | JSPROP_SETTER);
}




static bool
ValidatePropertyDescriptor(JSContext* cx, bool extensible, Handle<PropertyDescriptor> desc,
                           Handle<PropertyDescriptor> current, bool* bp)
{
    
    if (!current.object()) {
        
        *bp = extensible;
        return true;
    }

    
    if (!desc.hasValue() && !desc.hasWritable() &&
        !desc.hasGetterObject() && !desc.hasSetterObject() &&
        !desc.hasEnumerable() && !desc.hasConfigurable())
    {
        *bp = true;
        return true;
    }

    
    if ((!desc.hasWritable() ||
         (current.hasWritable() && desc.writable() == current.writable())) &&
        (!desc.hasGetterObject() || desc.getter() == current.getter()) &&
        (!desc.hasSetterObject() || desc.setter() == current.setter()) &&
        (!desc.hasEnumerable() || desc.enumerable() == current.enumerable()) &&
        (!desc.hasConfigurable() || desc.configurable() == current.configurable()))
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

    
    if (!current.configurable()) {
        if (desc.hasConfigurable() && desc.configurable()) {
            *bp = false;
            return true;
        }

        if (desc.hasEnumerable() && desc.enumerable() != current.enumerable()) {
            *bp = false;
            return true;
        }
    }

    
    if (desc.isGenericDescriptor()) {
        *bp = true;
        return true;
    }

    
    if (current.isDataDescriptor() != desc.isDataDescriptor()) {
        *bp = current.configurable();
        return true;
    }

    
    if (current.isDataDescriptor()) {
        MOZ_ASSERT(desc.isDataDescriptor()); 
        if (!current.configurable() && !current.writable()) {
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

    
    MOZ_ASSERT(current.isAccessorDescriptor()); 
    MOZ_ASSERT(desc.isAccessorDescriptor()); 
    *bp = (current.configurable() ||
           ((!desc.hasSetterObject() || desc.setter() == current.setter()) &&
            (!desc.hasGetterObject() || desc.getter() == current.getter())));
    return true;
}


static bool
IsSealed(JSContext* cx, HandleObject obj, HandleId id, bool* bp)
{
    
    Rooted<PropertyDescriptor> desc(cx);
    if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;

    
    *bp = desc.object() && !desc.configurable();
    return true;
}


static JSObject*
GetDirectProxyHandlerObject(JSObject* proxy)
{
    MOZ_ASSERT(proxy->as<ProxyObject>().handler() == &ScriptedDirectProxyHandler::singleton);
    return proxy->as<ProxyObject>().extra(ScriptedDirectProxyHandler::HANDLER_EXTRA).toObjectOrNull();
}

static inline void
ReportInvalidTrapResult(JSContext* cx, JSObject* proxy, JSAtom* atom)
{
    RootedValue v(cx, ObjectOrNullValue(proxy));
    JSAutoByteString bytes;
    if (!AtomToPrintableString(cx, atom, &bytes))
        return;
    ReportValueError2(cx, JSMSG_INVALID_TRAP_RESULT, JSDVG_IGNORE_STACK, v,
                      js::NullPtr(), bytes.ptr());
}



static bool
ArrayToIdVector(JSContext* cx, HandleObject proxy, HandleObject target, HandleValue v,
                AutoIdVector& props, unsigned flags, JSAtom* trapName_)
{
    MOZ_ASSERT(v.isObject());
    RootedObject array(cx, &v.toObject());
    RootedAtom trapName(cx, trapName_);

    
    uint32_t n;
    if (!GetLengthProperty(cx, array, &n))
        return false;

    
    for (uint32_t i = 0; i < n; ++i) {
        
        RootedValue v(cx);
        if (!GetElement(cx, array, array, i, &v))
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
        if (!HasOwnProperty(cx, target, id, &isFixed))
            return false;

        
        bool extensible;
        if (!IsExtensible(cx, target, &extensible))
            return false;
        if (!extensible && !isFixed) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_NEW);
            return false;
        }

        
        if (!props.append(id))
            return false;
    }

    
    AutoIdVector ownProps(cx);
    if (!GetPropertyKeys(cx, target, flags, &ownProps))
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
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_SKIP_NC);
            return false;
        }

        
        bool isFixed;
        if (!HasOwnProperty(cx, target, id, &isFixed))
            return false;

        
        bool extensible;
        if (!IsExtensible(cx, target, &extensible))
            return false;
        if (!extensible && isFixed) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_E_AS_NE);
            return false;
        }
    }

    
    return true;
}



bool
ScriptedDirectProxyHandler::getPrototype(JSContext* cx, HandleObject proxy,
                                         MutableHandleObject protop) const
{
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    
    if (!target) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    return GetPrototype(cx, target, protop);
}

bool
ScriptedDirectProxyHandler::setPrototype(JSContext* cx, HandleObject proxy, HandleObject proto,
                                         ObjectOpResult& result) const
{
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    if (!target) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    return SetPrototype(cx, target, proto, result);
}



bool
ScriptedDirectProxyHandler::setImmutablePrototype(JSContext* cx, HandleObject proxy,
                                                  bool* succeeded) const
{
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    if (!target) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    return SetImmutablePrototype(cx, target, succeeded);
}


bool
ScriptedDirectProxyHandler::preventExtensions(JSContext* cx, HandleObject proxy,
                                              ObjectOpResult& result) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().preventExtensions, &trap))
        return false;

    
    if (trap.isUndefined())
        return PreventExtensions(cx, target, result);

    
    Value argv[] = {
        ObjectValue(*target)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    if (ToBoolean(trapResult)) {
        bool extensible;
        if (!IsExtensible(cx, target, &extensible))
            return false;
        if (extensible) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                                 JSMSG_CANT_REPORT_AS_NON_EXTENSIBLE);
            return false;
        }
        return result.succeed();
    }
    return result.fail(JSMSG_PROXY_PREVENTEXTENSIONS_RETURNED_FALSE);
}


bool
ScriptedDirectProxyHandler::isExtensible(JSContext* cx, HandleObject proxy, bool* extensible) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().isExtensible, &trap))
        return false;

    
    if (trap.isUndefined())
        return IsExtensible(cx, target, extensible);

    
    Value argv[] = {
        ObjectValue(*target)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    bool booleanTrapResult = ToBoolean(trapResult);

    
    bool targetResult;
    if (!IsExtensible(cx, target, &targetResult))
        return false;

    
    if (targetResult != booleanTrapResult) {
       JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_EXTENSIBILITY);
       return false;
    }

    
    *extensible = booleanTrapResult;
    return true;
}



bool
ScriptedDirectProxyHandler::getPropertyDescriptor(JSContext* cx, HandleObject proxy, HandleId id,
                                                  MutableHandle<PropertyDescriptor> desc) const
{
    JS_CHECK_RECURSION(cx, return false);

    if (!GetOwnPropertyDescriptor(cx, proxy, id, desc))
        return false;
    if (desc.object())
        return true;
    RootedObject proto(cx);
    if (!GetPrototype(cx, proxy, &proto))
        return false;
    if (!proto) {
        MOZ_ASSERT(!desc.object());
        return true;
    }
    return GetPropertyDescriptor(cx, proto, id, desc);
}


bool
ScriptedDirectProxyHandler::getOwnPropertyDescriptor(JSContext* cx, HandleObject proxy, HandleId id,
                                                     MutableHandle<PropertyDescriptor> desc) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().getOwnPropertyDescriptor, &trap))
        return false;

    
    if (trap.isUndefined())
        return GetOwnPropertyDescriptor(cx, target, id, desc);

    
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
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_GETOWN_OBJORUNDEF);
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

        
        if (!targetDesc.configurable()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_NC_AS_NE);
            return false;
        }

        
        bool extensibleTarget;
        if (!IsExtensible(cx, target, &extensibleTarget))
            return false;
        if (!extensibleTarget) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_E_AS_NE);
            return false;
        }

        
        desc.object().set(nullptr);
        return true;
    }

    
    bool extensibleTarget;
    if (!IsExtensible(cx, target, &extensibleTarget))
        return false;

    
    Rooted<PropertyDescriptor> resultDesc(cx);
    if (!ToPropertyDescriptor(cx, trapResult, true, &resultDesc))
        return false;

    
    CompletePropertyDescriptor(&resultDesc);

    
    bool valid;
    if (!ValidatePropertyDescriptor(cx, extensibleTarget, resultDesc, targetDesc, &valid))
        return false;

    
    if (!valid) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_INVALID);
        return false;
    }

    
    if (!resultDesc.configurable()) {
        if (!targetDesc.object()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_NE_AS_NC);
            return false;
        }

        if (targetDesc.configurable()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_C_AS_NC);
            return false;
        }
    }

    
    desc.set(resultDesc);
    desc.object().set(proxy);
    return true;
}


bool
ScriptedDirectProxyHandler::defineProperty(JSContext* cx, HandleObject proxy, HandleId id,
                                           Handle<PropertyDescriptor> desc,
                                           ObjectOpResult& result) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().defineProperty, &trap))
        return false;

    
    if (trap.isUndefined())
        return StandardDefineProperty(cx, target, id, desc, result);

    
    RootedValue descObj(cx);
    if (!FromPropertyDescriptor(cx, desc, &descObj))
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

    
    if (!ToBoolean(trapResult))
        return result.fail(JSMSG_PROXY_DEFINE_RETURNED_FALSE);

    
    Rooted<PropertyDescriptor> targetDesc(cx);
    if (!GetOwnPropertyDescriptor(cx, target, id, &targetDesc))
        return false;

    
    bool extensibleTarget;
    if (!IsExtensible(cx, target, &extensibleTarget))
        return false;

    
    bool settingConfigFalse = desc.hasConfigurable() && !desc.configurable();
    if (!targetDesc.object()) {
        
        if (!extensibleTarget) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_DEFINE_NEW);
            return false;
        }
        
        if (settingConfigFalse) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_DEFINE_NE_AS_NC);
            return false;
        }
    } else {
        
        bool valid;
        if (!ValidatePropertyDescriptor(cx, extensibleTarget, desc, targetDesc, &valid))
            return false;
        if (!valid || (settingConfigFalse && targetDesc.configurable())) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_DEFINE_INVALID);
            return false;
        }
    }

    
    return result.succeed();
}


bool
ScriptedDirectProxyHandler::ownPropertyKeys(JSContext* cx, HandleObject proxy,
                                            AutoIdVector& props) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().ownKeys, &trap))
        return false;

    
    if (trap.isUndefined())
        return GetPropertyKeys(cx, target, JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS, &props);

    
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
                           cx->names().ownKeys);
}


bool
ScriptedDirectProxyHandler::delete_(JSContext* cx, HandleObject proxy, HandleId id,
                                    ObjectOpResult& result) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().deleteProperty, &trap))
        return false;

    
    if (trap.isUndefined())
        return DeleteProperty(cx, target, id, result);

    
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

    
    if (!ToBoolean(trapResult))
        return result.fail(JSMSG_PROXY_DELETE_RETURNED_FALSE);

    
    Rooted<PropertyDescriptor> desc(cx);
    if (!GetOwnPropertyDescriptor(cx, target, id, &desc))
        return false;

    
    if (desc.object() && !desc.configurable()) {
        RootedValue v(cx, IdToValue(id));
        ReportValueError(cx, JSMSG_CANT_DELETE, JSDVG_IGNORE_STACK, v, js::NullPtr());
        return false;
    }

    
    return result.succeed();
}


bool
ScriptedDirectProxyHandler::enumerate(JSContext* cx, HandleObject proxy,
                                      MutableHandleObject objp) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().enumerate, &trap))
        return false;

    
    if (trap.isUndefined())
        return GetIterator(cx, target, 0, objp);

    
    Value argv[] = {
        ObjectOrNullValue(target)
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    if (trapResult.isPrimitive()) {
        ReportInvalidTrapResult(cx, proxy, cx->names().enumerate);
        return false;
    }

    
    objp.set(&trapResult.toObject());
    return true;
}


bool
ScriptedDirectProxyHandler::has(JSContext* cx, HandleObject proxy, HandleId id, bool* bp) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().has, &trap))
        return false;

    
    if (trap.isUndefined())
        return HasProperty(cx, target, id, bp);

    
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
            if (!desc.configurable()) {
                JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_NC_AS_NE);
                return false;
            }

            bool extensible;
            if (!IsExtensible(cx, target, &extensible))
                return false;
            if (!extensible) {
                JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_REPORT_E_AS_NE);
                return false;
            }
        }
    }

    
    *bp = success;
    return true;
}


bool
ScriptedDirectProxyHandler::get(JSContext* cx, HandleObject proxy, HandleObject receiver,
                                HandleId id, MutableHandleValue vp) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().get, &trap))
        return false;

    
    if (trap.isUndefined())
        return GetProperty(cx, target, receiver, id, vp);

    
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
        if (desc.isDataDescriptor() && !desc.configurable() && !desc.writable()) {
            bool same;
            if (!SameValue(cx, trapResult, desc.value(), &same))
                return false;
            if (!same) {
                JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_MUST_REPORT_SAME_VALUE);
                return false;
            }
        }

        if (desc.isAccessorDescriptor() && !desc.configurable() && desc.getterObject() == nullptr) {
            if (!trapResult.isUndefined()) {
                JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_MUST_REPORT_UNDEFINED);
                return false;
            }
        }
    }

    
    vp.set(trapResult);
    return true;
}


bool
ScriptedDirectProxyHandler::set(JSContext* cx, HandleObject proxy, HandleId id, HandleValue v,
                                HandleValue receiver, ObjectOpResult& result) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().set, &trap))
        return false;

    
    if (trap.isUndefined())
        return SetProperty(cx, target, id, v, receiver, result);

    
    RootedValue value(cx);
    if (!IdToStringOrSymbol(cx, id, &value))
        return false;
    Value argv[] = {
        ObjectOrNullValue(target),
        value,
        v.get(),
        receiver.get()
    };
    RootedValue trapResult(cx);
    if (!Invoke(cx, ObjectValue(*handler), trap, ArrayLength(argv), argv, &trapResult))
        return false;

    
    if (!ToBoolean(trapResult))
        return result.fail(JSMSG_PROXY_SET_RETURNED_FALSE);

    
    Rooted<PropertyDescriptor> desc(cx);
    if (!GetOwnPropertyDescriptor(cx, target, id, &desc))
        return false;

    
    if (desc.object()) {
        if (desc.isDataDescriptor() && !desc.configurable() && !desc.writable()) {
            bool same;
            if (!SameValue(cx, v, desc.value(), &same))
                return false;
            if (!same) {
                JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_SET_NW_NC);
                return false;
            }
        }

        if (desc.isAccessorDescriptor() && !desc.configurable() && desc.setterObject() == nullptr) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_SET_WO_SETTER);
            return false;
        }
    }

    
    return result.succeed();
}


bool
ScriptedDirectProxyHandler::call(JSContext* cx, HandleObject proxy, const CallArgs& args) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    MOZ_ASSERT(target->isCallable());

    
    RootedObject argsArray(cx, NewDenseCopiedArray(cx, args.length(), args.array()));
    if (!argsArray)
        return false;

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().apply, &trap))
        return false;

    
    if (trap.isUndefined()) {
        RootedValue targetv(cx, ObjectValue(*target));
        return Invoke(cx, args.thisv(), targetv, args.length(), args.array(), args.rval());
    }

    
    Value argv[] = {
        ObjectValue(*target),
        args.thisv(),
        ObjectValue(*argsArray)
    };
    RootedValue thisValue(cx, ObjectValue(*handler));
    return Invoke(cx, thisValue, trap, ArrayLength(argv), argv, args.rval());
}


bool
ScriptedDirectProxyHandler::construct(JSContext* cx, HandleObject proxy, const CallArgs& args) const
{
    
    RootedObject handler(cx, GetDirectProxyHandlerObject(proxy));

    
    if (!handler) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    MOZ_ASSERT(target->isConstructor());

    
    RootedObject argsArray(cx, NewDenseCopiedArray(cx, args.length(), args.array()));
    if (!argsArray)
        return false;

    
    RootedValue trap(cx);
    if (!GetProperty(cx, handler, handler, cx->names().construct, &trap))
        return false;

    
    if (trap.isUndefined()) {
        RootedValue targetv(cx, ObjectValue(*target));
        return InvokeConstructor(cx, targetv, args.length(), args.array(), args.rval());
    }

    
    Value constructArgv[] = {
        ObjectValue(*target),
        ObjectValue(*argsArray)
    };
    RootedValue thisValue(cx, ObjectValue(*handler));
    if (!Invoke(cx, thisValue, trap, ArrayLength(constructArgv), constructArgv, args.rval()))
        return false;

    
    if (!args.rval().isObject()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_CONSTRUCT_OBJECT);
        return false;
    }
    return true;
}

bool
ScriptedDirectProxyHandler::nativeCall(JSContext* cx, IsAcceptableThis test, NativeImpl impl,
                                       CallArgs args) const
{
    ReportIncompatible(cx, args);
    return false;
}

bool
ScriptedDirectProxyHandler::hasInstance(JSContext* cx, HandleObject proxy, MutableHandleValue v,
                                        bool* bp) const
{
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    if (!target) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_REVOKED);
        return false;
    }

    return HasInstance(cx, target, v, bp);
}

bool
ScriptedDirectProxyHandler::objectClassIs(HandleObject proxy, ESClassValue classValue,
                                          JSContext* cx) const
{
    
    
    if (classValue != ESClass_IsArray)
        return false;

    
    
    
    
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    if (!target)
        return false;

    return IsArray(target, cx);
}

const char*
ScriptedDirectProxyHandler::className(JSContext* cx, HandleObject proxy) const
{
    
    RootedObject target(cx, proxy->as<ProxyObject>().target());
    if (!target)
        return BaseProxyHandler::className(cx, proxy);

    return GetObjectClassName(cx, target);
}
JSString*
ScriptedDirectProxyHandler::fun_toString(JSContext* cx, HandleObject proxy,
                                         unsigned indent) const
{
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INCOMPATIBLE_PROTO,
                         js_Function_str, js_toString_str, "object");
    return nullptr;
}

bool
ScriptedDirectProxyHandler::regexp_toShared(JSContext* cx, HandleObject proxy,
                                            RegExpGuard* g) const
{
    MOZ_CRASH("Should not end up in ScriptedDirectProxyHandler::regexp_toShared");
    return false;
}

bool
ScriptedDirectProxyHandler::boxedValue_unbox(JSContext* cx, HandleObject proxy,
                                             MutableHandleValue vp) const
{
    MOZ_CRASH("Should not end up in ScriptedDirectProxyHandler::boxedValue_unbox");
    return false;
}

bool
ScriptedDirectProxyHandler::isCallable(JSObject* obj) const
{
    MOZ_ASSERT(obj->as<ProxyObject>().handler() == &ScriptedDirectProxyHandler::singleton);
    uint32_t callConstruct = obj->as<ProxyObject>().extra(IS_CALLCONSTRUCT_EXTRA).toPrivateUint32();
    return !!(callConstruct & IS_CALLABLE);
}

bool
ScriptedDirectProxyHandler::isConstructor(JSObject* obj) const
{
    MOZ_ASSERT(obj->as<ProxyObject>().handler() == &ScriptedDirectProxyHandler::singleton);
    uint32_t callConstruct = obj->as<ProxyObject>().extra(IS_CALLCONSTRUCT_EXTRA).toPrivateUint32();
    return !!(callConstruct & IS_CONSTRUCTOR);
}

const char ScriptedDirectProxyHandler::family = 0;
const ScriptedDirectProxyHandler ScriptedDirectProxyHandler::singleton;

bool
IsRevokedScriptedProxy(JSObject* obj)
{
    obj = CheckedUnwrap(obj);
    return obj && IsScriptedProxy(obj) && !obj->as<ProxyObject>().target();
}


static bool
NewScriptedProxy(JSContext* cx, CallArgs& args, const char* callerName)
{
    if (args.length() < 2) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             callerName, "1", "s");
        return false;
    }

    
    RootedObject target(cx, NonNullObject(cx, args[0]));
    if (!target)
        return false;

    
    if (IsRevokedScriptedProxy(target)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_ARG_REVOKED, "1");
        return false;
    }

    
    RootedObject handler(cx, NonNullObject(cx, args[1]));
    if (!handler)
        return false;

    
    if (IsRevokedScriptedProxy(handler)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_PROXY_ARG_REVOKED, "2");
        return false;
    }

    
    RootedValue priv(cx, ObjectValue(*target));
    JSObject* proxy_ =
        NewProxyObject(cx, &ScriptedDirectProxyHandler::singleton,
                       priv, TaggedProto::LazyProto);
    if (!proxy_)
        return false;

    
    Rooted<ProxyObject*> proxy(cx, &proxy_->as<ProxyObject>());
    proxy->setExtra(ScriptedDirectProxyHandler::HANDLER_EXTRA, ObjectValue(*handler));

    
    uint32_t callable = target->isCallable() ? ScriptedDirectProxyHandler::IS_CALLABLE : 0;
    uint32_t constructor = target->isConstructor() ? ScriptedDirectProxyHandler::IS_CONSTRUCTOR : 0;
    proxy->as<ProxyObject>().setExtra(ScriptedDirectProxyHandler::IS_CALLCONSTRUCT_EXTRA,
                                      PrivateUint32Value(callable | constructor));

    
    args.rval().setObject(*proxy);
    return true;
}

bool
js::proxy(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_FUNCTION, "Proxy");
        return false;
    }

    return NewScriptedProxy(cx, args, "Proxy");
}

static bool
RevokeProxy(JSContext* cx, unsigned argc, Value* vp)
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
js::proxy_revocable(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!NewScriptedProxy(cx, args, "Proxy.revocable"))
        return false;

    RootedValue proxyVal(cx, args.rval());
    MOZ_ASSERT(proxyVal.toObject().is<ProxyObject>());

    RootedObject revoker(cx, NewFunctionByIdWithReserved(cx, RevokeProxy, 0, 0,
                         AtomToId(cx->names().revoke)));
    if (!revoker)
        return false;

    revoker->as<JSFunction>().initExtendedSlot(ScriptedDirectProxyHandler::REVOKE_SLOT, proxyVal);

    RootedPlainObject result(cx, NewBuiltinClassInstance<PlainObject>(cx));
    if (!result)
        return false;

    RootedValue revokeVal(cx, ObjectValue(*revoker));
    if (!DefineProperty(cx, result, cx->names().proxy, proxyVal) ||
        !DefineProperty(cx, result, cx->names().revoke, revokeVal))
    {
        return false;
    }

    args.rval().setObject(*result);
    return true;
}
