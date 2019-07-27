









#include "jsobjinlines.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/SizePrintfMacros.h"
#include "mozilla/TemplateLib.h"
#include "mozilla/UniquePtr.h"

#include <string.h>

#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsfriendapi.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsiter.h"
#include "jsnum.h"
#include "jsopcode.h"
#include "jsprf.h"
#include "jsscript.h"
#include "jsstr.h"
#include "jstypes.h"
#include "jsutil.h"
#include "jswatchpoint.h"
#include "jswrapper.h"

#include "asmjs/AsmJSModule.h"
#include "builtin/Eval.h"
#include "builtin/Object.h"
#include "builtin/SymbolObject.h"
#include "frontend/BytecodeCompiler.h"
#include "gc/Marking.h"
#include "jit/BaselineJIT.h"
#include "js/MemoryMetrics.h"
#include "js/Proxy.h"
#include "vm/ArgumentsObject.h"
#include "vm/Interpreter.h"
#include "vm/ProxyObject.h"
#include "vm/RegExpStaticsObject.h"
#include "vm/Shape.h"
#include "vm/TypedArrayCommon.h"

#include "jsatominlines.h"
#include "jsboolinlines.h"
#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"

#include "vm/ArrayObject-inl.h"
#include "vm/BooleanObject-inl.h"
#include "vm/Interpreter-inl.h"
#include "vm/NativeObject-inl.h"
#include "vm/NumberObject-inl.h"
#include "vm/Runtime-inl.h"
#include "vm/Shape-inl.h"
#include "vm/StringObject-inl.h"

using namespace js;
using namespace js::gc;

using mozilla::DebugOnly;
using mozilla::Maybe;
using mozilla::UniquePtr;

JS_FRIEND_API(JSObject*)
JS_ObjectToInnerObject(JSContext* cx, HandleObject obj)
{
    if (!obj) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INACTIVE);
        return nullptr;
    }
    return GetInnerObject(obj);
}

JS_FRIEND_API(JSObject*)
JS_ObjectToOuterObject(JSContext* cx, HandleObject obj)
{
    assertSameCompartment(cx, obj);
    return GetOuterObject(cx, obj);
}

JSObject*
js::NonNullObject(JSContext* cx, const Value& v)
{
    if (v.isPrimitive()) {
        RootedValue value(cx, v);
        UniquePtr<char[], JS::FreePolicy> bytes =
            DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, value, NullPtr());
        if (!bytes)
            return nullptr;
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT, bytes.get());
        return nullptr;
    }
    return &v.toObject();
}

const char*
js::InformalValueTypeName(const Value& v)
{
    if (v.isObject())
        return v.toObject().getClass()->name;
    if (v.isString())
        return "string";
    if (v.isSymbol())
        return "symbol";
    if (v.isNumber())
        return "number";
    if (v.isBoolean())
        return "boolean";
    if (v.isNull())
        return "null";
    if (v.isUndefined())
        return "undefined";
    return "value";
}

bool
js::FromPropertyDescriptor(JSContext* cx, Handle<PropertyDescriptor> desc,
                           MutableHandleValue vp)
{
    if (!desc.object()) {
        vp.setUndefined();
        return true;
    }

    RootedObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx));
    if (!obj)
        return false;

    const JSAtomState& names = cx->names();
    RootedValue v(cx);
    if (desc.hasConfigurable()) {
        v.setBoolean(desc.configurable());
        if (!DefineProperty(cx, obj, names.configurable, v))
            return false;
    }
    if (desc.hasEnumerable()) {
        v.setBoolean(desc.enumerable());
        if (!DefineProperty(cx, obj, names.enumerable, v))
            return false;
    }
    if (desc.hasValue()) {
        if (!DefineProperty(cx, obj, names.value, desc.value()))
            return false;
    }
    if (desc.hasWritable()) {
        v.setBoolean(desc.writable());
        if (!DefineProperty(cx, obj, names.writable, v))
            return false;
    }
    if (desc.hasGetterObject()) {
        if (JSObject* get = desc.getterObject())
            v.setObject(*get);
        else
            v.setUndefined();
        if (!DefineProperty(cx, obj, names.get, v))
            return false;
    }
    if (desc.hasSetterObject()) {
        if (JSObject* set = desc.setterObject())
            v.setObject(*set);
        else
            v.setUndefined();
        if (!DefineProperty(cx, obj, names.set, v))
            return false;
    }
    vp.setObject(*obj);
    return true;
}

bool
js::GetFirstArgumentAsObject(JSContext* cx, const CallArgs& args, const char* method,
                             MutableHandleObject objp)
{
    if (args.length() == 0) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             method, "0", "s");
        return false;
    }

    HandleValue v = args[0];
    if (!v.isObject()) {
        UniquePtr<char[], JS::FreePolicy> bytes =
            DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, v, NullPtr());
        if (!bytes)
            return false;
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                             bytes.get(), "not an object");
        return false;
    }

    objp.set(&v.toObject());
    return true;
}

static bool
GetPropertyIfPresent(JSContext* cx, HandleObject obj, HandleId id, MutableHandleValue vp,
                     bool* foundp)
{
    if (!HasProperty(cx, obj, id, foundp))
        return false;
    if (!*foundp) {
        vp.setUndefined();
        return true;
    }

    return GetProperty(cx, obj, obj, id, vp);
}

bool
js::Throw(JSContext* cx, jsid id, unsigned errorNumber)
{
    MOZ_ASSERT(js_ErrorFormatString[errorNumber].argCount == 1);

    RootedValue idVal(cx, IdToValue(id));
    JSString* idstr = ValueToSource(cx, idVal);
    if (!idstr)
       return false;
    JSAutoByteString bytes(cx, idstr);
    if (!bytes)
        return false;
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, errorNumber, bytes.ptr());
    return false;
}

bool
js::Throw(JSContext* cx, JSObject* obj, unsigned errorNumber)
{
    if (js_ErrorFormatString[errorNumber].argCount == 1) {
        RootedValue val(cx, ObjectValue(*obj));
        ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,
                              JSDVG_IGNORE_STACK, val, NullPtr(),
                              nullptr, nullptr);
    } else {
        MOZ_ASSERT(js_ErrorFormatString[errorNumber].argCount == 0);
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, errorNumber);
    }
    return false;
}




static bool
DefinePropertyOnObject(JSContext* cx, HandleNativeObject obj, HandleId id,
                       Handle<PropertyDescriptor> desc, ObjectOpResult& result)
{
    
    RootedShape shape(cx);
    MOZ_ASSERT(!obj->getOps()->lookupProperty);
    if (!NativeLookupOwnProperty<CanGC>(cx, obj, id, &shape))
        return false;

    MOZ_ASSERT(!obj->getOps()->defineProperty);

    
    if (!shape) {
        bool extensible;
        if (!IsExtensible(cx, obj, &extensible))
            return false;
        if (!extensible)
            return result.fail(JSMSG_OBJECT_NOT_EXTENSIBLE);

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            MOZ_ASSERT(!obj->getOps()->defineProperty);
            RootedValue v(cx, desc.hasValue() ? desc.value() : UndefinedValue());
            unsigned attrs = desc.attributes() & (JSPROP_PERMANENT | JSPROP_ENUMERATE | JSPROP_READONLY);

            if (!desc.hasConfigurable())
                attrs |= JSPROP_PERMANENT;
            if (!desc.hasWritable())
                attrs |= JSPROP_READONLY;
            return NativeDefineProperty(cx, obj, id, v, nullptr, nullptr, attrs, result);
        }

        MOZ_ASSERT(desc.isAccessorDescriptor());

        unsigned attrs = desc.attributes() & (JSPROP_PERMANENT | JSPROP_ENUMERATE |
                                              JSPROP_SHARED | JSPROP_GETTER | JSPROP_SETTER);
        if (!desc.hasConfigurable())
            attrs |= JSPROP_PERMANENT;
        return NativeDefineProperty(cx, obj, id, UndefinedHandleValue,
                                    desc.getter(), desc.setter(), attrs, result);
    }

    
    RootedValue v(cx);

    bool shapeDataDescriptor = true,
         shapeAccessorDescriptor = false,
         shapeWritable = true,
         shapeConfigurable = true,
         shapeEnumerable = true,
         shapeHasDefaultGetter = true,
         shapeHasDefaultSetter = true,
         shapeHasGetterValue = false,
         shapeHasSetterValue = false;
    uint8_t shapeAttributes = GetShapeAttributes(obj, shape);
    if (!IsImplicitDenseOrTypedArrayElement(shape)) {
        shapeDataDescriptor = shape->isDataDescriptor();
        shapeAccessorDescriptor = shape->isAccessorDescriptor();
        shapeWritable = shape->writable();
        shapeConfigurable = shape->configurable();
        shapeEnumerable = shape->enumerable();
        shapeHasDefaultGetter = shape->hasDefaultGetter();
        shapeHasDefaultSetter = shape->hasDefaultSetter();
        shapeHasGetterValue = shape->hasGetterValue();
        shapeHasSetterValue = shape->hasSetterValue();
        shapeAttributes = shape->attributes();
    }

    do {
        if (desc.isAccessorDescriptor()) {
            if (!shapeAccessorDescriptor)
                break;

            if (desc.hasGetterObject()) {
                if (!shape->hasGetterValue() || desc.getterObject() != shape->getterObject())
                    break;
            }

            if (desc.hasSetterObject()) {
                if (!shape->hasSetterValue() || desc.setterObject() != shape->setterObject())
                    break;
            }
        } else {
            






            if (IsImplicitDenseOrTypedArrayElement(shape)) {
                v = obj->getDenseOrTypedArrayElement(JSID_TO_INT(id));
            } else if (shape->isDataDescriptor()) {
                









                if (!shape->configurable() &&
                    (!shape->hasDefaultGetter() || !shape->hasDefaultSetter()) &&
                    desc.isDataDescriptor() &&
                    (desc.hasWritable() ? desc.writable() : shape->writable()))
                {
                    return result.fail(JSMSG_CANT_REDEFINE_PROP);
                }

                if (!NativeGetExistingProperty(cx, obj, obj, shape, &v))
                    return false;
            }

            if (desc.isDataDescriptor()) {
                if (!shapeDataDescriptor)
                    break;

                bool same;
                if (desc.hasValue()) {
                    if (!SameValue(cx, desc.value(), v, &same))
                        return false;
                    if (!same) {
                        















                        if (!shapeConfigurable &&
                            (!shape->hasDefaultGetter() || !shape->hasDefaultSetter()))
                        {
                            return result.fail(JSMSG_CANT_REDEFINE_PROP);
                        }
                        break;
                    }
                }
                if (desc.hasWritable() && desc.writable() != shapeWritable)
                    break;
            } else {
                
                MOZ_ASSERT(desc.isGenericDescriptor());
            }
        }

        if (desc.hasConfigurable() && desc.configurable() != shapeConfigurable)
            break;
        if (desc.hasEnumerable() && desc.enumerable() != shapeEnumerable)
            break;

        
        return result.succeed();
    } while (0);

    
    if (!shapeConfigurable) {
        if ((desc.hasConfigurable() && desc.configurable()) ||
            (desc.hasEnumerable() && desc.enumerable() != shape->enumerable())) {
            return result.fail(JSMSG_CANT_REDEFINE_PROP);
        }
    }

    bool callDelProperty = false;

    if (desc.isGenericDescriptor()) {
        
    } else if (desc.isDataDescriptor() != shapeDataDescriptor) {
        
        if (!shapeConfigurable)
            return result.fail(JSMSG_CANT_REDEFINE_PROP);
    } else if (desc.isDataDescriptor()) {
        
        MOZ_ASSERT(shapeDataDescriptor);
        if (!shapeConfigurable && !shape->writable()) {
            if (desc.hasWritable() && desc.writable())
                return result.fail(JSMSG_CANT_REDEFINE_PROP);
            if (desc.hasValue()) {
                bool same;
                if (!SameValue(cx, desc.value(), v, &same))
                    return false;
                if (!same)
                    return result.fail(JSMSG_CANT_REDEFINE_PROP);
            }
        }

        callDelProperty = !shapeHasDefaultGetter || !shapeHasDefaultSetter;
    } else {
        
        MOZ_ASSERT(desc.isAccessorDescriptor() && shape->isAccessorDescriptor());
        if (!shape->configurable()) {
            
            
            
            
            
            if (desc.hasSetterObject() &&
                desc.setterObject() != (shape->hasSetterValue() ? shape->setterObject() : nullptr))
            {
                return result.fail(JSMSG_CANT_REDEFINE_PROP);
            }

            if (desc.hasGetterObject() &&
                desc.getterObject() != (shape->hasGetterValue() ? shape->getterObject() : nullptr))
            {
                return result.fail(JSMSG_CANT_REDEFINE_PROP);
            }
        }
    }

    
    unsigned attrs;
    GetterOp getter;
    SetterOp setter;
    if (desc.isGenericDescriptor()) {
        unsigned changed = 0;
        if (desc.hasConfigurable())
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable())
            changed |= JSPROP_ENUMERATE;

        attrs = (shapeAttributes & ~changed) | (desc.attributes() & changed);
        getter = IsImplicitDenseOrTypedArrayElement(shape) ? nullptr : shape->getter();
        setter = IsImplicitDenseOrTypedArrayElement(shape) ? nullptr : shape->setter();
    } else if (desc.isDataDescriptor()) {
        
        unsigned changed = JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED;
        unsigned descAttrs = desc.attributes();
        if (desc.hasConfigurable())
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable())
            changed |= JSPROP_ENUMERATE;

        if (desc.hasWritable()) {
            changed |= JSPROP_READONLY;
        } else if (!shapeDataDescriptor) {
            changed |= JSPROP_READONLY;
            descAttrs |= JSPROP_READONLY;
        }

        if (desc.hasValue())
            v = desc.value();
        attrs = (descAttrs & changed) | (shapeAttributes & ~changed);
        getter = nullptr;
        setter = nullptr;
    } else {
        MOZ_ASSERT(desc.isAccessorDescriptor());

        
        unsigned changed = 0;
        if (desc.hasConfigurable())
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable())
            changed |= JSPROP_ENUMERATE;
        if (desc.hasGetterObject())
            changed |= JSPROP_GETTER | JSPROP_SHARED | JSPROP_READONLY | JSPROP_SHADOWABLE;
        if (desc.hasSetterObject())
            changed |= JSPROP_SETTER | JSPROP_SHARED | JSPROP_READONLY | JSPROP_SHADOWABLE;

        attrs = (desc.attributes() & changed) | (shapeAttributes & ~changed);
        if (desc.hasGetterObject())
            getter = desc.getter();
        else
            getter = shapeHasGetterValue ? shape->getter() : nullptr;
        if (desc.hasSetterObject())
            setter = desc.setter();
        else
            setter = shapeHasSetterValue ? shape->setter() : nullptr;
    }

    








    if (callDelProperty) {
        ObjectOpResult ignored;
        if (!CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, id, ignored))
            return false;
    }

    return NativeDefineProperty(cx, obj, id, v, getter, setter, attrs, result);
}


static bool
DefinePropertyOnArray(JSContext* cx, Handle<ArrayObject*> arr, HandleId id,
                      Handle<PropertyDescriptor> desc, ObjectOpResult& result)
{
    
    if (id == NameToId(cx->names().length)) {
        
        
        
        
        
        
        
        RootedValue v(cx);
        if (desc.hasValue()) {
            uint32_t newLen;
            if (!CanonicalizeArrayLengthValue(cx, desc.value(), &newLen))
                return false;
            v.setNumber(newLen);
        } else {
            v.setNumber(arr->length());
        }

        if (desc.hasConfigurable() && desc.configurable())
            return result.fail(JSMSG_CANT_REDEFINE_PROP);
        if (desc.hasEnumerable() && desc.enumerable())
            return result.fail(JSMSG_CANT_REDEFINE_PROP);

        if (desc.isAccessorDescriptor())
            return result.fail(JSMSG_CANT_REDEFINE_PROP);

        unsigned attrs = arr->lookup(cx, id)->attributes();
        if (!arr->lengthIsWritable()) {
            if (desc.hasWritable() && desc.writable())
                return result.fail(JSMSG_CANT_REDEFINE_PROP);
        } else {
            if (desc.hasWritable() && !desc.writable())
                attrs = attrs | JSPROP_READONLY;
        }

        return ArraySetLength(cx, arr, id, attrs, v, result);
    }

    
    uint32_t index;
    if (IdIsIndex(id, &index)) {
        
        uint32_t oldLen = arr->length();

        
        if (index >= oldLen && !arr->lengthIsWritable())
            return result.fail(JSMSG_CANT_APPEND_TO_ARRAY);

        
        return DefinePropertyOnObject(cx, arr, id, desc, result);
    }

    
    return DefinePropertyOnObject(cx, arr, id, desc, result);
}


static bool
DefinePropertyOnTypedArray(JSContext* cx, HandleObject obj, HandleId id,
                           Handle<PropertyDescriptor> desc, ObjectOpResult& result)
{
    MOZ_ASSERT(IsAnyTypedArray(obj));
    
    uint64_t index;
    if (IsTypedArrayIndex(id, &index))
        return DefineTypedArrayElement(cx, obj, index, desc, result);

    
    return DefinePropertyOnObject(cx, obj.as<NativeObject>(), id, desc, result);
}

bool
js::StandardDefineProperty(JSContext* cx, HandleObject obj, HandleId id,
                           Handle<PropertyDescriptor> desc, ObjectOpResult& result)
{
    if (obj->is<ArrayObject>()) {
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        return DefinePropertyOnArray(cx, arr, id, desc, result);
    }

    if (IsAnyTypedArray(obj))
        return DefinePropertyOnTypedArray(cx, obj, id, desc, result);

    if (obj->is<UnboxedPlainObject>() && !UnboxedPlainObject::convertToNative(cx, obj))
        return false;

    if (obj->getOps()->lookupProperty) {
        if (obj->is<ProxyObject>()) {
            Rooted<PropertyDescriptor> pd(cx, desc);
            pd.object().set(obj);
            return Proxy::defineProperty(cx, obj, id, pd, result);
        }
        return result.fail(JSMSG_OBJECT_NOT_EXTENSIBLE);
    }

    return DefinePropertyOnObject(cx, obj.as<NativeObject>(), id, desc, result);
}

bool
js::StandardDefineProperty(JSContext* cx, HandleObject obj, HandleId id,
                           Handle<PropertyDescriptor> desc)
{
    ObjectOpResult success;
    return StandardDefineProperty(cx, obj, id, desc, success) &&
           success.checkStrict(cx, obj, id);
}

bool
CheckCallable(JSContext* cx, JSObject* obj, const char* fieldName)
{
    if (obj && !obj->isCallable()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_GET_SET_FIELD,
                             fieldName);
        return false;
    }
    return true;
}

bool
js::ToPropertyDescriptor(JSContext* cx, HandleValue descval, bool checkAccessors,
                         MutableHandle<PropertyDescriptor> desc)
{
    
    if (!descval.isObject()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT,
                             InformalValueTypeName(descval));
        return false;
    }
    RootedObject obj(cx, &descval.toObject());

    
    desc.clear();

    bool found = false;
    RootedId id(cx);
    RootedValue v(cx);
    unsigned attrs = 0;

    
    id = NameToId(cx->names().enumerable);
    if (!GetPropertyIfPresent(cx, obj, id, &v, &found))
        return false;
    if (found) {
        if (ToBoolean(v))
            attrs |= JSPROP_ENUMERATE;
    } else {
        attrs |= JSPROP_IGNORE_ENUMERATE;
    }

    
    id = NameToId(cx->names().configurable);
    if (!GetPropertyIfPresent(cx, obj, id, &v, &found))
        return false;
    if (found) {
        if (!ToBoolean(v))
            attrs |= JSPROP_PERMANENT;
    } else {
        attrs |= JSPROP_IGNORE_PERMANENT;
    }

    
    id = NameToId(cx->names().value);
    if (!GetPropertyIfPresent(cx, obj, id, &v, &found))
        return false;
    if (found)
        desc.value().set(v);
    else
        attrs |= JSPROP_IGNORE_VALUE;

    
    id = NameToId(cx->names().writable);
    if (!GetPropertyIfPresent(cx, obj, id, &v, &found))
        return false;
    if (found) {
        if (!ToBoolean(v))
            attrs |= JSPROP_READONLY;
    } else {
        attrs |= JSPROP_IGNORE_READONLY;
    }

    
    bool hasGetOrSet;
    id = NameToId(cx->names().get);
    if (!GetPropertyIfPresent(cx, obj, id, &v, &found))
        return false;
    hasGetOrSet = found;
    if (found) {
        if (v.isObject()) {
            if (checkAccessors && !CheckCallable(cx, &v.toObject(), js_getter_str))
                return false;
            desc.setGetterObject(&v.toObject());
        } else if (!v.isUndefined()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_GET_SET_FIELD,
                                 js_getter_str);
            return false;
        }
        attrs |= JSPROP_GETTER | JSPROP_SHARED;
    }

    
    id = NameToId(cx->names().set);
    if (!GetPropertyIfPresent(cx, obj, id, &v, &found))
        return false;
    hasGetOrSet |= found;
    if (found) {
        if (v.isObject()) {
            if (checkAccessors && !CheckCallable(cx, &v.toObject(), js_setter_str))
                return false;
            desc.setSetterObject(&v.toObject());
        } else if (!v.isUndefined()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_GET_SET_FIELD,
                                 js_setter_str);
            return false;
        }
        attrs |= JSPROP_SETTER | JSPROP_SHARED;
    }

    
    if (hasGetOrSet) {
        if (!(attrs & JSPROP_IGNORE_READONLY) || !(attrs & JSPROP_IGNORE_VALUE)) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_INVALID_DESCRIPTOR);
            return false;
        }

        
        attrs &= ~(JSPROP_IGNORE_READONLY | JSPROP_IGNORE_VALUE);
    }

    desc.setAttributes(attrs);
    MOZ_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    MOZ_ASSERT_IF(attrs & (JSPROP_GETTER | JSPROP_SETTER), attrs & JSPROP_SHARED);
    return true;
}

bool
js::CheckPropertyDescriptorAccessors(JSContext* cx, Handle<PropertyDescriptor> desc)
{
    if (desc.hasGetterObject()) {
        if (!CheckCallable(cx, desc.getterObject(), js_getter_str))
            return false;
    }
    if (desc.hasSetterObject()) {
        if (!CheckCallable(cx, desc.setterObject(), js_setter_str))
            return false;
    }
    return true;
}

void
js::CompletePropertyDescriptor(MutableHandle<PropertyDescriptor> desc)
{
    if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
        if (!desc.hasValue())
            desc.value().setUndefined();
        if (!desc.hasWritable())
            desc.attributesRef() |= JSPROP_READONLY;
        desc.attributesRef() &= ~(JSPROP_IGNORE_READONLY | JSPROP_IGNORE_VALUE);
    } else {
        if (!desc.hasGetterObject())
            desc.setGetterObject(nullptr);
        if (!desc.hasSetterObject())
            desc.setSetterObject(nullptr);
        desc.attributesRef() |= JSPROP_GETTER | JSPROP_SETTER | JSPROP_SHARED;
    }
    if (!desc.hasEnumerable())
        desc.attributesRef() &= ~JSPROP_ENUMERATE;
    if (!desc.hasConfigurable())
        desc.attributesRef() |= JSPROP_PERMANENT;
    desc.attributesRef() &= ~(JSPROP_IGNORE_PERMANENT | JSPROP_IGNORE_ENUMERATE);
}

bool
js::ReadPropertyDescriptors(JSContext* cx, HandleObject props, bool checkAccessors,
                            AutoIdVector* ids, AutoPropertyDescriptorVector* descs)
{
    if (!GetPropertyKeys(cx, props, JSITER_OWNONLY | JSITER_SYMBOLS, ids))
        return false;

    RootedId id(cx);
    for (size_t i = 0, len = ids->length(); i < len; i++) {
        id = (*ids)[i];
        Rooted<PropertyDescriptor> desc(cx);
        RootedValue v(cx);
        if (!GetProperty(cx, props, props, id, &v) ||
            !ToPropertyDescriptor(cx, v, checkAccessors, &desc) ||
            !descs->append(desc))
        {
            return false;
        }
    }
    return true;
}

bool
js::DefineProperties(JSContext* cx, HandleObject obj, HandleObject props)
{
    AutoIdVector ids(cx);
    AutoPropertyDescriptorVector descs(cx);
    if (!ReadPropertyDescriptors(cx, props, true, &ids, &descs))
        return false;

    for (size_t i = 0, len = ids.length(); i < len; i++) {
        if (!StandardDefineProperty(cx, obj, ids[i], descs[i]))
            return false;
    }

    return true;
}



static unsigned
GetSealedOrFrozenAttributes(unsigned attrs, IntegrityLevel level)
{
    
    if (level == IntegrityLevel::Frozen && !(attrs & (JSPROP_GETTER | JSPROP_SETTER)))
        return JSPROP_PERMANENT | JSPROP_READONLY;
    return JSPROP_PERMANENT;
}


bool
js::SetIntegrityLevel(JSContext* cx, HandleObject obj, IntegrityLevel level)
{
    assertSameCompartment(cx, obj);

    
    if (!PreventExtensions(cx, obj))
        return false;

    
    AutoIdVector keys(cx);
    if (!GetPropertyKeys(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY | JSITER_SYMBOLS, &keys))
        return false;

    
    
    MOZ_ASSERT_IF(obj->isNative(), obj->as<NativeObject>().getDenseCapacity() == 0);

    
    if (obj->isNative() && !obj->as<NativeObject>().inDictionaryMode() && !IsAnyTypedArray(obj)) {
        HandleNativeObject nobj = obj.as<NativeObject>();

        
        
        
        
        
        RootedShape last(cx, EmptyShape::getInitialShape(cx, nobj->getClass(),
                                                         nobj->getTaggedProto(),
                                                         nobj->numFixedSlots(),
                                                         nobj->lastProperty()->getObjectFlags()));
        if (!last)
            return false;

        
        AutoShapeVector shapes(cx);
        for (Shape::Range<NoGC> r(nobj->lastProperty()); !r.empty(); r.popFront()) {
            if (!shapes.append(&r.front()))
                return false;
        }
        Reverse(shapes.begin(), shapes.end());

        for (size_t i = 0; i < shapes.length(); i++) {
            StackShape unrootedChild(shapes[i]);
            RootedGeneric<StackShape*> child(cx, &unrootedChild);
            child->attrs |= GetSealedOrFrozenAttributes(child->attrs, level);

            if (!JSID_IS_EMPTY(child->propid) && level == IntegrityLevel::Frozen)
                MarkTypePropertyNonWritable(cx, nobj, child->propid);

            last = cx->compartment()->propertyTree.getChild(cx, last, *child);
            if (!last)
                return false;
        }

        MOZ_ASSERT(nobj->lastProperty()->slotSpan() == last->slotSpan());
        JS_ALWAYS_TRUE(nobj->setLastProperty(cx, last));
    } else {
        RootedId id(cx);
        Rooted<PropertyDescriptor> desc(cx);

        const unsigned AllowConfigure = JSPROP_IGNORE_ENUMERATE | JSPROP_IGNORE_READONLY |
                                        JSPROP_IGNORE_VALUE;
        const unsigned AllowConfigureAndWritable = AllowConfigure & ~JSPROP_IGNORE_READONLY;

        
        for (size_t i = 0; i < keys.length(); i++) {
            id = keys[i];

            if (level == IntegrityLevel::Sealed) {
                
                desc.setAttributes(AllowConfigure | JSPROP_PERMANENT);
            } else {
                
                Rooted<PropertyDescriptor> currentDesc(cx);
                if (!GetOwnPropertyDescriptor(cx, obj, id, &currentDesc))
                    return false;

                
                if (!currentDesc.object())
                    continue;

                
                if (currentDesc.isAccessorDescriptor())
                    desc.setAttributes(AllowConfigure | JSPROP_PERMANENT);
                else
                    desc.setAttributes(AllowConfigureAndWritable | JSPROP_PERMANENT | JSPROP_READONLY);
            }

            desc.object().set(obj);

            
            if (!StandardDefineProperty(cx, obj, id, desc))
                return false;
        }
    }

    
    
    
    
    
    
    
    
    
    if (level == IntegrityLevel::Frozen && obj->is<ArrayObject>()) {
        if (!obj->as<ArrayObject>().maybeCopyElementsForWrite(cx))
            return false;
        obj->as<ArrayObject>().getElementsHeader()->setNonwritableArrayLength();
    }

    return true;
}


bool
js::TestIntegrityLevel(JSContext* cx, HandleObject obj, IntegrityLevel level, bool* result)
{
    
    bool status;
    if (!IsExtensible(cx, obj, &status))
        return false;
    if (status) {
        *result = false;
        return true;
    }

    
    AutoIdVector props(cx);
    if (!GetPropertyKeys(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY | JSITER_SYMBOLS, &props))
        return false;

    
    RootedId id(cx);
    Rooted<PropertyDescriptor> desc(cx);
    for (size_t i = 0, len = props.length(); i < len; i++) {
        id = props[i];

        
        if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
            return false;

        
        if (!desc.object())
            continue;

        
        if (desc.configurable() ||
            (level == IntegrityLevel::Frozen && desc.isDataDescriptor() && desc.writable()))
        {
            *result = false;
            return true;
        }
    }

    
    *result = true;
    return true;
}








static inline gc::AllocKind
NewObjectGCKind(const js::Class* clasp)
{
    if (clasp == &ArrayObject::class_)
        return gc::AllocKind::OBJECT8;
    if (clasp == &JSFunction::class_)
        return gc::AllocKind::OBJECT2;
    return gc::AllocKind::OBJECT4;
}

static inline JSObject*
NewObject(ExclusiveContext* cx, HandleObjectGroup group, gc::AllocKind kind,
          NewObjectKind newKind)
{
    const Class* clasp = group->clasp();

    MOZ_ASSERT(clasp != &ArrayObject::class_);
    MOZ_ASSERT_IF(clasp == &JSFunction::class_,
                  kind == JSFunction::FinalizeKind || kind == JSFunction::ExtendedFinalizeKind);

    
    
    
    size_t nfixed = ClassCanHaveFixedData(clasp)
                    ? GetGCKindSlots(gc::GetGCObjectKind(clasp), clasp)
                    : GetGCKindSlots(kind, clasp);

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, clasp, group->proto(), nfixed));
    if (!shape)
        return nullptr;

    gc::InitialHeap heap = GetInitialHeap(newKind, clasp);
    JSObject* obj = JSObject::create(cx, kind, heap, shape, group);
    if (!obj)
        return nullptr;

    if (newKind == SingletonObject) {
        RootedObject nobj(cx, obj);
        if (!JSObject::setSingleton(cx, nobj))
            return nullptr;
        obj = nobj;
    }

    bool globalWithoutCustomTrace = clasp->trace == JS_GlobalObjectTraceHook &&
                                    !cx->compartment()->options().getTrace();
    if (clasp->trace && !globalWithoutCustomTrace)
        MOZ_RELEASE_ASSERT(clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS);

    probes::CreateObject(cx, obj);
    return obj;
}

void
NewObjectCache::fillProto(EntryIndex entry, const Class* clasp, js::TaggedProto proto,
                          gc::AllocKind kind, NativeObject* obj)
{
    MOZ_ASSERT_IF(proto.isObject(), !proto.toObject()->is<GlobalObject>());
    MOZ_ASSERT(obj->getTaggedProto() == proto);
    return fill(entry, clasp, proto.raw(), kind, obj);
}

static bool
NewObjectWithTaggedProtoIsCachable(ExclusiveContext* cxArg, Handle<TaggedProto> proto,
                                   NewObjectKind newKind, const Class* clasp)
{
    return cxArg->isJSContext() &&
           proto.isObject() &&
           newKind == GenericObject &&
           clasp->isNative() &&
           !proto.toObject()->is<GlobalObject>();
}

JSObject*
js::NewObjectWithGivenTaggedProto(ExclusiveContext* cxArg, const Class* clasp,
                                  Handle<TaggedProto> proto,
                                  gc::AllocKind allocKind, NewObjectKind newKind)
{
    if (CanBeFinalizedInBackground(allocKind, clasp))
        allocKind = GetBackgroundAllocKind(allocKind);

    bool isCachable = NewObjectWithTaggedProtoIsCachable(cxArg, proto, newKind, clasp);
    if (isCachable) {
        JSContext* cx = cxArg->asJSContext();
        JSRuntime* rt = cx->runtime();
        NewObjectCache& cache = rt->newObjectCache;
        NewObjectCache::EntryIndex entry = -1;
        if (cache.lookupProto(clasp, proto.toObject(), allocKind, &entry)) {
            JSObject* obj = cache.newObjectFromHit(cx, entry, GetInitialHeap(newKind, clasp));
            if (obj)
                return obj;
        }
    }

    RootedObjectGroup group(cxArg, ObjectGroup::defaultNewGroup(cxArg, clasp, proto, nullptr));
    if (!group)
        return nullptr;

    RootedObject obj(cxArg, NewObject(cxArg, group, allocKind, newKind));
    if (!obj)
        return nullptr;

    if (isCachable && !obj->as<NativeObject>().hasDynamicSlots()) {
        NewObjectCache& cache = cxArg->asJSContext()->runtime()->newObjectCache;
        NewObjectCache::EntryIndex entry = -1;
        cache.lookupProto(clasp, proto.toObject(), allocKind, &entry);
        cache.fillProto(entry, clasp, proto, allocKind, &obj->as<NativeObject>());
    }

    return obj;
}

static bool
NewObjectIsCachable(ExclusiveContext* cxArg, NewObjectKind newKind, const Class* clasp)
{
    return cxArg->isJSContext() &&
           newKind == GenericObject &&
           clasp->isNative();
}

JSObject*
js::NewObjectWithClassProtoCommon(ExclusiveContext* cxArg, const Class* clasp,
                                  HandleObject protoArg,
                                  gc::AllocKind allocKind, NewObjectKind newKind)
{
    if (protoArg) {
        return NewObjectWithGivenTaggedProto(cxArg, clasp, AsTaggedProto(protoArg),
                                             allocKind, newKind);
    }

    if (CanBeFinalizedInBackground(allocKind, clasp))
        allocKind = GetBackgroundAllocKind(allocKind);

    Handle<GlobalObject*> global = cxArg->global();

    bool isCachable = NewObjectIsCachable(cxArg, newKind, clasp);
    if (isCachable) {
        JSContext* cx = cxArg->asJSContext();
        JSRuntime* rt = cx->runtime();
        NewObjectCache& cache = rt->newObjectCache;
        NewObjectCache::EntryIndex entry = -1;
        if (cache.lookupGlobal(clasp, global, allocKind, &entry)) {
            JSObject* obj = cache.newObjectFromHit(cx, entry, GetInitialHeap(newKind, clasp));
            if (obj)
                return obj;
        }
    }

    



    JSProtoKey protoKey = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (protoKey == JSProto_Null)
        protoKey = JSProto_Object;

    RootedObject proto(cxArg, protoArg);
    if (!GetBuiltinPrototype(cxArg, protoKey, &proto))
        return nullptr;

    Rooted<TaggedProto> taggedProto(cxArg, TaggedProto(proto));
    RootedObjectGroup group(cxArg, ObjectGroup::defaultNewGroup(cxArg, clasp, taggedProto));
    if (!group)
        return nullptr;

    JSObject* obj = NewObject(cxArg, group, allocKind, newKind);
    if (!obj)
        return nullptr;

    if (isCachable && !obj->as<NativeObject>().hasDynamicSlots()) {
        NewObjectCache& cache = cxArg->asJSContext()->runtime()->newObjectCache;
        NewObjectCache::EntryIndex entry = -1;
        cache.lookupGlobal(clasp, global, allocKind, &entry);
        cache.fillGlobal(entry, clasp, global, allocKind,
                         &obj->as<NativeObject>());
    }

    return obj;
}

static bool
NewObjectWithGroupIsCachable(ExclusiveContext* cx, HandleObjectGroup group,
                             NewObjectKind newKind)
{
    return group->proto().isObject() &&
           newKind == GenericObject &&
           group->clasp()->isNative() &&
           (!group->newScript() || group->newScript()->analyzed()) &&
           cx->isJSContext();
}





JSObject*
js::NewObjectWithGroupCommon(ExclusiveContext* cx, HandleObjectGroup group,
                             gc::AllocKind allocKind, NewObjectKind newKind)
{
    MOZ_ASSERT(gc::IsObjectAllocKind(allocKind));
    if (CanBeFinalizedInBackground(allocKind, group->clasp()))
        allocKind = GetBackgroundAllocKind(allocKind);

    bool isCachable = NewObjectWithGroupIsCachable(cx, group, newKind);
    if (isCachable) {
        NewObjectCache& cache = cx->asJSContext()->runtime()->newObjectCache;
        NewObjectCache::EntryIndex entry = -1;
        if (cache.lookupGroup(group, allocKind, &entry)) {
            JSObject* obj = cache.newObjectFromHit(cx->asJSContext(), entry,
                                                   GetInitialHeap(newKind, group->clasp()));
            if (obj)
                return obj;
        }
    }

    JSObject* obj = NewObject(cx, group, allocKind, newKind);
    if (!obj)
        return nullptr;

    if (isCachable && !obj->as<NativeObject>().hasDynamicSlots()) {
        NewObjectCache& cache = cx->asJSContext()->runtime()->newObjectCache;
        NewObjectCache::EntryIndex entry = -1;
        cache.lookupGroup(group, allocKind, &entry);
        cache.fillGroup(entry, group, allocKind, &obj->as<NativeObject>());
    }

    return obj;
}

bool
js::NewObjectScriptedCall(JSContext* cx, MutableHandleObject pobj)
{
    jsbytecode* pc;
    RootedScript script(cx, cx->currentScript(&pc));
    gc::AllocKind allocKind = NewObjectGCKind(&PlainObject::class_);
    NewObjectKind newKind = GenericObject;
    if (script && ObjectGroup::useSingletonForAllocationSite(script, pc, &PlainObject::class_))
        newKind = SingletonObject;
    RootedObject obj(cx, NewBuiltinClassInstance<PlainObject>(cx, allocKind, newKind));
    if (!obj)
        return false;

    if (script) {
        
        if (!ObjectGroup::setAllocationSiteObjectGroup(cx, script, pc, obj, newKind == SingletonObject))
            return false;
    }

    pobj.set(obj);
    return true;
}

JSObject*
js::CreateThis(JSContext* cx, const Class* newclasp, HandleObject callee)
{
    RootedValue protov(cx);
    if (!GetProperty(cx, callee, callee, cx->names().prototype, &protov))
        return nullptr;

    RootedObject proto(cx, protov.isObjectOrNull() ? protov.toObjectOrNull() : nullptr);
    gc::AllocKind kind = NewObjectGCKind(newclasp);
    return NewObjectWithClassProto(cx, newclasp, proto, kind);
}

static inline JSObject*
CreateThisForFunctionWithGroup(JSContext* cx, HandleObjectGroup group,
                               NewObjectKind newKind)
{
    if (group->maybeUnboxedLayout() && newKind != SingletonObject)
        return UnboxedPlainObject::create(cx, group, newKind);

    if (TypeNewScript* newScript = group->newScript()) {
        if (newScript->analyzed()) {
            
            
            
            RootedPlainObject templateObject(cx, newScript->templateObject());
            MOZ_ASSERT(templateObject->group() == group);

            RootedPlainObject res(cx, CopyInitializerObject(cx, templateObject, newKind));
            if (!res)
                return nullptr;

            if (newKind == SingletonObject) {
                Rooted<TaggedProto> proto(cx, TaggedProto(templateObject->getProto()));
                if (!res->splicePrototype(cx, &PlainObject::class_, proto))
                    return nullptr;
            } else {
                res->setGroup(group);
            }
            return res;
        }

        
        
        if (newKind == GenericObject)
            newKind = TenuredObject;

        
        
        
        gc::AllocKind allocKind = GuessObjectGCKind(NativeObject::MAX_FIXED_SLOTS);
        PlainObject* res = NewObjectWithGroup<PlainObject>(cx, group, allocKind, newKind);
        if (!res)
            return nullptr;

        if (newKind != SingletonObject)
            newScript->registerNewObject(res);

        return res;
    }

    gc::AllocKind allocKind = NewObjectGCKind(&PlainObject::class_);

    if (newKind == SingletonObject) {
        Rooted<TaggedProto> protoRoot(cx, group->proto());
        return NewObjectWithGivenTaggedProto(cx, &PlainObject::class_, protoRoot, allocKind, newKind);
    }
    return NewObjectWithGroup<PlainObject>(cx, group, allocKind, newKind);
}

JSObject*
js::CreateThisForFunctionWithProto(JSContext* cx, HandleObject callee, HandleObject proto,
                                   NewObjectKind newKind )
{
    RootedObject res(cx);

    if (proto) {
        RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, nullptr, TaggedProto(proto),
                                                                 &callee->as<JSFunction>()));
        if (!group)
            return nullptr;

        if (group->newScript() && !group->newScript()->analyzed()) {
            bool regenerate;
            if (!group->newScript()->maybeAnalyze(cx, group, &regenerate))
                return nullptr;
            if (regenerate) {
                
                
                group = ObjectGroup::defaultNewGroup(cx, nullptr, TaggedProto(proto),
                                                     &callee->as<JSFunction>());
                MOZ_ASSERT(group && group->newScript());
            }
        }

        res = CreateThisForFunctionWithGroup(cx, group, newKind);
    } else {
        gc::AllocKind allocKind = NewObjectGCKind(&PlainObject::class_);
        res = NewObjectWithProto<PlainObject>(cx, proto, allocKind, newKind);
    }

    if (res) {
        JSScript* script = callee->as<JSFunction>().getOrCreateScript(cx);
        if (!script)
            return nullptr;
        TypeScript::SetThis(cx, script, TypeSet::ObjectType(res));
    }

    return res;
}

JSObject*
js::CreateThisForFunction(JSContext* cx, HandleObject callee, NewObjectKind newKind)
{
    RootedValue protov(cx);
    if (!GetProperty(cx, callee, callee, cx->names().prototype, &protov))
        return nullptr;
    RootedObject proto(cx);
    if (protov.isObject())
        proto = &protov.toObject();
    JSObject* obj = CreateThisForFunctionWithProto(cx, callee, proto, newKind);

    if (obj && newKind == SingletonObject) {
        RootedPlainObject nobj(cx, &obj->as<PlainObject>());

        
        NativeObject::clear(cx, nobj);

        JSScript* calleeScript = callee->as<JSFunction>().nonLazyScript();
        TypeScript::SetThis(cx, calleeScript, TypeSet::ObjectType(nobj));

        return nobj;
    }

    return obj;
}

 bool
JSObject::nonNativeSetProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                               HandleValue receiver, ObjectOpResult& result)
{
    RootedValue value(cx, v);
    if (MOZ_UNLIKELY(obj->watched())) {
        WatchpointMap* wpmap = cx->compartment()->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, &value))
            return false;
    }
    return obj->getOps()->setProperty(cx, obj, id, value, receiver, result);
}

 bool
JSObject::nonNativeSetElement(JSContext* cx, HandleObject obj, uint32_t index, HandleValue v,
                              HandleValue receiver, ObjectOpResult& result)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return nonNativeSetProperty(cx, obj, id, v, receiver, result);
}

JS_FRIEND_API(bool)
JS_CopyPropertyFrom(JSContext* cx, HandleId id, HandleObject target,
                    HandleObject obj, PropertyCopyBehavior copyBehavior)
{
    
    assertSameCompartment(cx, obj, id);
    Rooted<JSPropertyDescriptor> desc(cx);

    if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;
    MOZ_ASSERT(desc.object());

    
    if (desc.getter() && !desc.hasGetterObject())
        return true;
    if (desc.setter() && !desc.hasSetterObject())
        return true;

    if (copyBehavior == MakeNonConfigurableIntoConfigurable) {
        
        desc.attributesRef() &= ~JSPROP_PERMANENT;
    }

    JSAutoCompartment ac(cx, target);
    RootedId wrappedId(cx, id);
    if (!cx->compartment()->wrap(cx, &desc))
        return false;

    return StandardDefineProperty(cx, target, wrappedId, desc);
}

JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext* cx, HandleObject target, HandleObject obj)
{
    JSAutoCompartment ac(cx, obj);

    AutoIdVector props(cx);
    if (!GetPropertyKeys(cx, obj, JSITER_OWNONLY | JSITER_HIDDEN | JSITER_SYMBOLS, &props))
        return false;

    for (size_t i = 0; i < props.length(); ++i) {
        if (!JS_CopyPropertyFrom(cx, props[i], target, obj))
            return false;
    }

    return true;
}

static bool
CopyProxyObject(JSContext* cx, Handle<ProxyObject*> from, Handle<ProxyObject*> to)
{
    MOZ_ASSERT(from->getClass() == to->getClass());

    if (from->is<WrapperObject>() &&
        (Wrapper::wrapperHandler(from)->flags() &
         Wrapper::CROSS_COMPARTMENT))
    {
        to->setCrossCompartmentPrivate(GetProxyPrivate(from));
    } else {
        RootedValue v(cx, GetProxyPrivate(from));
        if (!cx->compartment()->wrap(cx, &v))
            return false;
        to->setSameCompartmentPrivate(v);
    }

    RootedValue v(cx);
    for (size_t n = 0; n < PROXY_EXTRA_SLOTS; n++) {
        v = GetProxyExtra(from, n);
        if (!cx->compartment()->wrap(cx, &v))
            return false;
        SetProxyExtra(to, n, v);
    }

    return true;
}

JSObject*
js::CloneObject(JSContext* cx, HandleObject obj, Handle<js::TaggedProto> proto)
{
    if (!obj->isNative() && !obj->is<ProxyObject>()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_CANT_CLONE_OBJECT);
        return nullptr;
    }

    RootedObject clone(cx);
    if (obj->isNative()) {
        clone = NewObjectWithGivenTaggedProto(cx, obj->getClass(), proto);
        if (!clone)
            return nullptr;

        if (clone->is<JSFunction>() && (obj->compartment() != clone->compartment())) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                                 JSMSG_CANT_CLONE_OBJECT);
            return nullptr;
        }

        if (obj->as<NativeObject>().hasPrivate())
            clone->as<NativeObject>().setPrivate(obj->as<NativeObject>().getPrivate());
    } else {
        ProxyOptions options;
        options.setClass(obj->getClass());

        clone = ProxyObject::New(cx, GetProxyHandler(obj), JS::NullHandleValue, proto, options);
        if (!clone)
            return nullptr;

        if (!CopyProxyObject(cx, obj.as<ProxyObject>(), clone.as<ProxyObject>()))
            return nullptr;
    }

    return clone;
}

static bool
GetScriptArrayObjectElements(JSContext* cx, HandleArrayObject obj, AutoValueVector& values)
{
    MOZ_ASSERT(!obj->isSingleton());

    if (obj->nonProxyIsExtensible()) {
        MOZ_ASSERT(obj->slotSpan() == 0);

        if (!values.appendN(MagicValue(JS_ELEMENTS_HOLE), obj->getDenseInitializedLength()))
            return false;

        for (size_t i = 0; i < obj->getDenseInitializedLength(); i++)
            values[i].set(obj->getDenseElement(i));
    } else {
        
        

        for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront()) {
            Shape& shape = r.front();
            if (shape.propid() == NameToId(cx->names().length))
                continue;
            MOZ_ASSERT(shape.isDataDescriptor());

            
            
            
            
            if (shape.propid() == NameToId(cx->names().raw))
                continue;

            uint32_t index = JSID_TO_INT(shape.propid());
            while (index >= values.length()) {
                if (!values.append(MagicValue(JS_ELEMENTS_HOLE)))
                    return false;
            }

            values[index].set(obj->getSlot(shape.slot()));
        }
    }

    return true;
}

static bool
GetScriptPlainObjectProperties(JSContext* cx, HandleObject obj, AutoIdValueVector& properties)
{
    if (obj->is<PlainObject>()) {
        PlainObject* nobj = &obj->as<PlainObject>();

        if (!properties.appendN(IdValuePair(), nobj->slotSpan()))
            return false;

        for (Shape::Range<NoGC> r(nobj->lastProperty()); !r.empty(); r.popFront()) {
            Shape& shape = r.front();
            MOZ_ASSERT(shape.isDataDescriptor());
            uint32_t slot = shape.slot();
            properties[slot].get().id = shape.propid();
            properties[slot].get().value = nobj->getSlot(slot);
        }

        for (size_t i = 0; i < nobj->getDenseInitializedLength(); i++) {
            Value v = nobj->getDenseElement(i);
            if (!v.isMagic(JS_ELEMENTS_HOLE) && !properties.append(IdValuePair(INT_TO_JSID(i), v)))
                return false;
        }

        return true;
    }

    if (obj->is<UnboxedPlainObject>()) {
        UnboxedPlainObject* nobj = &obj->as<UnboxedPlainObject>();

        const UnboxedLayout& layout = nobj->layout();
        if (!properties.appendN(IdValuePair(), layout.properties().length()))
            return false;

        for (size_t i = 0; i < layout.properties().length(); i++) {
            const UnboxedLayout::Property& property = layout.properties()[i];
            properties[i].get().id = NameToId(property.name);
            properties[i].get().value = nobj->getValue(property);
        }

        return true;
    }

    MOZ_CRASH("Bad object kind");
}

static bool
DeepCloneValue(JSContext* cx, Value* vp, NewObjectKind newKind)
{
    if (vp->isObject()) {
        RootedObject obj(cx, &vp->toObject());
        obj = DeepCloneObjectLiteral(cx, obj, newKind);
        if (!obj)
            return false;
        vp->setObject(*obj);
    }
    return true;
}

JSObject*
js::DeepCloneObjectLiteral(JSContext* cx, HandleObject obj, NewObjectKind newKind)
{
    
    MOZ_ASSERT_IF(obj->isSingleton(),
                  JS::CompartmentOptionsRef(cx).getSingletonsAsTemplates());
    MOZ_ASSERT(obj->is<PlainObject>() || obj->is<UnboxedPlainObject>() || obj->is<ArrayObject>());
    MOZ_ASSERT(cx->isInsideCurrentCompartment(obj));
    MOZ_ASSERT(newKind != SingletonObject);

    if (obj->is<ArrayObject>()) {
        HandleArrayObject aobj = obj.as<ArrayObject>();

        AutoValueVector values(cx);
        if (!GetScriptArrayObjectElements(cx, aobj, values))
            return nullptr;

        
        uint32_t initialized = aobj->getDenseInitializedLength();
        for (uint32_t i = 0; i < initialized; ++i) {
            if (!DeepCloneValue(cx, values[i].address(), newKind))
                return nullptr;
        }

        RootedArrayObject clone(cx, NewDenseUnallocatedArray(cx, aobj->length(),
                                                             NullPtr(), newKind));
        if (!clone || !clone->ensureElements(cx, values.length()))
            return nullptr;

        clone->setDenseInitializedLength(values.length());
        clone->initDenseElements(0, values.begin(), values.length());

        if (aobj->denseElementsAreCopyOnWrite()) {
            if (!ObjectElements::MakeElementsCopyOnWrite(cx, clone))
                return nullptr;
        } else {
            ObjectGroup::fixArrayGroup(cx, &clone->as<ArrayObject>());
        }

        return clone;
    }

    AutoIdValueVector properties(cx);
    if (!GetScriptPlainObjectProperties(cx, obj, properties))
        return nullptr;

    for (size_t i = 0; i < properties.length(); i++) {
        if (!DeepCloneValue(cx, &properties[i].get().value, newKind))
            return nullptr;
    }

    if (obj->isSingleton())
        newKind = SingletonObject;

    return ObjectGroup::newPlainObject(cx, properties.begin(), properties.length(), newKind);
}

static bool
InitializePropertiesFromCompatibleNativeObject(JSContext* cx,
                                               HandleNativeObject dst,
                                               HandleNativeObject src)
{
    assertSameCompartment(cx, src, dst);
    MOZ_ASSERT(src->getClass() == dst->getClass());
    MOZ_ASSERT(dst->lastProperty()->getObjectFlags() == 0);
    MOZ_ASSERT(!src->isSingleton());
    MOZ_ASSERT(src->numFixedSlots() == dst->numFixedSlots());

    if (!dst->ensureElements(cx, src->getDenseInitializedLength()))
        return false;

    uint32_t initialized = src->getDenseInitializedLength();
    for (uint32_t i = 0; i < initialized; ++i) {
        dst->setDenseInitializedLength(i + 1);
        dst->initDenseElement(i, src->getDenseElement(i));
    }

    MOZ_ASSERT(!src->hasPrivate());
    RootedShape shape(cx);
    if (src->getProto() == dst->getProto()) {
        shape = src->lastProperty();
    } else {
        
        
        
        shape = EmptyShape::getInitialShape(cx, dst->getClass(), dst->getTaggedProto(),
                                            dst->numFixedSlots(), 0);
        if (!shape)
            return false;

        
        AutoShapeVector shapes(cx);
        for (Shape::Range<NoGC> r(src->lastProperty()); !r.empty(); r.popFront()) {
            if (!shapes.append(&r.front()))
                return false;
        }
        Reverse(shapes.begin(), shapes.end());

        for (size_t i = 0; i < shapes.length(); i++) {
            StackShape unrootedChild(shapes[i]);
            RootedGeneric<StackShape*> child(cx, &unrootedChild);
            shape = cx->compartment()->propertyTree.getChild(cx, shape, *child);
            if (!shape)
                return false;
        }
    }
    size_t span = shape->slotSpan();
    if (!dst->setLastProperty(cx, shape))
        return false;
    for (size_t i = JSCLASS_RESERVED_SLOTS(src->getClass()); i < span; i++)
        dst->setSlot(i, src->getSlot(i));

    return true;
}

JS_FRIEND_API(bool)
JS_InitializePropertiesFromCompatibleNativeObject(JSContext* cx,
                                                  HandleObject dst,
                                                  HandleObject src)
{
    return InitializePropertiesFromCompatibleNativeObject(cx,
                                                          dst.as<NativeObject>(),
                                                          src.as<NativeObject>());
}

template<XDRMode mode>
bool
js::XDRObjectLiteral(XDRState<mode>* xdr, MutableHandleObject obj)
{
    

    JSContext* cx = xdr->cx();
    MOZ_ASSERT_IF(mode == XDR_ENCODE && obj->isSingleton(),
                  JS::CompartmentOptionsRef(cx).getSingletonsAsTemplates());

    
    uint32_t isArray = 0;
    {
        if (mode == XDR_ENCODE) {
            MOZ_ASSERT(obj->is<PlainObject>() ||
                       obj->is<UnboxedPlainObject>() ||
                       obj->is<ArrayObject>());
            isArray = obj->is<ArrayObject>() ? 1 : 0;
        }

        if (!xdr->codeUint32(&isArray))
            return false;
    }

    RootedValue tmpValue(cx), tmpIdValue(cx);
    RootedId tmpId(cx);

    if (isArray) {
        uint32_t length;
        RootedArrayObject aobj(cx);

        if (mode == XDR_ENCODE) {
            aobj = &obj->as<ArrayObject>();
            length = aobj->length();
        }

        if (!xdr->codeUint32(&length))
            return false;

        if (mode == XDR_DECODE) {
            obj.set(NewDenseUnallocatedArray(cx, length, NullPtr(), TenuredObject));
            if (!obj)
                return false;
            aobj = &obj->as<ArrayObject>();
        }

        AutoValueVector values(cx);
        if (mode == XDR_ENCODE && !GetScriptArrayObjectElements(cx, aobj, values))
            return false;

        uint32_t initialized;
        {
            if (mode == XDR_ENCODE)
                initialized = values.length();
            if (!xdr->codeUint32(&initialized))
                return false;
            if (mode == XDR_DECODE) {
                if (initialized) {
                    if (!aobj->ensureElements(cx, initialized))
                        return false;
                }
            }
        }

        
        for (unsigned i = 0; i < initialized; i++) {
            if (mode == XDR_ENCODE)
                tmpValue = values[i];

            if (!xdr->codeConstValue(&tmpValue))
                return false;

            if (mode == XDR_DECODE) {
                aobj->setDenseInitializedLength(i + 1);
                aobj->initDenseElement(i, tmpValue);
            }
        }

        uint32_t copyOnWrite;
        if (mode == XDR_ENCODE)
            copyOnWrite = aobj->denseElementsAreCopyOnWrite();
        if (!xdr->codeUint32(&copyOnWrite))
            return false;

        if (mode == XDR_DECODE) {
            if (copyOnWrite) {
                if (!ObjectElements::MakeElementsCopyOnWrite(cx, aobj))
                    return false;
            } else {
                ObjectGroup::fixArrayGroup(cx, aobj);
            }
        }

        return true;
    }

    
    AutoIdValueVector properties(cx);
    if (mode == XDR_ENCODE && !GetScriptPlainObjectProperties(cx, obj, properties))
        return false;

    uint32_t nproperties = properties.length();
    if (!xdr->codeUint32(&nproperties))
        return false;

    if (mode == XDR_DECODE && !properties.appendN(IdValuePair(), nproperties))
        return false;

    for (size_t i = 0; i < nproperties; i++) {
        if (mode == XDR_ENCODE) {
            tmpIdValue = IdToValue(properties[i].get().id);
            tmpValue = properties[i].get().value;
        }

        if (!xdr->codeConstValue(&tmpIdValue) || !xdr->codeConstValue(&tmpValue))
            return false;

        if (mode == XDR_DECODE) {
            if (!ValueToId<CanGC>(cx, tmpIdValue, &tmpId))
                return false;
            properties[i].get().id = tmpId;
            properties[i].get().value = tmpValue;
        }
    }

    
    uint32_t isSingleton;
    if (mode == XDR_ENCODE)
        isSingleton = obj->isSingleton() ? 1 : 0;
    if (!xdr->codeUint32(&isSingleton))
        return false;

    if (mode == XDR_DECODE) {
        NewObjectKind newKind = isSingleton ? SingletonObject : TenuredObject;
        obj.set(ObjectGroup::newPlainObject(cx, properties.begin(), properties.length(), newKind));
        if (!obj)
            return false;
    }

    return true;
}

template bool
js::XDRObjectLiteral(XDRState<XDR_ENCODE>* xdr, MutableHandleObject obj);

template bool
js::XDRObjectLiteral(XDRState<XDR_DECODE>* xdr, MutableHandleObject obj);

JSObject*
js::CloneObjectLiteral(JSContext* cx, HandleObject srcObj)
{
    if (srcObj->is<PlainObject>()) {
        AllocKind kind = GetBackgroundAllocKind(gc::GetGCObjectKind(srcObj->as<PlainObject>().numFixedSlots()));
        MOZ_ASSERT_IF(srcObj->isTenured(), kind == srcObj->asTenured().getAllocKind());

        RootedObject proto(cx, cx->global()->getOrCreateObjectPrototype(cx));
        if (!proto)
            return nullptr;
        RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &PlainObject::class_,
                                                                 TaggedProto(proto)));
        if (!group)
            return nullptr;

        RootedPlainObject res(cx, NewObjectWithGroup<PlainObject>(cx, group, kind, TenuredObject));
        if (!res)
            return nullptr;

        
        
        RootedShape newShape(cx, ReshapeForAllocKind(cx, srcObj->as<PlainObject>().lastProperty(),
                                                     TaggedProto(proto), kind));
        if (!newShape || !res->setLastProperty(cx, newShape))
            return nullptr;

        return res;
    }

    RootedArrayObject srcArray(cx, &srcObj->as<ArrayObject>());
    MOZ_ASSERT(srcArray->denseElementsAreCopyOnWrite());
    MOZ_ASSERT(srcArray->getElementsHeader()->ownerObject() == srcObj);

    size_t length = srcArray->as<ArrayObject>().length();
    RootedArrayObject res(cx, NewDenseFullyAllocatedArray(cx, length, NullPtr(), TenuredObject));
    if (!res)
        return nullptr;

    RootedId id(cx);
    RootedValue value(cx);
    for (size_t i = 0; i < length; i++) {
        
        
        value = srcArray->getDenseElement(i);
        MOZ_ASSERT_IF(value.isMarkable(),
                      value.toGCThing()->isTenured() &&
                      cx->runtime()->isAtomsZone(value.toGCThing()->asTenured().zoneFromAnyThread()));

        id = INT_TO_JSID(i);
        if (!DefineProperty(cx, res, id, value, nullptr, nullptr, JSPROP_ENUMERATE))
            return nullptr;
    }

    if (!ObjectElements::MakeElementsCopyOnWrite(cx, res))
        return nullptr;

    return res;
}

void
NativeObject::fillInAfterSwap(JSContext* cx, const Vector<Value>& values, void* priv)
{
    
    
    
    MOZ_ASSERT(slotSpan() == values.length());

    
    size_t nfixed = gc::GetGCKindSlots(asTenured().getAllocKind(), getClass());
    if (nfixed != shape_->numFixedSlots()) {
        if (!generateOwnShape(cx))
            CrashAtUnhandlableOOM("fillInAfterSwap");
        shape_->setNumFixedSlots(nfixed);
    }

    if (hasPrivate())
        setPrivate(priv);
    else
        MOZ_ASSERT(!priv);

    if (slots_) {
        js_free(slots_);
        slots_ = nullptr;
    }

    if (size_t ndynamic = dynamicSlotsCount(nfixed, values.length(), getClass())) {
        slots_ = cx->zone()->pod_malloc<HeapSlot>(ndynamic);
        if (!slots_)
            CrashAtUnhandlableOOM("fillInAfterSwap");
        Debug_SetSlotRangeToCrashOnTouch(slots_, ndynamic);
    }

    initSlotRange(0, values.begin(), values.length());
}

void
JSObject::fixDictionaryShapeAfterSwap()
{
    
    
    if (isNative() && as<NativeObject>().inDictionaryMode())
        as<NativeObject>().shape_->listp = &as<NativeObject>().shape_;
}


bool
JSObject::swap(JSContext* cx, HandleObject a, HandleObject b)
{
    
    MOZ_ASSERT(IsBackgroundFinalized(a->asTenured().getAllocKind()) ==
               IsBackgroundFinalized(b->asTenured().getAllocKind()));
    MOZ_ASSERT(a->compartment() == b->compartment());

    AutoCompartment ac(cx, a);

    if (!a->getGroup(cx))
        CrashAtUnhandlableOOM("JSObject::swap");
    if (!b->getGroup(cx))
        CrashAtUnhandlableOOM("JSObject::swap");

    



    MOZ_ASSERT(!IsInsideNursery(a) && !IsInsideNursery(b));
    cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(a);
    cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(b);

    unsigned r = NotifyGCPreSwap(a, b);

    
    MOZ_ASSERT(a->compartment() == b->compartment());
    MOZ_ASSERT(a->is<JSFunction>() == b->is<JSFunction>());

    
    MOZ_ASSERT_IF(a->is<JSFunction>(), a->tenuredSizeOfThis() == b->tenuredSizeOfThis());

    
    
    MOZ_ASSERT(!a->is<RegExpObject>() && !b->is<RegExpObject>());
    MOZ_ASSERT(!a->is<ArrayObject>() && !b->is<ArrayObject>());
    MOZ_ASSERT(!a->is<ArrayBufferObject>() && !b->is<ArrayBufferObject>());
    MOZ_ASSERT(!a->is<TypedArrayObject>() && !b->is<TypedArrayObject>());
    MOZ_ASSERT(!a->is<TypedObject>() && !b->is<TypedObject>());

    if (a->tenuredSizeOfThis() == b->tenuredSizeOfThis()) {
        
        
        size_t size = a->tenuredSizeOfThis();

        char tmp[mozilla::tl::Max<sizeof(JSFunction), sizeof(JSObject_Slots16)>::value];
        MOZ_ASSERT(size <= sizeof(tmp));

        js_memcpy(tmp, a, size);
        js_memcpy(a, b, size);
        js_memcpy(b, tmp, size);

        a->fixDictionaryShapeAfterSwap();
        b->fixDictionaryShapeAfterSwap();
    } else {
        
        
        AutoSuppressGC suppress(cx);

        
        
        
        NativeObject* na = a->isNative() ? &a->as<NativeObject>() : nullptr;
        NativeObject* nb = b->isNative() ? &b->as<NativeObject>() : nullptr;

        
        Vector<Value> avals(cx);
        void* apriv = nullptr;
        if (na) {
            apriv = na->hasPrivate() ? na->getPrivate() : nullptr;
            for (size_t i = 0; i < na->slotSpan(); i++) {
                if (!avals.append(na->getSlot(i)))
                    CrashAtUnhandlableOOM("JSObject::swap");
            }
        }
        Vector<Value> bvals(cx);
        void* bpriv = nullptr;
        if (nb) {
            bpriv = nb->hasPrivate() ? nb->getPrivate() : nullptr;
            for (size_t i = 0; i < nb->slotSpan(); i++) {
                if (!bvals.append(nb->getSlot(i)))
                    CrashAtUnhandlableOOM("JSObject::swap");
            }
        }

        
        char tmp[sizeof(JSObject_Slots0)];
        js_memcpy(&tmp, a, sizeof tmp);
        js_memcpy(a, b, sizeof tmp);
        js_memcpy(b, &tmp, sizeof tmp);

        a->fixDictionaryShapeAfterSwap();
        b->fixDictionaryShapeAfterSwap();

        if (na)
            b->as<NativeObject>().fillInAfterSwap(cx, avals, apriv);
        if (nb)
            a->as<NativeObject>().fillInAfterSwap(cx, bvals, bpriv);
    }

    
    
    MarkObjectGroupUnknownProperties(cx, a->group());
    MarkObjectGroupUnknownProperties(cx, b->group());

    







    JS::Zone* zone = a->zone();
    if (zone->needsIncrementalBarrier()) {
        a->markChildren(zone->barrierTracer());
        b->markChildren(zone->barrierTracer());
    }

    NotifyGCPostSwap(a, b, r);
    return true;
}

static bool
DefineStandardSlot(JSContext* cx, HandleObject obj, JSProtoKey key, JSAtom* atom,
                   HandleValue v, uint32_t attrs, bool& named)
{
    RootedId id(cx, AtomToId(atom));

    if (key != JSProto_Null) {
        




        Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

        if (!global->lookup(cx, id)) {
            global->setConstructorPropertySlot(key, v);

            uint32_t slot = GlobalObject::constructorPropertySlot(key);
            if (!NativeObject::addProperty(cx, global, id, nullptr, nullptr, slot, attrs, 0))
                return false;

            named = true;
            return true;
        }
    }

    named = DefineProperty(cx, obj, id, v, nullptr, nullptr, attrs);
    return named;
}

static void
SetClassObject(JSObject* obj, JSProtoKey key, JSObject* cobj, JSObject* proto)
{
    if (!obj->is<GlobalObject>())
        return;

    obj->as<GlobalObject>().setConstructor(key, ObjectOrNullValue(cobj));
    obj->as<GlobalObject>().setPrototype(key, ObjectOrNullValue(proto));
}

static void
ClearClassObject(JSObject* obj, JSProtoKey key)
{
    if (!obj->is<GlobalObject>())
        return;

    obj->as<GlobalObject>().setConstructor(key, UndefinedValue());
    obj->as<GlobalObject>().setPrototype(key, UndefinedValue());
}

static NativeObject*
DefineConstructorAndPrototype(JSContext* cx, HandleObject obj, JSProtoKey key, HandleAtom atom,
                              HandleObject protoProto, const Class* clasp,
                              Native constructor, unsigned nargs,
                              const JSPropertySpec* ps, const JSFunctionSpec* fs,
                              const JSPropertySpec* static_ps, const JSFunctionSpec* static_fs,
                              NativeObject** ctorp, AllocKind ctorKind)
{
    





















    



    RootedNativeObject proto(cx, NewNativeObjectWithClassProto(cx, clasp, protoProto, SingletonObject));
    if (!proto)
        return nullptr;

    
    RootedNativeObject ctor(cx);
    bool named = false;
    bool cached = false;
    if (!constructor) {
        





        if (!(clasp->flags & JSCLASS_IS_ANONYMOUS) || !obj->is<GlobalObject>() ||
            key == JSProto_Null)
        {
            uint32_t attrs = (clasp->flags & JSCLASS_IS_ANONYMOUS)
                           ? JSPROP_READONLY | JSPROP_PERMANENT
                           : 0;
            RootedValue value(cx, ObjectValue(*proto));
            if (!DefineStandardSlot(cx, obj, key, atom, value, attrs, named))
                goto bad;
        }

        ctor = proto;
    } else {
        RootedFunction fun(cx, NewNativeConstructor(cx, constructor, nargs, atom, ctorKind));
        if (!fun)
            goto bad;

        




        if (key != JSProto_Null) {
            SetClassObject(obj, key, fun, proto);
            cached = true;
        }

        RootedValue value(cx, ObjectValue(*fun));
        if (!DefineStandardSlot(cx, obj, key, atom, value, 0, named))
            goto bad;

        




        ctor = fun;
        if (!LinkConstructorAndPrototype(cx, ctor, proto))
            goto bad;

        
        Rooted<TaggedProto> tagged(cx, TaggedProto(proto));
        if (ctor->getClass() == clasp && !ctor->splicePrototype(cx, clasp, tagged))
            goto bad;
    }

    if (!DefinePropertiesAndFunctions(cx, proto, ps, fs) ||
        (ctor != proto && !DefinePropertiesAndFunctions(cx, ctor, static_ps, static_fs)))
    {
        goto bad;
    }

    
    if (!cached && key != JSProto_Null)
        SetClassObject(obj, key, ctor, proto);

    if (ctorp)
        *ctorp = ctor;
    return proto;

bad:
    if (named) {
        ObjectOpResult ignored;
        RootedId id(cx, AtomToId(atom));

        
        DeleteProperty(cx, obj, id, ignored);
    }
    if (cached)
        ClearClassObject(obj, key);
    return nullptr;
}

NativeObject*
js::InitClass(JSContext* cx, HandleObject obj, HandleObject protoProto_,
              const Class* clasp, Native constructor, unsigned nargs,
              const JSPropertySpec* ps, const JSFunctionSpec* fs,
              const JSPropertySpec* static_ps, const JSFunctionSpec* static_fs,
              NativeObject** ctorp, AllocKind ctorKind)
{
    RootedObject protoProto(cx, protoProto_);

    
    MOZ_ASSERT(clasp->getProperty != JS_PropertyStub);
    MOZ_ASSERT(clasp->setProperty != JS_StrictPropertyStub);

    RootedAtom atom(cx, Atomize(cx, clasp->name, strlen(clasp->name)));
    if (!atom)
        return nullptr;

    








    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null &&
        !protoProto &&
        !GetBuiltinPrototype(cx, JSProto_Object, &protoProto))
    {
        return nullptr;
    }

    return DefineConstructorAndPrototype(cx, obj, key, atom, protoProto, clasp, constructor, nargs,
                                         ps, fs, static_ps, static_fs, ctorp, ctorKind);
}

void
JSObject::fixupAfterMovingGC()
{
    
    
    if (is<NativeObject>()) {
        NativeObject& obj = as<NativeObject>();
        if (obj.denseElementsAreCopyOnWrite()) {
            NativeObject* owner = MaybeForwarded(obj.getElementsHeader()->ownerObject().get());
            if (owner != &obj && owner->hasFixedElements())
                obj.elements_ = owner->getElementsHeader()->elements();
            MOZ_ASSERT(!IsForwarded(obj.getElementsHeader()->ownerObject().get()));
        }
    }
}

bool
js::SetClassAndProto(JSContext* cx, HandleObject obj,
                     const Class* clasp, Handle<js::TaggedProto> proto)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    RootedObject oldproto(cx, obj);
    while (oldproto && oldproto->isNative()) {
        if (oldproto->isSingleton()) {
            if (!oldproto->as<NativeObject>().generateOwnShape(cx))
                return false;
        } else {
            if (!oldproto->setUncacheableProto(cx))
                return false;
        }
        if (!obj->isDelegate()) {
            
            
            MOZ_ASSERT(obj == oldproto);
            break;
        }
        oldproto = oldproto->getProto();
    }

    if (proto.isObject() && !proto.toObject()->setDelegate(cx))
        return false;

    if (obj->isSingleton()) {
        



        if (!obj->splicePrototype(cx, clasp, proto))
            return false;
        MarkObjectGroupUnknownProperties(cx, obj->group());
        return true;
    }

    if (proto.isObject()) {
        RootedObject protoObj(cx, proto.toObject());
        if (!JSObject::setNewGroupUnknown(cx, clasp, protoObj))
            return false;
    }

    ObjectGroup* group = ObjectGroup::defaultNewGroup(cx, clasp, proto);
    if (!group)
        return false;

    






    MarkObjectGroupUnknownProperties(cx, obj->group());
    MarkObjectGroupUnknownProperties(cx, group);

    obj->setGroup(group);

    return true;
}

static bool
MaybeResolveConstructor(ExclusiveContext* cxArg, Handle<GlobalObject*> global, JSProtoKey key)
{
    if (global->isStandardClassResolved(key))
        return true;
    if (!cxArg->shouldBeJSContext())
        return false;

    JSContext* cx = cxArg->asJSContext();
    return GlobalObject::resolveConstructor(cx, global, key);
}

bool
js::GetBuiltinConstructor(ExclusiveContext* cx, JSProtoKey key, MutableHandleObject objp)
{
    MOZ_ASSERT(key != JSProto_Null);
    Rooted<GlobalObject*> global(cx, cx->global());
    if (!MaybeResolveConstructor(cx, global, key))
        return false;

    objp.set(&global->getConstructor(key).toObject());
    return true;
}

bool
js::GetBuiltinPrototype(ExclusiveContext* cx, JSProtoKey key, MutableHandleObject protop)
{
    MOZ_ASSERT(key != JSProto_Null);
    Rooted<GlobalObject*> global(cx, cx->global());
    if (!MaybeResolveConstructor(cx, global, key))
        return false;

    protop.set(&global->getPrototype(key).toObject());
    return true;
}

static bool
IsStandardPrototype(JSObject* obj, JSProtoKey key)
{
    GlobalObject& global = obj->global();
    Value v = global.getPrototype(key);
    return v.isObject() && obj == &v.toObject();
}

JSProtoKey
JS::IdentifyStandardInstance(JSObject* obj)
{
    
    MOZ_ASSERT(!obj->is<CrossCompartmentWrapperObject>());
    JSProtoKey key = StandardProtoKeyOrNull(obj);
    if (key != JSProto_Null && !IsStandardPrototype(obj, key))
        return key;
    return JSProto_Null;
}

JSProtoKey
JS::IdentifyStandardPrototype(JSObject* obj)
{
    
    MOZ_ASSERT(!obj->is<CrossCompartmentWrapperObject>());
    JSProtoKey key = StandardProtoKeyOrNull(obj);
    if (key != JSProto_Null && IsStandardPrototype(obj, key))
        return key;
    return JSProto_Null;
}

JSProtoKey
JS::IdentifyStandardInstanceOrPrototype(JSObject* obj)
{
    return StandardProtoKeyOrNull(obj);
}

JSProtoKey
JS::IdentifyStandardConstructor(JSObject* obj)
{
    
    
    
    
    if (!obj->is<JSFunction>() || !(obj->as<JSFunction>().flags() & JSFunction::NATIVE_CTOR))
        return JSProto_Null;

    GlobalObject& global = obj->global();
    for (size_t k = 0; k < JSProto_LIMIT; ++k) {
        JSProtoKey key = static_cast<JSProtoKey>(k);
        if (global.getConstructor(key) == ObjectValue(*obj))
            return key;
    }

    return JSProto_Null;
}

bool
JSObject::isCallable() const
{
    if (is<JSFunction>())
        return true;
    return callHook() != nullptr;
}

bool
JSObject::isConstructor() const
{
    if (is<JSFunction>()) {
        const JSFunction& fun = as<JSFunction>();
        return fun.isNativeConstructor() || fun.isInterpretedConstructor();
    }
    return constructHook() != nullptr;
}

JSNative
JSObject::callHook() const
{
    const js::Class* clasp = getClass();

    if (clasp->call)
        return clasp->call;

    if (is<js::ProxyObject>()) {
        const js::ProxyObject& p = as<js::ProxyObject>();
        if (p.handler()->isCallable(const_cast<JSObject*>(this)))
            return js::proxy_Call;
    }
    return nullptr;
}

JSNative
JSObject::constructHook() const
{
    const js::Class* clasp = getClass();

    if (clasp->construct)
        return clasp->construct;

    if (is<js::ProxyObject>()) {
        const js::ProxyObject& p = as<js::ProxyObject>();
        if (p.handler()->isConstructor(const_cast<JSObject*>(this)))
            return js::proxy_Construct;
    }
    return nullptr;
}

bool
js::LookupProperty(JSContext* cx, HandleObject obj, js::HandleId id,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    



    if (LookupPropertyOp op = obj->getOps()->lookupProperty)
        return op(cx, obj, id, objp, propp);
    return LookupPropertyInline<CanGC>(cx, obj.as<NativeObject>(), id, objp, propp);
}

bool
js::LookupName(JSContext* cx, HandlePropertyName name, HandleObject scopeChain,
               MutableHandleObject objp, MutableHandleObject pobjp, MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));

    for (RootedObject scope(cx, scopeChain); scope; scope = scope->enclosingScope()) {
        if (!LookupProperty(cx, scope, id, pobjp, propp))
            return false;
        if (propp) {
            objp.set(scope);
            return true;
        }
    }

    objp.set(nullptr);
    pobjp.set(nullptr);
    propp.set(nullptr);
    return true;
}

bool
js::LookupNameNoGC(JSContext* cx, PropertyName* name, JSObject* scopeChain,
                   JSObject** objp, JSObject** pobjp, Shape** propp)
{
    AutoAssertNoException nogc(cx);

    MOZ_ASSERT(!*objp && !*pobjp && !*propp);

    for (JSObject* scope = scopeChain; scope; scope = scope->enclosingScope()) {
        if (scope->getOps()->lookupProperty)
            return false;
        if (!LookupPropertyInline<NoGC>(cx, &scope->as<NativeObject>(), NameToId(name), pobjp, propp))
            return false;
        if (*propp) {
            *objp = scope;
            return true;
        }
    }

    return true;
}

bool
js::LookupNameWithGlobalDefault(JSContext* cx, HandlePropertyName name, HandleObject scopeChain,
                                MutableHandleObject objp)
{
    RootedId id(cx, NameToId(name));

    RootedObject pobj(cx);
    RootedShape shape(cx);

    RootedObject scope(cx, scopeChain);
    for (; !scope->is<GlobalObject>(); scope = scope->enclosingScope()) {
        if (!LookupProperty(cx, scope, id, &pobj, &shape))
            return false;
        if (shape)
            break;
    }

    objp.set(scope);
    return true;
}

bool
js::LookupNameUnqualified(JSContext* cx, HandlePropertyName name, HandleObject scopeChain,
                          MutableHandleObject objp)
{
    RootedId id(cx, NameToId(name));

    RootedObject pobj(cx);
    RootedShape shape(cx);

    RootedObject scope(cx, scopeChain);
    for (; !scope->isUnqualifiedVarObj(); scope = scope->enclosingScope()) {
        if (!LookupProperty(cx, scope, id, &pobj, &shape))
            return false;
        if (shape)
            break;
    }

    
    if (pobj == scope && IsUninitializedLexicalSlot(scope, shape)) {
        scope = UninitializedLexicalObject::create(cx, scope);
        if (!scope)
            return false;
    }

    objp.set(scope);
    return true;
}

bool
js::HasOwnProperty(JSContext* cx, HandleObject obj, HandleId id, bool* result)
{
    if (obj->is<ProxyObject>())
        return Proxy::hasOwn(cx, obj, id, result);

    if (GetOwnPropertyOp op = obj->getOps()->getOwnPropertyDescriptor) {
        Rooted<PropertyDescriptor> desc(cx);
        if (!op(cx, obj, id, &desc))
            return false;
        *result = !!desc.object();
        return true;
    }

    RootedShape shape(cx);
    if (!NativeLookupOwnProperty<CanGC>(cx, obj.as<NativeObject>(), id, &shape))
        return false;
    *result = (shape != nullptr);
    return true;
}

bool
js::LookupPropertyPure(ExclusiveContext* cx, JSObject* obj, jsid id, JSObject** objp,
                       Shape** propp)
{
    do {
        if (obj->isNative()) {
            

            if (JSID_IS_INT(id) && obj->as<NativeObject>().containsDenseElement(JSID_TO_INT(id))) {
                *objp = obj;
                MarkDenseOrTypedArrayElementFound<NoGC>(propp);
                return true;
            }

            if (IsAnyTypedArray(obj)) {
                uint64_t index;
                if (IsTypedArrayIndex(id, &index)) {
                    if (index < AnyTypedArrayLength(obj)) {
                        *objp = obj;
                        MarkDenseOrTypedArrayElementFound<NoGC>(propp);
                    } else {
                        *objp = nullptr;
                        *propp = nullptr;
                    }
                    return true;
                }
            }

            if (Shape* shape = obj->as<NativeObject>().lookupPure(id)) {
                *objp = obj;
                *propp = shape;
                return true;
            }

            
            
            
            do {
                const Class* clasp = obj->getClass();
                if (!clasp->resolve)
                    break;
                if (clasp->resolve == fun_resolve && !FunctionHasResolveHook(cx->names(), id))
                    break;
                if (clasp->resolve == str_resolve && !JSID_IS_INT(id))
                    break;
                return false;
            } while (0);
        } else if (obj->is<UnboxedPlainObject>()) {
            if (obj->as<UnboxedPlainObject>().containsUnboxedOrExpandoProperty(cx, id)) {
                *objp = obj;
                MarkNonNativePropertyFound<NoGC>(propp);
                return true;
            }
        } else if (obj->is<TypedObject>()) {
            if (obj->as<TypedObject>().typeDescr().hasProperty(cx->names(), id)) {
                *objp = obj;
                MarkNonNativePropertyFound<NoGC>(propp);
                return true;
            }
        } else {
            return false;
        }

        obj = obj->getProto();
    } while (obj);

    *objp = nullptr;
    *propp = nullptr;
    return true;
}

static inline bool
NativeGetPureInline(NativeObject* pobj, Shape* shape, Value* vp)
{
    if (shape->hasSlot()) {
        *vp = pobj->getSlot(shape->slot());
        MOZ_ASSERT(!vp->isMagic());
    } else {
        vp->setUndefined();
    }

    
    return shape->hasDefaultGetter();
}

bool
js::GetPropertyPure(ExclusiveContext* cx, JSObject* obj, jsid id, Value* vp)
{
    JSObject* pobj;
    Shape* shape;
    if (!LookupPropertyPure(cx, obj, id, &pobj, &shape))
        return false;
    return pobj->isNative() && NativeGetPureInline(&pobj->as<NativeObject>(), shape, vp);
}

bool
JSObject::reportReadOnly(JSContext* cx, jsid id, unsigned report)
{
    RootedValue val(cx, IdToValue(id));
    return ReportValueErrorFlags(cx, report, JSMSG_READ_ONLY,
                                 JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                 nullptr, nullptr);
}

bool
JSObject::reportNotConfigurable(JSContext* cx, jsid id, unsigned report)
{
    RootedValue val(cx, IdToValue(id));
    return ReportValueErrorFlags(cx, report, JSMSG_CANT_DELETE,
                                 JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                 nullptr, nullptr);
}

bool
JSObject::reportNotExtensible(JSContext* cx, unsigned report)
{
    RootedValue val(cx, ObjectValue(*this));
    return ReportValueErrorFlags(cx, report, JSMSG_OBJECT_NOT_EXTENSIBLE,
                                 JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                 nullptr, nullptr);
}




bool
js::SetPrototype(JSContext* cx, HandleObject obj, HandleObject proto, JS::ObjectOpResult& result)
{
    





    if (obj->hasLazyPrototype()) {
        MOZ_ASSERT(obj->is<ProxyObject>());
        return Proxy::setPrototype(cx, obj, proto, result);
    }

    
    if (obj->nonLazyPrototypeIsImmutable()) {
        return result.fail(JSMSG_CANT_SET_PROTO);
    }

    




    if (obj->is<ArrayBufferObject>()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_SET_PROTO_OF,
                             "incompatible ArrayBuffer");
        return false;
    }

    


    if (obj->is<TypedObject>()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_SET_PROTO_OF,
                             "incompatible TypedObject");
        return false;
    }

    



    if (!strcmp(obj->getClass()->name, "Location")) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_SET_PROTO_OF,
                             "incompatible Location object");
        return false;
    }

    
    bool extensible;
    if (!IsExtensible(cx, obj, &extensible))
        return false;
    if (!extensible)
        return result.fail(JSMSG_CANT_SET_PROTO);

    
    RootedObject obj2(cx);
    for (obj2 = proto; obj2; ) {
        if (obj2 == obj)
            return result.fail(JSMSG_CANT_SET_PROTO_CYCLE);

        if (!GetPrototype(cx, obj2, &obj2))
            return false;
    }

    
    
    if (obj->is<UnboxedPlainObject>() && !UnboxedPlainObject::convertToNative(cx, obj))
        return false;

    Rooted<TaggedProto> taggedProto(cx, TaggedProto(proto));
    if (!SetClassAndProto(cx, obj, obj->getClass(), taggedProto))
        return false;

    return result.succeed();
}

bool
js::SetPrototype(JSContext* cx, HandleObject obj, HandleObject proto)
{
    ObjectOpResult result;
    return SetPrototype(cx, obj, proto, result) && result.checkStrict(cx, obj);
}

bool
js::PreventExtensions(JSContext* cx, HandleObject obj, ObjectOpResult& result)
{
    if (obj->is<ProxyObject>())
        return js::Proxy::preventExtensions(cx, obj, result);

    if (!obj->nonProxyIsExtensible())
        return result.succeed();

    
    AutoIdVector props(cx);
    if (!js::GetPropertyKeys(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    
    
    
    
    if (obj->isNative()) {
        if (!NativeObject::sparsifyDenseElements(cx, obj.as<NativeObject>()))
            return false;
    }

    if (!obj->setFlags(cx, BaseShape::NOT_EXTENSIBLE, JSObject::GENERATE_SHAPE))
        return false;
    return result.succeed();
}

bool
js::PreventExtensions(JSContext* cx, HandleObject obj)
{
    ObjectOpResult result;
    return PreventExtensions(cx, obj, result) && result.checkStrict(cx, obj);
}

bool
js::GetOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                             MutableHandle<PropertyDescriptor> desc)
{
    if (GetOwnPropertyOp op = obj->getOps()->getOwnPropertyDescriptor) {
        bool ok = op(cx, obj, id, desc);
        if (ok)
            desc.assertCompleteIfFound();
        return ok;
    }

    RootedShape shape(cx);
    if (!NativeLookupOwnProperty<CanGC>(cx, obj.as<NativeObject>(), id, &shape))
        return false;
    if (!shape) {
        desc.object().set(nullptr);
        return true;
    }

    bool doGet = true;
    desc.setAttributes(GetShapeAttributes(obj, shape));
    if (desc.isAccessorDescriptor()) {
        MOZ_ASSERT(desc.isShared());
        doGet = false;

        
        
        
        
        
        
        
        
        if (desc.hasGetterObject()) {
            desc.setGetterObject(shape->getterObject());
        } else {
            desc.setGetterObject(nullptr);
            desc.attributesRef() |= JSPROP_GETTER;
        }
        if (desc.hasSetterObject()) {
            desc.setSetterObject(shape->setterObject());
        } else {
            desc.setSetterObject(nullptr);
            desc.attributesRef() |= JSPROP_SETTER;
        }
    } else {
        
        
        
        
        desc.attributesRef() &= ~JSPROP_SHARED;
    }

    RootedValue value(cx);
    if (doGet && !GetProperty(cx, obj, obj, id, &value))
        return false;

    desc.value().set(value);
    desc.object().set(obj);
    desc.assertComplete();
    return true;
}

bool
js::DefineProperty(JSContext* cx, HandleObject obj, HandleId id, Handle<PropertyDescriptor> desc,
                   ObjectOpResult& result)
{
    desc.assertValid();
    if (DefinePropertyOp op = obj->getOps()->defineProperty)
        return op(cx, obj, id, desc, result);
    return NativeDefineProperty(cx, obj.as<NativeObject>(), id, desc, result);
}

bool
js::DefineProperty(ExclusiveContext* cx, HandleObject obj, HandleId id, HandleValue value,
                   JSGetterOp getter, JSSetterOp setter, unsigned attrs,
                   ObjectOpResult& result)
{
    MOZ_ASSERT(!(attrs & JSPROP_PROPOP_ACCESSORS));

    Rooted<PropertyDescriptor> desc(cx);
    desc.initFields(obj, value, attrs, getter, setter);
    if (DefinePropertyOp op = obj->getOps()->defineProperty) {
        if (!cx->shouldBeJSContext())
            return false;
        return op(cx->asJSContext(), obj, id, desc, result);
    }
    return NativeDefineProperty(cx, obj.as<NativeObject>(), id, desc, result);
}

bool
js::DefineProperty(ExclusiveContext* cx, HandleObject obj, PropertyName* name, HandleValue value,
                   JSGetterOp getter, JSSetterOp setter, unsigned attrs,
                   ObjectOpResult& result)
{
    RootedId id(cx, NameToId(name));
    return DefineProperty(cx, obj, id, value, getter, setter, attrs, result);
}

bool
js::DefineElement(ExclusiveContext* cx, HandleObject obj, uint32_t index, HandleValue value,
                  JSGetterOp getter, JSSetterOp setter, unsigned attrs,
                  ObjectOpResult& result)
{
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return DefineProperty(cx, obj, id, value, getter, setter, attrs, result);
}

bool
js::DefineProperty(ExclusiveContext* cx, HandleObject obj, HandleId id, HandleValue value,
                   JSGetterOp getter, JSSetterOp setter, unsigned attrs)
{
    ObjectOpResult result;
    if (!DefineProperty(cx, obj, id, value, getter, setter, attrs, result))
        return false;
    if (!result) {
        if (!cx->shouldBeJSContext())
            return false;
        result.reportError(cx->asJSContext(), obj, id);
        return false;
    }
    return true;
}

bool
js::DefineProperty(ExclusiveContext* cx, HandleObject obj, PropertyName* name, HandleValue value,
                   JSGetterOp getter, JSSetterOp setter, unsigned attrs)
{
    RootedId id(cx, NameToId(name));
    return DefineProperty(cx, obj, id, value, getter, setter, attrs);
}

bool
js::DefineElement(ExclusiveContext* cx, HandleObject obj, uint32_t index, HandleValue value,
                  JSGetterOp getter, JSSetterOp setter, unsigned attrs)
{
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return DefineProperty(cx, obj, id, value, getter, setter, attrs);
}




bool
js::SetImmutablePrototype(ExclusiveContext* cx, HandleObject obj, bool* succeeded)
{
    if (obj->hasLazyPrototype()) {
        if (!cx->shouldBeJSContext())
            return false;
        return Proxy::setImmutablePrototype(cx->asJSContext(), obj, succeeded);
    }

    if (!obj->setFlags(cx, BaseShape::IMMUTABLE_PROTOTYPE))
        return false;
    *succeeded = true;
    return true;
}

bool
js::GetPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                          MutableHandle<PropertyDescriptor> desc)
{
    RootedObject pobj(cx);

    for (pobj = obj; pobj;) {
        if (pobj->is<ProxyObject>()) {
            bool ok = Proxy::getPropertyDescriptor(cx, pobj, id, desc);
            if (ok)
                desc.assertCompleteIfFound();
            return ok;
        }

        if (!GetOwnPropertyDescriptor(cx, pobj, id, desc))
            return false;

        if (desc.object())
            return true;

        if (!GetPrototype(cx, pobj, &pobj))
            return false;
    }

    MOZ_ASSERT(!desc.object());
    return true;
}

bool
js::ToPrimitive(JSContext* cx, HandleObject obj, JSType hint, MutableHandleValue vp)
{
    bool ok;
    if (JSConvertOp op = obj->getClass()->convert)
        ok = op(cx, obj, hint, vp);
    else
        ok = JS::OrdinaryToPrimitive(cx, obj, hint, vp);
    MOZ_ASSERT_IF(ok, vp.isPrimitive());
    return ok;
}

bool
js::WatchGuts(JSContext* cx, JS::HandleObject origObj, JS::HandleId id, JS::HandleObject callable)
{
    RootedObject obj(cx, GetInnerObject(origObj));
    if (obj->isNative()) {
        
        
        if (!NativeObject::sparsifyDenseElements(cx, obj.as<NativeObject>()))
            return false;

        MarkTypePropertyNonData(cx, obj, id);
    }

    WatchpointMap* wpmap = cx->compartment()->watchpointMap;
    if (!wpmap) {
        wpmap = cx->runtime()->new_<WatchpointMap>();
        if (!wpmap || !wpmap->init()) {
            ReportOutOfMemory(cx);
            return false;
        }
        cx->compartment()->watchpointMap = wpmap;
    }

    return wpmap->watch(cx, obj, id, js::WatchHandler, callable);
}

bool
js::UnwatchGuts(JSContext* cx, JS::HandleObject origObj, JS::HandleId id)
{
    
    
    RootedObject obj(cx, GetInnerObject(origObj));
    if (WatchpointMap* wpmap = cx->compartment()->watchpointMap)
        wpmap->unwatch(obj, id, nullptr, nullptr);
    return true;
}

bool
js::WatchProperty(JSContext* cx, HandleObject obj, HandleId id, HandleObject callable)
{
    if (WatchOp op = obj->getOps()->watch)
        return op(cx, obj, id, callable);

    if (!obj->isNative() || IsAnyTypedArray(obj)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_WATCH,
                             obj->getClass()->name);
        return false;
    }

    return WatchGuts(cx, obj, id, callable);
}

bool
js::UnwatchProperty(JSContext* cx, HandleObject obj, HandleId id)
{
    if (UnwatchOp op = obj->getOps()->unwatch)
        return op(cx, obj, id);

    return UnwatchGuts(cx, obj, id);
}

const char*
js::GetObjectClassName(JSContext* cx, HandleObject obj)
{
    assertSameCompartment(cx, obj);

    if (obj->is<ProxyObject>())
        return Proxy::className(cx, obj);

    return obj->getClass()->name;
}

bool
JSObject::callMethod(JSContext* cx, HandleId id, unsigned argc, Value* argv, MutableHandleValue vp)
{
    RootedValue fval(cx);
    RootedObject obj(cx, this);
    if (!GetProperty(cx, obj, obj, id, &fval))
        return false;
    return Invoke(cx, ObjectValue(*obj), fval, argc, argv, vp);
}




bool
js::HasDataProperty(JSContext* cx, NativeObject* obj, jsid id, Value* vp)
{
    if (JSID_IS_INT(id) && obj->containsDenseElement(JSID_TO_INT(id))) {
        *vp = obj->getDenseElement(JSID_TO_INT(id));
        return true;
    }

    if (Shape* shape = obj->lookup(cx, id)) {
        if (shape->hasDefaultGetter() && shape->hasSlot()) {
            *vp = obj->getSlot(shape->slot());
            return true;
        }
    }

    return false;
}









static bool
MaybeCallMethod(JSContext* cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    if (!GetProperty(cx, obj, obj, id, vp))
        return false;
    if (!IsCallable(vp)) {
        vp.setObject(*obj);
        return true;
    }
    return Invoke(cx, ObjectValue(*obj), vp, 0, nullptr, vp);
}

bool
JS::OrdinaryToPrimitive(JSContext* cx, HandleObject obj, JSType hint, MutableHandleValue vp)
{
    MOZ_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);

    Rooted<jsid> id(cx);

    const Class* clasp = obj->getClass();
    if (hint == JSTYPE_STRING) {
        id = NameToId(cx->names().toString);

        
        if (clasp == &StringObject::class_) {
            StringObject* nobj = &obj->as<StringObject>();
            if (ClassMethodIsNative(cx, nobj, &StringObject::class_, id, str_toString)) {
                vp.setString(nobj->unbox());
                return true;
            }
        }

        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp.isPrimitive())
            return true;

        id = NameToId(cx->names().valueOf);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp.isPrimitive())
            return true;
    } else {

        
        if (clasp == &StringObject::class_) {
            id = NameToId(cx->names().valueOf);
            StringObject* nobj = &obj->as<StringObject>();
            if (ClassMethodIsNative(cx, nobj, &StringObject::class_, id, str_toString)) {
                vp.setString(nobj->unbox());
                return true;
            }
        }

        
        if (clasp == &NumberObject::class_) {
            id = NameToId(cx->names().valueOf);
            NumberObject* nobj = &obj->as<NumberObject>();
            if (ClassMethodIsNative(cx, nobj, &NumberObject::class_, id, num_valueOf)) {
                vp.setNumber(nobj->unbox());
                return true;
            }
        }

        id = NameToId(cx->names().valueOf);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp.isPrimitive())
            return true;

        id = NameToId(cx->names().toString);
        if (!MaybeCallMethod(cx, obj, id, vp))
            return false;
        if (vp.isPrimitive())
            return true;
    }

    
    RootedString str(cx);
    if (hint == JSTYPE_STRING) {
        str = JS_InternString(cx, clasp->name);
        if (!str)
            return false;
    } else {
        str = nullptr;
    }

    RootedValue val(cx, ObjectValue(*obj));
    ReportValueError2(cx, JSMSG_CANT_CONVERT_TO, JSDVG_SEARCH_STACK, val, str,
                      hint == JSTYPE_VOID
                      ? "primitive type"
                      : hint == JSTYPE_STRING ? "string" : "number");
    return false;
}

bool
js::IsDelegate(JSContext* cx, HandleObject obj, const js::Value& v, bool* result)
{
    if (v.isPrimitive()) {
        *result = false;
        return true;
    }
    return IsDelegateOfObject(cx, obj, &v.toObject(), result);
}

bool
js::IsDelegateOfObject(JSContext* cx, HandleObject protoObj, JSObject* obj, bool* result)
{
    RootedObject obj2(cx, obj);
    for (;;) {
        if (!GetPrototype(cx, obj2, &obj2))
            return false;
        if (!obj2) {
            *result = false;
            return true;
        }
        if (obj2 == protoObj) {
            *result = true;
            return true;
        }
    }
}

JSObject*
js::GetBuiltinPrototypePure(GlobalObject* global, JSProtoKey protoKey)
{
    MOZ_ASSERT(JSProto_Null <= protoKey);
    MOZ_ASSERT(protoKey < JSProto_LIMIT);

    if (protoKey != JSProto_Null) {
        const Value& v = global->getPrototype(protoKey);
        if (v.isObject())
            return &v.toObject();
    }

    return nullptr;
}

JSObject*
js::PrimitiveToObject(JSContext* cx, const Value& v)
{
    if (v.isString()) {
        Rooted<JSString*> str(cx, v.toString());
        return StringObject::create(cx, str);
    }
    if (v.isNumber())
        return NumberObject::create(cx, v.toNumber());
    if (v.isBoolean())
        return BooleanObject::create(cx, v.toBoolean());
    MOZ_ASSERT(v.isSymbol());
    RootedSymbol symbol(cx, v.toSymbol());
    return SymbolObject::create(cx, symbol);
}








JSObject*
js::ToObjectSlow(JSContext* cx, JS::HandleValue val, bool reportScanStack)
{
    MOZ_ASSERT(!val.isMagic());
    MOZ_ASSERT(!val.isObject());

    if (val.isNullOrUndefined()) {
        if (reportScanStack) {
            ReportIsNullOrUndefined(cx, JSDVG_SEARCH_STACK, val, NullPtr());
        } else {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_CONVERT_TO,
                                 val.isNull() ? "null" : "undefined", "object");
        }
        return nullptr;
    }

    return PrimitiveToObject(cx, val);
}

void
js::GetObjectSlotName(JSTracer* trc, char* buf, size_t bufsize)
{
    MOZ_ASSERT(trc->debugPrinter() == GetObjectSlotName);

    JSObject* obj = (JSObject*)trc->debugPrintArg();
    uint32_t slot = uint32_t(trc->debugPrintIndex());

    Shape* shape;
    if (obj->isNative()) {
        shape = obj->as<NativeObject>().lastProperty();
        while (shape && (!shape->hasSlot() || shape->slot() != slot))
            shape = shape->previous();
    } else {
        shape = nullptr;
    }

    if (!shape) {
        do {
            const char* slotname = nullptr;
            const char* pattern = nullptr;
            if (obj->is<GlobalObject>()) {
                pattern = "CLASS_OBJECT(%s)";
                if (false)
                    ;
#define TEST_SLOT_MATCHES_PROTOTYPE(name,code,init,clasp) \
                else if ((code) == slot) { slotname = js_##name##_str; }
                JS_FOR_EACH_PROTOTYPE(TEST_SLOT_MATCHES_PROTOTYPE)
#undef TEST_SLOT_MATCHES_PROTOTYPE
            } else {
                pattern = "%s";
                if (obj->is<ScopeObject>()) {
                    if (slot == ScopeObject::enclosingScopeSlot()) {
                        slotname = "enclosing_environment";
                    } else if (obj->is<CallObject>()) {
                        if (slot == CallObject::calleeSlot())
                            slotname = "callee_slot";
                    } else if (obj->is<DeclEnvObject>()) {
                        if (slot == DeclEnvObject::lambdaSlot())
                            slotname = "named_lambda";
                    } else if (obj->is<DynamicWithObject>()) {
                        if (slot == DynamicWithObject::objectSlot())
                            slotname = "with_object";
                        else if (slot == DynamicWithObject::thisSlot())
                            slotname = "with_this";
                    }
                }
            }

            if (slotname)
                JS_snprintf(buf, bufsize, pattern, slotname);
            else
                JS_snprintf(buf, bufsize, "**UNKNOWN SLOT %ld**", (long)slot);
        } while (false);
    } else {
        jsid propid = shape->propid();
        if (JSID_IS_INT(propid)) {
            JS_snprintf(buf, bufsize, "%ld", (long)JSID_TO_INT(propid));
        } else if (JSID_IS_ATOM(propid)) {
            PutEscapedString(buf, bufsize, JSID_TO_ATOM(propid), 0);
        } else if (JSID_IS_SYMBOL(propid)) {
            JS_snprintf(buf, bufsize, "**SYMBOL KEY**");
        } else {
            JS_snprintf(buf, bufsize, "**FINALIZED ATOM KEY**");
        }
    }
}

bool
js::ReportGetterOnlyAssignment(JSContext* cx, bool strict)
{
    return JS_ReportErrorFlagsAndNumber(cx,
                                        strict
                                        ? JSREPORT_ERROR
                                        : JSREPORT_WARNING | JSREPORT_STRICT,
                                        GetErrorMessage, nullptr,
                                        JSMSG_GETTER_ONLY);
}




#ifdef DEBUG







static void
dumpValue(const Value& v)
{
    if (v.isNull())
        fprintf(stderr, "null");
    else if (v.isUndefined())
        fprintf(stderr, "undefined");
    else if (v.isInt32())
        fprintf(stderr, "%d", v.toInt32());
    else if (v.isDouble())
        fprintf(stderr, "%g", v.toDouble());
    else if (v.isString())
        v.toString()->dump();
    else if (v.isSymbol())
        v.toSymbol()->dump();
    else if (v.isObject() && v.toObject().is<JSFunction>()) {
        JSFunction* fun = &v.toObject().as<JSFunction>();
        if (fun->displayAtom()) {
            fputs("<function ", stderr);
            FileEscapedString(stderr, fun->displayAtom(), 0);
        } else {
            fputs("<unnamed function", stderr);
        }
        if (fun->hasScript()) {
            JSScript* script = fun->nonLazyScript();
            fprintf(stderr, " (%s:%" PRIuSIZE ")",
                    script->filename() ? script->filename() : "", script->lineno());
        }
        fprintf(stderr, " at %p>", (void*) fun);
    } else if (v.isObject()) {
        JSObject* obj = &v.toObject();
        const Class* clasp = obj->getClass();
        fprintf(stderr, "<%s%s at %p>",
                clasp->name,
                (clasp == &PlainObject::class_) ? "" : " object",
                (void*) obj);
    } else if (v.isBoolean()) {
        if (v.toBoolean())
            fprintf(stderr, "true");
        else
            fprintf(stderr, "false");
    } else if (v.isMagic()) {
        fprintf(stderr, "<invalid");
#ifdef DEBUG
        switch (v.whyMagic()) {
          case JS_ELEMENTS_HOLE:     fprintf(stderr, " elements hole");      break;
          case JS_NO_ITER_VALUE:     fprintf(stderr, " no iter value");      break;
          case JS_GENERATOR_CLOSING: fprintf(stderr, " generator closing");  break;
          case JS_OPTIMIZED_OUT:     fprintf(stderr, " optimized out");      break;
          default:                   fprintf(stderr, " ?!");                 break;
        }
#endif
        fprintf(stderr, ">");
    } else {
        fprintf(stderr, "unexpected value");
    }
}

JS_FRIEND_API(void)
js::DumpValue(const Value& val)
{
    dumpValue(val);
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js::DumpId(jsid id)
{
    fprintf(stderr, "jsid %p = ", (void*) JSID_BITS(id));
    dumpValue(IdToValue(id));
    fputc('\n', stderr);
}

static void
DumpProperty(NativeObject* obj, Shape& shape)
{
    jsid id = shape.propid();
    uint8_t attrs = shape.attributes();

    fprintf(stderr, "    ((js::Shape*) %p) ", (void*) &shape);
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");

    if (shape.hasGetterValue())
        fprintf(stderr, "getterValue=%p ", (void*) shape.getterObject());
    else if (!shape.hasDefaultGetter())
        fprintf(stderr, "getterOp=%p ", JS_FUNC_TO_DATA_PTR(void*, shape.getterOp()));

    if (shape.hasSetterValue())
        fprintf(stderr, "setterValue=%p ", (void*) shape.setterObject());
    else if (!shape.hasDefaultSetter())
        fprintf(stderr, "setterOp=%p ", JS_FUNC_TO_DATA_PTR(void*, shape.setterOp()));

    if (JSID_IS_ATOM(id) || JSID_IS_INT(id) || JSID_IS_SYMBOL(id))
        dumpValue(js::IdToValue(id));
    else
        fprintf(stderr, "unknown jsid %p", (void*) JSID_BITS(id));

    uint32_t slot = shape.hasSlot() ? shape.maybeSlot() : SHAPE_INVALID_SLOT;
    fprintf(stderr, ": slot %d", slot);
    if (shape.hasSlot()) {
        fprintf(stderr, " = ");
        dumpValue(obj->getSlot(slot));
    } else if (slot != SHAPE_INVALID_SLOT) {
        fprintf(stderr, " (INVALID!)");
    }
    fprintf(stderr, "\n");
}

bool
JSObject::uninlinedIsProxy() const
{
    return is<ProxyObject>();
}

void
JSObject::dump()
{
    JSObject* obj = this;
    JSObject* globalObj = &global();
    fprintf(stderr, "object %p from global %p [%s]\n", (void*) obj,
            (void*) globalObj, globalObj->getClass()->name);
    const Class* clasp = obj->getClass();
    fprintf(stderr, "class %p %s\n", (const void*)clasp, clasp->name);

    fprintf(stderr, "flags:");
    if (obj->isDelegate()) fprintf(stderr, " delegate");
    if (!obj->is<ProxyObject>() && !obj->nonProxyIsExtensible()) fprintf(stderr, " not_extensible");
    if (obj->isIndexed()) fprintf(stderr, " indexed");
    if (obj->isBoundFunction()) fprintf(stderr, " bound_function");
    if (obj->isQualifiedVarObj()) fprintf(stderr, " varobj");
    if (obj->isUnqualifiedVarObj()) fprintf(stderr, " unqualified_varobj");
    if (obj->watched()) fprintf(stderr, " watched");
    if (obj->isIteratedSingleton()) fprintf(stderr, " iterated_singleton");
    if (obj->isNewGroupUnknown()) fprintf(stderr, " new_type_unknown");
    if (obj->hasUncacheableProto()) fprintf(stderr, " has_uncacheable_proto");
    if (obj->hadElementsAccess()) fprintf(stderr, " had_elements_access");
    if (obj->wasNewScriptCleared()) fprintf(stderr, " new_script_cleared");

    if (obj->isNative()) {
        NativeObject* nobj = &obj->as<NativeObject>();
        if (nobj->inDictionaryMode())
            fprintf(stderr, " inDictionaryMode");
        if (nobj->hasShapeTable())
            fprintf(stderr, " hasShapeTable");
    }
    fprintf(stderr, "\n");

    if (obj->isNative()) {
        NativeObject* nobj = &obj->as<NativeObject>();
        uint32_t slots = nobj->getDenseInitializedLength();
        if (slots) {
            fprintf(stderr, "elements\n");
            for (uint32_t i = 0; i < slots; i++) {
                fprintf(stderr, " %3d: ", i);
                dumpValue(nobj->getDenseElement(i));
                fprintf(stderr, "\n");
                fflush(stderr);
            }
        }
    }

    fprintf(stderr, "proto ");
    TaggedProto proto = obj->getTaggedProto();
    if (proto.isLazy())
        fprintf(stderr, "<lazy>");
    else
        dumpValue(ObjectOrNullValue(proto.toObjectOrNull()));
    fputc('\n', stderr);

    if (clasp->flags & JSCLASS_HAS_PRIVATE)
        fprintf(stderr, "private %p\n", obj->as<NativeObject>().getPrivate());

    if (!obj->isNative())
        fprintf(stderr, "not native\n");

    uint32_t reservedEnd = JSCLASS_RESERVED_SLOTS(clasp);
    uint32_t slots = obj->isNative() ? obj->as<NativeObject>().slotSpan() : 0;
    uint32_t stop = obj->isNative() ? reservedEnd : slots;
    if (stop > 0)
        fprintf(stderr, obj->isNative() ? "reserved slots:\n" : "slots:\n");
    for (uint32_t i = 0; i < stop; i++) {
        fprintf(stderr, " %3d ", i);
        if (i < reservedEnd)
            fprintf(stderr, "(reserved) ");
        fprintf(stderr, "= ");
        dumpValue(obj->as<NativeObject>().getSlot(i));
        fputc('\n', stderr);
    }

    if (obj->isNative()) {
        fprintf(stderr, "properties:\n");
        Vector<Shape*, 8, SystemAllocPolicy> props;
        for (Shape::Range<NoGC> r(obj->as<NativeObject>().lastProperty()); !r.empty(); r.popFront())
            props.append(&r.front());
        for (size_t i = props.length(); i-- != 0;)
            DumpProperty(&obj->as<NativeObject>(), *props[i]);
    }
    fputc('\n', stderr);
}

static void
MaybeDumpObject(const char* name, JSObject* obj)
{
    if (obj) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(ObjectValue(*obj));
        fputc('\n', stderr);
    }
}

static void
MaybeDumpValue(const char* name, const Value& v)
{
    if (!v.isNull()) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(v);
        fputc('\n', stderr);
    }
}

JS_FRIEND_API(void)
js::DumpInterpreterFrame(JSContext* cx, InterpreterFrame* start)
{
    
    ScriptFrameIter i(cx, ScriptFrameIter::GO_THROUGH_SAVED);
    if (!start) {
        if (i.done()) {
            fprintf(stderr, "no stack for cx = %p\n", (void*) cx);
            return;
        }
    } else {
        while (!i.done() && !i.isJit() && i.interpFrame() != start)
            ++i;

        if (i.done()) {
            fprintf(stderr, "fp = %p not found in cx = %p\n",
                    (void*)start, (void*)cx);
            return;
        }
    }

    for (; !i.done(); ++i) {
        if (i.isJit())
            fprintf(stderr, "JIT frame\n");
        else
            fprintf(stderr, "InterpreterFrame at %p\n", (void*) i.interpFrame());

        if (i.isFunctionFrame()) {
            fprintf(stderr, "callee fun: ");
            RootedValue v(cx);
            JSObject* fun = i.callee(cx);
            v.setObject(*fun);
            dumpValue(v);
        } else {
            fprintf(stderr, "global frame, no callee");
        }
        fputc('\n', stderr);

        fprintf(stderr, "file %s line %" PRIuSIZE "\n",
                i.script()->filename(), i.script()->lineno());

        if (jsbytecode* pc = i.pc()) {
            fprintf(stderr, "  pc = %p\n", pc);
            fprintf(stderr, "  current op: %s\n", js_CodeName[*pc]);
            MaybeDumpObject("staticScope", i.script()->getStaticBlockScope(pc));
        }
        MaybeDumpValue("this", i.thisv(cx));
        if (!i.isJit()) {
            fprintf(stderr, "  rval: ");
            dumpValue(i.interpFrame()->returnValue());
            fputc('\n', stderr);
        }

        fprintf(stderr, "  flags:");
        if (i.isConstructing())
            fprintf(stderr, " constructing");
        if (!i.isJit() && i.interpFrame()->isDebuggerEvalFrame())
            fprintf(stderr, " debugger eval");
        if (i.isEvalFrame())
            fprintf(stderr, " eval");
        fputc('\n', stderr);

        fprintf(stderr, "  scopeChain: (JSObject*) %p\n", (void*) i.scopeChain(cx));

        fputc('\n', stderr);
    }
}

#endif 

JS_FRIEND_API(void)
js::DumpBacktrace(JSContext* cx)
{
    Sprinter sprinter(cx);
    sprinter.init();
    size_t depth = 0;
    for (AllFramesIter i(cx); !i.done(); ++i, ++depth) {
        const char* filename = JS_GetScriptFilename(i.script());
        unsigned line = PCToLineNumber(i.script(), i.pc());
        JSScript* script = i.script();
        sprinter.printf("#%d %14p   %s:%d (%p @ %d)\n",
                        depth, (i.isJit() ? 0 : i.interpFrame()), filename, line,
                        script, script->pcToOffset(i.pc()));
    }
    fprintf(stdout, "%s", sprinter.string());
#ifdef XP_WIN32
    if (IsDebuggerPresent()) {
        OutputDebugStringA(sprinter.string());
    }
#endif
}




js::gc::AllocKind
JSObject::allocKindForTenure(const js::Nursery& nursery) const
{
    if (is<ArrayObject>()) {
        const ArrayObject& aobj = as<ArrayObject>();
        MOZ_ASSERT(aobj.numFixedSlots() == 0);

        
        if (!nursery.isInside(aobj.getElementsHeader()))
            return AllocKind::OBJECT0_BACKGROUND;

        size_t nelements = aobj.getDenseCapacity();
        return GetBackgroundAllocKind(GetGCArrayKind(nelements));
    }

    if (is<JSFunction>())
        return as<JSFunction>().getAllocKind();

    



    if (is<TypedArrayObject>() && !as<TypedArrayObject>().buffer()) {
        size_t nbytes = as<TypedArrayObject>().byteLength();
        return GetBackgroundAllocKind(TypedArrayObject::AllocKindForLazyBuffer(nbytes));
    }

    
    MOZ_ASSERT(!IsProxy(this));

    
    if (is<UnboxedPlainObject>()) {
        size_t nbytes = as<UnboxedPlainObject>().layoutDontCheckGeneration().size();
        return GetGCObjectKindForBytes(UnboxedPlainObject::offsetOfData() + nbytes);
    }

    
    
    if (is<InlineTypedObject>()) {
        
        
        
        TypeDescr& descr = as<InlineTypedObject>().typeDescr();
        MOZ_ASSERT(!IsInsideNursery(&descr));
        return InlineTypedObject::allocKindForTypeDescriptor(&descr);
    }

    
    if (is<OutlineTypedObject>())
        return AllocKind::OBJECT0;

    
    MOZ_ASSERT(isNative());

    AllocKind kind = GetGCObjectFixedSlotsKind(as<NativeObject>().numFixedSlots());
    MOZ_ASSERT(!IsBackgroundFinalized(kind));
    if (!CanBeFinalizedInBackground(kind, getClass()))
        return kind;
    return GetBackgroundAllocKind(kind);
}


void
JSObject::addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::ClassInfo* info)
{
    if (is<NativeObject>() && as<NativeObject>().hasDynamicSlots())
        info->objectsMallocHeapSlots += mallocSizeOf(as<NativeObject>().slots_);

    if (is<NativeObject>() && as<NativeObject>().hasDynamicElements()) {
        js::ObjectElements* elements = as<NativeObject>().getElementsHeader();
        if (!elements->isCopyOnWrite() || elements->ownerObject() == this)
            info->objectsMallocHeapElementsNonAsmJS += mallocSizeOf(elements);
    }

    
    if (is<JSFunction>() ||
        is<PlainObject>() ||
        is<ArrayObject>() ||
        is<CallObject>() ||
        is<RegExpObject>() ||
        is<ProxyObject>())
    {
        
        
        
        
        
        
        
        
        

    } else if (is<ArgumentsObject>()) {
        info->objectsMallocHeapMisc += as<ArgumentsObject>().sizeOfMisc(mallocSizeOf);
    } else if (is<RegExpStaticsObject>()) {
        info->objectsMallocHeapMisc += as<RegExpStaticsObject>().sizeOfData(mallocSizeOf);
    } else if (is<PropertyIteratorObject>()) {
        info->objectsMallocHeapMisc += as<PropertyIteratorObject>().sizeOfMisc(mallocSizeOf);
    } else if (is<ArrayBufferObject>()) {
        ArrayBufferObject::addSizeOfExcludingThis(this, mallocSizeOf, info);
    } else if (is<SharedArrayBufferObject>()) {
        SharedArrayBufferObject::addSizeOfExcludingThis(this, mallocSizeOf, info);
    } else if (is<AsmJSModuleObject>()) {
        as<AsmJSModuleObject>().addSizeOfMisc(mallocSizeOf, &info->objectsNonHeapCodeAsmJS,
                                              &info->objectsMallocHeapMisc);
#ifdef JS_HAS_CTYPES
    } else {
        
        info->objectsMallocHeapMisc +=
            js::SizeOfDataIfCDataObject(mallocSizeOf, const_cast<JSObject*>(this));
#endif
    }
}

void
JSObject::markChildren(JSTracer* trc)
{
    TraceEdge(trc, &group_, "group");

    const Class* clasp = group_->clasp();
    if (clasp->trace)
        clasp->trace(trc, this);

    if (clasp->isNative()) {
        NativeObject* nobj = &as<NativeObject>();

        TraceEdge(trc, &nobj->shape_, "shape");

        MarkObjectSlots(trc, nobj, 0, nobj->slotSpan());

        do {
            if (nobj->denseElementsAreCopyOnWrite()) {
                HeapPtrNativeObject& owner = nobj->getElementsHeader()->ownerObject();
                if (owner != nobj) {
                    TraceEdge(trc, &owner, "objectElementsOwner");
                    break;
                }
            }

            TraceRange(trc,
                       nobj->getDenseInitializedLength(),
                       static_cast<HeapSlot*>(nobj->getDenseElementsAllowCopyOnWrite()),
                       "objectElements");
        } while (false);
    }
}
