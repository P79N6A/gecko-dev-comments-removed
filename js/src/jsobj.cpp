









#include "jsobjinlines.h"

#include "mozilla/ArrayUtils.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/TemplateLib.h"

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
#include "jsproxy.h"
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
using namespace js::types;

using mozilla::DebugOnly;
using mozilla::Maybe;

JS_FRIEND_API(JSObject *)
JS_ObjectToInnerObject(JSContext *cx, HandleObject obj)
{
    if (!obj) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INACTIVE);
        return nullptr;
    }
    return GetInnerObject(obj);
}

JS_FRIEND_API(JSObject *)
JS_ObjectToOuterObject(JSContext *cx, HandleObject obj)
{
    assertSameCompartment(cx, obj);
    return GetOuterObject(cx, obj);
}

JSObject *
js::NonNullObject(JSContext *cx, const Value &v)
{
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT);
        return nullptr;
    }
    return &v.toObject();
}

const char *
js::InformalValueTypeName(const Value &v)
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
js::NewPropertyDescriptorObject(JSContext *cx, Handle<PropertyDescriptor> desc,
                                MutableHandleValue vp)
{
    if (!desc.object()) {
        vp.setUndefined();
        return true;
    }

    Rooted<PropDesc> d(cx);

    d.initFromPropertyDescriptor(desc);
    RootedObject descObj(cx);
    if (!d.makeObject(cx, &descObj))
        return false;
    vp.setObject(*descObj);
    return true;
}

void
PropDesc::initFromPropertyDescriptor(Handle<PropertyDescriptor> desc)
{
    MOZ_ASSERT(isUndefined());

    if (!desc.object())
        return;

    isUndefined_ = false;
    attrs = uint8_t(desc.attributes());
    MOZ_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    if (desc.hasGetterOrSetterObject()) {
        hasGet_ = true;
        get_ = desc.hasGetterObject() && desc.getterObject()
               ? ObjectValue(*desc.getterObject())
               : UndefinedValue();
        hasSet_ = true;
        set_ = desc.hasSetterObject() && desc.setterObject()
               ? ObjectValue(*desc.setterObject())
               : UndefinedValue();
        hasValue_ = false;
        value_.setUndefined();
        hasWritable_ = false;
    } else {
        hasGet_ = false;
        get_.setUndefined();
        hasSet_ = false;
        set_.setUndefined();
        hasValue_ = !(desc.attributes() & JSPROP_IGNORE_VALUE);
        value_ = hasValue_ ? desc.value() : UndefinedValue();
        hasWritable_ = !(desc.attributes() & JSPROP_IGNORE_READONLY);
    }
    hasEnumerable_ = !(desc.attributes() & JSPROP_IGNORE_ENUMERATE);
    hasConfigurable_ = !(desc.attributes() & JSPROP_IGNORE_PERMANENT);
}

void
PropDesc::populatePropertyDescriptor(HandleObject obj, MutableHandle<PropertyDescriptor> desc) const
{
    if (isUndefined()) {
        desc.object().set(nullptr);
        return;
    }

    desc.value().set(hasValue() ? value() : UndefinedValue());
    desc.setGetter(getter());
    desc.setSetter(setter());

    
    unsigned attrs = attributes();
    if (!hasEnumerable())
        attrs |= JSPROP_IGNORE_ENUMERATE;
    if (!hasWritable())
        attrs |= JSPROP_IGNORE_READONLY;
    if (!hasConfigurable())
        attrs |= JSPROP_IGNORE_PERMANENT;
    if (!hasValue())
        attrs |= JSPROP_IGNORE_VALUE;
    desc.setAttributes(attrs);

    desc.object().set(obj);
}

bool
PropDesc::makeObject(JSContext *cx, MutableHandleObject obj)
{
    MOZ_ASSERT(!isUndefined());

    obj.set(NewBuiltinClassInstance<PlainObject>(cx));
    if (!obj)
        return false;

    const JSAtomState &names = cx->names();
    RootedValue configurableVal(cx, BooleanValue((attrs & JSPROP_PERMANENT) == 0));
    RootedValue enumerableVal(cx, BooleanValue((attrs & JSPROP_ENUMERATE) != 0));
    RootedValue writableVal(cx, BooleanValue((attrs & JSPROP_READONLY) == 0));
    if ((hasConfigurable() &&
         !DefineProperty(cx, obj, names.configurable, configurableVal)) ||
        (hasEnumerable() &&
         !DefineProperty(cx, obj, names.enumerable, enumerableVal)) ||
        (hasGet() &&
         !DefineProperty(cx, obj, names.get, getterValue())) ||
        (hasSet() &&
         !DefineProperty(cx, obj, names.set, setterValue())) ||
        (hasValue() &&
         !DefineProperty(cx, obj, names.value, value())) ||
        (hasWritable() &&
         !DefineProperty(cx, obj, names.writable, writableVal)))
    {
        return false;
    }

    return true;
}

bool
js::GetFirstArgumentAsObject(JSContext *cx, const CallArgs &args, const char *method,
                             MutableHandleObject objp)
{
    if (args.length() == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             method, "0", "s");
        return false;
    }

    HandleValue v = args[0];
    if (!v.isObject()) {
        char *bytes = DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, v, NullPtr());
        if (!bytes)
            return false;
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_UNEXPECTED_TYPE,
                             bytes, "not an object");
        js_free(bytes);
        return false;
    }

    objp.set(&v.toObject());
    return true;
}

static bool
GetPropertyIfPresent(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp,
                     bool *foundp)
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
PropDesc::initialize(JSContext *cx, const Value &origval, bool checkAccessors)
{
    MOZ_ASSERT(isUndefined());

    RootedValue v(cx, origval);

    
    if (v.isPrimitive()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_NONNULL_OBJECT);
        return false;
    }
    RootedObject desc(cx, &v.toObject());

    isUndefined_ = false;

    



    attrs = JSPROP_PERMANENT | JSPROP_READONLY;

    bool found = false;
    RootedId id(cx);

    
    id = NameToId(cx->names().enumerable);
    if (!GetPropertyIfPresent(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasEnumerable_ = true;
        if (ToBoolean(v))
            attrs |= JSPROP_ENUMERATE;
    }

    
    id = NameToId(cx->names().configurable);
    if (!GetPropertyIfPresent(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasConfigurable_ = true;
        if (ToBoolean(v))
            attrs &= ~JSPROP_PERMANENT;
    }

    
    id = NameToId(cx->names().value);
    if (!GetPropertyIfPresent(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasValue_ = true;
        value_ = v;
    }

    
    id = NameToId(cx->names().writable);
    if (!GetPropertyIfPresent(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasWritable_ = true;
        if (ToBoolean(v))
            attrs &= ~JSPROP_READONLY;
    }

    
    id = NameToId(cx->names().get);
    if (!GetPropertyIfPresent(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasGet_ = true;
        get_ = v;
        attrs |= JSPROP_GETTER | JSPROP_SHARED;
        attrs &= ~JSPROP_READONLY;
        if (checkAccessors && !checkGetter(cx))
            return false;
    }

    
    id = NameToId(cx->names().set);
    if (!GetPropertyIfPresent(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasSet_ = true;
        set_ = v;
        attrs |= JSPROP_SETTER | JSPROP_SHARED;
        attrs &= ~JSPROP_READONLY;
        if (checkAccessors && !checkSetter(cx))
            return false;
    }

    
    if ((hasGet() || hasSet()) && (hasValue() || hasWritable())) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INVALID_DESCRIPTOR);
        return false;
    }

    MOZ_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

    return true;
}

void
PropDesc::complete()
{
    MOZ_ASSERT(!isUndefined());

    if (isGenericDescriptor() || isDataDescriptor()) {
        if (!hasValue_) {
            hasValue_ = true;
            value_.setUndefined();
        }
        if (!hasWritable_) {
            hasWritable_ = true;
            attrs |= JSPROP_READONLY;
        }
    } else {
        if (!hasGet_) {
            hasGet_ = true;
            get_.setUndefined();
        }
        if (!hasSet_) {
            hasSet_ = true;
            set_.setUndefined();
        }
    }
    if (!hasEnumerable_) {
        hasEnumerable_ = true;
        attrs &= ~JSPROP_ENUMERATE;
    }
    if (!hasConfigurable_) {
        hasConfigurable_ = true;
        attrs |= JSPROP_PERMANENT;
    }
}

bool
js::Throw(JSContext *cx, jsid id, unsigned errorNumber)
{
    MOZ_ASSERT(js_ErrorFormatString[errorNumber].argCount == 1);

    JSString *idstr = IdToString(cx, id);
    if (!idstr)
       return false;
    JSAutoByteString bytes(cx, idstr);
    if (!bytes)
        return false;
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, errorNumber, bytes.ptr());
    return false;
}

bool
js::Throw(JSContext *cx, JSObject *obj, unsigned errorNumber)
{
    if (js_ErrorFormatString[errorNumber].argCount == 1) {
        RootedValue val(cx, ObjectValue(*obj));
        js_ReportValueErrorFlags(cx, JSREPORT_ERROR, errorNumber,
                                 JSDVG_IGNORE_STACK, val, NullPtr(),
                                 nullptr, nullptr);
    } else {
        MOZ_ASSERT(js_ErrorFormatString[errorNumber].argCount == 0);
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, errorNumber);
    }
    return false;
}

static bool
Reject(JSContext *cx, unsigned errorNumber, bool throwError, jsid id, bool *rval)
{
    if (throwError)
        return Throw(cx, id, errorNumber);

    *rval = false;
    return true;
}

static bool
Reject(JSContext *cx, JSObject *obj, unsigned errorNumber, bool throwError, bool *rval)
{
    if (throwError)
        return Throw(cx, obj, errorNumber);

    *rval = false;
    return true;
}

static bool
Reject(JSContext *cx, HandleId id, unsigned errorNumber, bool throwError, bool *rval)
{
    if (throwError)
        return Throw(cx, id, errorNumber);

    *rval = false;
    return true;
}

static unsigned
ApplyOrDefaultAttributes(unsigned attrs, Handle<PropertyDescriptor> desc)
{
    bool present = !!desc.object();
    bool enumerable = present ? desc.isEnumerable() : false;
    bool writable = present ? !desc.isReadonly() : false;
    bool configurable = present ? !desc.isPermanent() : false;
    return ApplyAttributes(attrs, enumerable, writable, configurable);
}





JS_FRIEND_API(bool)
js::CheckDefineProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                        unsigned attrs, PropertyOp getter, StrictPropertyOp setter)
{
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    if (!obj->isNative())
        return true;

    
    
    Rooted<PropertyDescriptor> desc(cx);
    if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;

    
    
    
    attrs = ApplyOrDefaultAttributes(attrs, desc) & ~JSPROP_IGNORE_VALUE;

    
    
    
    if (desc.object() && desc.isPermanent()) {
        
        
        
        if (getter != desc.getter() ||
            setter != desc.setter() ||
            (attrs != desc.attributes() && attrs != (desc.attributes() | JSPROP_READONLY)))
        {
            return Throw(cx, id, JSMSG_CANT_REDEFINE_PROP);
        }

        
        
        if ((desc.attributes() & (JSPROP_GETTER | JSPROP_SETTER | JSPROP_READONLY)) == JSPROP_READONLY) {
            bool same;
            if (!SameValue(cx, value, desc.value(), &same))
                return false;
            if (!same)
                return JSObject::reportReadOnly(cx, id);
        }
    }
    return true;
}




static bool
DefinePropertyOnObject(JSContext *cx, HandleNativeObject obj, HandleId id, const PropDesc &desc,
                       bool throwError, bool *rval)
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
            return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);

        *rval = true;

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            MOZ_ASSERT(!obj->getOps()->defineProperty);
            RootedValue v(cx, desc.hasValue() ? desc.value() : UndefinedValue());
            return NativeDefineProperty(cx, obj, id, v, nullptr, nullptr, desc.attributes());
        }

        MOZ_ASSERT(desc.isAccessorDescriptor());

        return NativeDefineProperty(cx, obj, id, UndefinedHandleValue,
                                    desc.getter(), desc.setter(), desc.attributes());
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

            if (desc.hasGet()) {
                bool same;
                if (!SameValue(cx, desc.getterValue(), shape->getterOrUndefined(), &same))
                    return false;
                if (!same)
                    break;
            }

            if (desc.hasSet()) {
                bool same;
                if (!SameValue(cx, desc.setterValue(), shape->setterOrUndefined(), &same))
                    return false;
                if (!same)
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
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
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
                            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
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

        
        *rval = true;
        return true;
    } while (0);

    
    if (!shapeConfigurable) {
        if ((desc.hasConfigurable() && desc.configurable()) ||
            (desc.hasEnumerable() && desc.enumerable() != shape->enumerable())) {
            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
        }
    }

    bool callDelProperty = false;

    if (desc.isGenericDescriptor()) {
        
    } else if (desc.isDataDescriptor() != shapeDataDescriptor) {
        
        if (!shapeConfigurable)
            return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
    } else if (desc.isDataDescriptor()) {
        
        MOZ_ASSERT(shapeDataDescriptor);
        if (!shapeConfigurable && !shape->writable()) {
            if (desc.hasWritable() && desc.writable())
                return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            if (desc.hasValue()) {
                bool same;
                if (!SameValue(cx, desc.value(), v, &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }
        }

        callDelProperty = !shapeHasDefaultGetter || !shapeHasDefaultSetter;
    } else {
        
        MOZ_ASSERT(desc.isAccessorDescriptor() && shape->isAccessorDescriptor());
        if (!shape->configurable()) {
            if (desc.hasSet()) {
                bool same;
                if (!SameValue(cx, desc.setterValue(), shape->setterOrUndefined(), &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }

            if (desc.hasGet()) {
                bool same;
                if (!SameValue(cx, desc.getterValue(), shape->getterOrUndefined(), &same))
                    return false;
                if (!same)
                    return Reject(cx, JSMSG_CANT_REDEFINE_PROP, throwError, id, rval);
            }
        }
    }

    
    unsigned attrs;
    PropertyOp getter;
    StrictPropertyOp setter;
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
        unsigned unchanged = 0;
        if (!desc.hasConfigurable())
            unchanged |= JSPROP_PERMANENT;
        if (!desc.hasEnumerable())
            unchanged |= JSPROP_ENUMERATE;
        
        if (!desc.hasWritable() && shapeDataDescriptor)
            unchanged |= JSPROP_READONLY;

        if (desc.hasValue())
            v = desc.value();
        attrs = (desc.attributes() & ~unchanged) | (shapeAttributes & unchanged);
        getter = nullptr;
        setter = nullptr;
    } else {
        MOZ_ASSERT(desc.isAccessorDescriptor());

        
        unsigned changed = 0;
        if (desc.hasConfigurable())
            changed |= JSPROP_PERMANENT;
        if (desc.hasEnumerable())
            changed |= JSPROP_ENUMERATE;
        if (desc.hasGet())
            changed |= JSPROP_GETTER | JSPROP_SHARED | JSPROP_READONLY;
        if (desc.hasSet())
            changed |= JSPROP_SETTER | JSPROP_SHARED | JSPROP_READONLY;

        attrs = (desc.attributes() & changed) | (shapeAttributes & ~changed);
        if (desc.hasGet()) {
            getter = desc.getter();
        } else {
            getter = (shapeHasDefaultGetter && !shapeHasGetterValue)
                     ? nullptr
                     : shape->getter();
        }
        if (desc.hasSet()) {
            setter = desc.setter();
        } else {
            setter = (shapeHasDefaultSetter && !shapeHasSetterValue)
                     ? nullptr
                     : shape->setter();
        }
    }

    *rval = true;

    








    if (callDelProperty) {
        bool succeeded;
        if (!CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, id, &succeeded))
            return false;
    }

    return NativeDefineProperty(cx, obj, id, v, getter, setter, attrs);
}


static bool
DefinePropertyOnArray(JSContext *cx, Handle<ArrayObject*> arr, HandleId id, const PropDesc &desc,
                      bool throwError, bool *rval)
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
            return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);
        if (desc.hasEnumerable() && desc.enumerable())
            return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);

        if (desc.isAccessorDescriptor())
            return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);

        unsigned attrs = arr->lookup(cx, id)->attributes();
        if (!arr->lengthIsWritable()) {
            if (desc.hasWritable() && desc.writable())
                return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);
        } else {
            if (desc.hasWritable() && !desc.writable())
                attrs = attrs | JSPROP_READONLY;
        }

        return ArraySetLength(cx, arr, id, attrs, v, throwError);
    }

    
    uint32_t index;
    if (js_IdIsIndex(id, &index)) {
        
        uint32_t oldLen = arr->length();

        
        if (index >= oldLen && !arr->lengthIsWritable())
            return Reject(cx, arr, JSMSG_CANT_APPEND_TO_ARRAY, throwError, rval);

        
        return DefinePropertyOnObject(cx, arr, id, desc, throwError, rval);
    }

    
    return DefinePropertyOnObject(cx, arr, id, desc, throwError, rval);
}


static bool
DefinePropertyOnTypedArray(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                           bool throwError, bool *rval)
{

    MOZ_ASSERT(IsAnyTypedArray(obj));
    
    uint64_t index;
    if (IsTypedArrayIndex(id, &index)) {
        
        
        
        if (index >= AnyTypedArrayLength(obj)) {
            *rval = true;
            return true;
        }

        
        if (desc.isAccessorDescriptor())
            return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);

        
        if (desc.hasConfigurable() && desc.configurable())
            return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);

        
        if (desc.hasEnumerable() && !desc.enumerable())
            return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);

        
        if (desc.hasWritable() && !desc.writable())
            return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);

        
        if (desc.hasValue()) {
            double d;
            if (!ToNumber(cx, desc.value(), &d))
                return false;

            if (obj->is<TypedArrayObject>())
                TypedArrayObject::setElement(obj->as<TypedArrayObject>(), index, d);
            else
                SharedTypedArrayObject::setElement(obj->as<SharedTypedArrayObject>(), index, d);
        }

        
        *rval = true;
        return true;
    }

    
    return DefinePropertyOnObject(cx, obj.as<NativeObject>(), id, desc, throwError, rval);
}

bool
js::StandardDefineProperty(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                           bool throwError, bool *rval)
{
    if (obj->is<ArrayObject>()) {
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        return DefinePropertyOnArray(cx, arr, id, desc, throwError, rval);
    }

    if (IsAnyTypedArray(obj))
        return DefinePropertyOnTypedArray(cx, obj, id, desc, throwError, rval);

    if (obj->is<UnboxedPlainObject>() && !obj->as<UnboxedPlainObject>().convertToNative(cx))
        return false;

    if (obj->getOps()->lookupProperty) {
        if (obj->is<ProxyObject>()) {
            Rooted<PropertyDescriptor> pd(cx);
            desc.populatePropertyDescriptor(obj, &pd);
            pd.object().set(obj);
            return Proxy::defineProperty(cx, obj, id, &pd);
        }
        return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);
    }

    return DefinePropertyOnObject(cx, obj.as<NativeObject>(), id, desc, throwError, rval);
}

bool
js::StandardDefineProperty(JSContext *cx, HandleObject obj, HandleId id,
                           Handle<PropertyDescriptor> descriptor, bool *bp)
{
    Rooted<PropDesc> desc(cx);
    desc.initFromPropertyDescriptor(descriptor);
    return StandardDefineProperty(cx, obj, id, desc, true, bp);
}

bool
js::ReadPropertyDescriptors(JSContext *cx, HandleObject props, bool checkAccessors,
                            AutoIdVector *ids, AutoPropDescVector *descs)
{
    if (!GetPropertyKeys(cx, props, JSITER_OWNONLY | JSITER_SYMBOLS, ids))
        return false;

    RootedId id(cx);
    for (size_t i = 0, len = ids->length(); i < len; i++) {
        id = (*ids)[i];
        Rooted<PropDesc> desc(cx);
        RootedValue v(cx);
        if (!GetProperty(cx, props, props, id, &v) ||
            !desc.initialize(cx, v, checkAccessors) ||
            !descs->append(desc))
        {
            return false;
        }
    }
    return true;
}

bool
js::DefineProperties(JSContext *cx, HandleObject obj, HandleObject props)
{
    AutoIdVector ids(cx);
    AutoPropDescVector descs(cx);
    if (!ReadPropertyDescriptors(cx, props, true, &ids, &descs))
        return false;

    bool dummy;
    for (size_t i = 0, len = ids.length(); i < len; i++) {
        if (!StandardDefineProperty(cx, obj, ids[i], descs[i], true, &dummy))
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
js::SetIntegrityLevel(JSContext *cx, HandleObject obj, IntegrityLevel level)
{
    assertSameCompartment(cx, obj);

    
    bool status;
    if (!PreventExtensions(cx, obj, &status))
        return false;
    if (!status) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CHANGE_EXTENSIBILITY);
        return false;
    }

    
    AutoIdVector keys(cx);
    if (!GetPropertyKeys(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY | JSITER_SYMBOLS, &keys))
        return false;

    
    
    MOZ_ASSERT_IF(obj->isNative(), obj->as<NativeObject>().getDenseCapacity() == 0);

    
    if (obj->isNative() && !obj->as<NativeObject>().inDictionaryMode() && !IsAnyTypedArray(obj)) {
        HandleNativeObject nobj = obj.as<NativeObject>();

        
        
        
        
        
        RootedShape last(cx, EmptyShape::getInitialShape(cx, nobj->getClass(),
                                                         nobj->getTaggedProto(),
                                                         nobj->getParent(),
                                                         nobj->getMetadata(),
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
        JS_ALWAYS_TRUE(NativeObject::setLastProperty(cx, nobj, last));
    } else {
        RootedId id(cx);
        Rooted<PropertyDescriptor> desc(cx);
        for (size_t i = 0; i < keys.length(); i++) {
            id = keys[i];

            if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
                return false;

            if (!desc.object())
                continue;

            unsigned attrs = desc.attributes();
            unsigned new_attrs = GetSealedOrFrozenAttributes(attrs, level);

            
            if ((attrs | new_attrs) == attrs)
                continue;

            attrs |= new_attrs;
            if (!SetPropertyAttributes(cx, obj, id, &attrs))
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
js::TestIntegrityLevel(JSContext *cx, HandleObject obj, IntegrityLevel level, bool *result)
{
    
    bool status;
    if (!IsExtensible(cx, obj, &status))
        return false;
    if (status) {
        *result = false;
        return true;
    }

    if (IsAnyTypedArray(obj)) {
        if (level == IntegrityLevel::Sealed) {
            
            *result = true;
        } else {
            
            
            *result = (AnyTypedArrayLength(obj) == 0);
        }
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

        
        
        
        if (!desc.isPermanent() ||
            (level == IntegrityLevel::Frozen && desc.isDataDescriptor() && desc.isWritable()))
        {
            *result = false;
            return true;
        }
    }

    
    *result = true;
    return true;
}








static inline gc::AllocKind
NewObjectGCKind(const js::Class *clasp)
{
    if (clasp == &ArrayObject::class_)
        return gc::FINALIZE_OBJECT8;
    if (clasp == &JSFunction::class_)
        return gc::FINALIZE_OBJECT2;
    return gc::FINALIZE_OBJECT4;
}

static inline JSObject *
NewObject(ExclusiveContext *cx, ObjectGroup *groupArg, JSObject *parent, gc::AllocKind kind,
          NewObjectKind newKind)
{
    const Class *clasp = groupArg->clasp();

    MOZ_ASSERT(clasp != &ArrayObject::class_);
    MOZ_ASSERT_IF(clasp == &JSFunction::class_,
                  kind == JSFunction::FinalizeKind || kind == JSFunction::ExtendedFinalizeKind);
    MOZ_ASSERT_IF(parent, &parent->global() == cx->global());

    RootedObjectGroup group(cx, groupArg);

    JSObject *metadata = nullptr;
    if (!NewObjectMetadata(cx, &metadata))
        return nullptr;

    
    
    
    size_t nfixed = ClassCanHaveFixedData(clasp)
                    ? GetGCKindSlots(gc::GetGCObjectKind(clasp), clasp)
                    : GetGCKindSlots(kind, clasp);

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, clasp, group->proto(),
                                                      parent, metadata, nfixed));
    if (!shape)
        return nullptr;

    gc::InitialHeap heap = GetInitialHeap(newKind, clasp);
    JSObject *obj = JSObject::create(cx, kind, heap, shape, group);
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
NewObjectCache::fillProto(EntryIndex entry, const Class *clasp, js::TaggedProto proto,
                          gc::AllocKind kind, NativeObject *obj)
{
    MOZ_ASSERT_IF(proto.isObject(), !proto.toObject()->is<GlobalObject>());
    MOZ_ASSERT(obj->getTaggedProto() == proto);
    return fill(entry, clasp, proto.raw(), kind, obj);
}

JSObject *
js::NewObjectWithGivenProto(ExclusiveContext *cxArg, const js::Class *clasp,
                            js::TaggedProto protoArg, JSObject *parentArg,
                            gc::AllocKind allocKind, NewObjectKind newKind)
{
    if (CanBeFinalizedInBackground(allocKind, clasp))
        allocKind = GetBackgroundAllocKind(allocKind);

    NewObjectCache::EntryIndex entry = -1;
    uint64_t gcNumber = 0;
    if (JSContext *cx = cxArg->maybeJSContext()) {
        JSRuntime *rt = cx->runtime();
        NewObjectCache &cache = rt->newObjectCache;
        if (protoArg.isObject() &&
            newKind == GenericObject &&
            clasp->isNative() &&
            !cx->compartment()->hasObjectMetadataCallback() &&
            (!parentArg || parentArg == protoArg.toObject()->getParent()) &&
            !protoArg.toObject()->is<GlobalObject>())
        {
            if (cache.lookupProto(clasp, protoArg.toObject(), allocKind, &entry)) {
                JSObject *obj = cache.newObjectFromHit<NoGC>(cx, entry, GetInitialHeap(newKind, clasp));
                if (obj) {
                    return obj;
                } else {
                    Rooted<TaggedProto> proto(cxArg, protoArg);
                    RootedObject parent(cxArg, parentArg);
                    obj = cache.newObjectFromHit<CanGC>(cx, entry, GetInitialHeap(newKind, clasp));
                    MOZ_ASSERT(!obj);
                    parentArg = parent;
                    protoArg = proto;
                }
            } else {
                gcNumber = rt->gc.gcNumber();
            }
        }
    }

    Rooted<TaggedProto> proto(cxArg, protoArg);
    RootedObject parent(cxArg, parentArg);

    ObjectGroup *group = ObjectGroup::defaultNewGroup(cxArg, clasp, proto, nullptr);
    if (!group)
        return nullptr;

    



    if (!parent && proto.isObject())
        parent = proto.toObject()->getParent();

    RootedObject obj(cxArg, NewObject(cxArg, group, parent, allocKind, newKind));
    if (!obj)
        return nullptr;

    if (entry != -1 && !obj->as<NativeObject>().hasDynamicSlots() &&
        cxArg->asJSContext()->runtime()->gc.gcNumber() == gcNumber)
    {
        cxArg->asJSContext()->runtime()->newObjectCache.fillProto(entry, clasp,
                                                                  proto, allocKind,
                                                                  &obj->as<NativeObject>());
    }

    return obj;
}

static JSProtoKey
ClassProtoKeyOrAnonymousOrNull(const js::Class *clasp)
{
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(clasp);
    if (key != JSProto_Null)
        return key;
    if (clasp->flags & JSCLASS_IS_ANONYMOUS)
        return JSProto_Object;
    return JSProto_Null;
}

static inline bool
NativeGetPureInline(NativeObject *pobj, Shape *shape, MutableHandleValue vp)
{
    if (shape->hasSlot()) {
        vp.set(pobj->getSlot(shape->slot()));
        MOZ_ASSERT(!vp.isMagic());
    } else {
        vp.setUndefined();
    }

    
    return shape->hasDefaultGetter();
}

static bool
FindClassPrototype(ExclusiveContext *cx, MutableHandleObject protop, const Class *clasp)
{
    protop.set(nullptr);

    JSAtom *atom = Atomize(cx, clasp->name, strlen(clasp->name));
    if (!atom)
        return false;
    RootedId id(cx, AtomToId(atom));

    RootedObject pobj(cx);
    RootedShape shape(cx);
    if (!NativeLookupProperty<CanGC>(cx, cx->global(), id, &pobj, &shape))
        return false;

    RootedObject ctor(cx);
    if (shape && pobj->isNative()) {
        if (shape->hasSlot()) {
            RootedValue v(cx, pobj->as<NativeObject>().getSlot(shape->slot()));
            if (v.isObject())
                ctor = &v.toObject();
        }
    }

    if (ctor && ctor->is<JSFunction>()) {
        JSFunction *nctor = &ctor->as<JSFunction>();
        RootedValue v(cx);
        if (cx->isJSContext()) {
            if (!GetProperty(cx->asJSContext(), ctor, ctor, cx->names().prototype, &v))
                return false;
        } else {
            Shape *shape = nctor->lookup(cx, cx->names().prototype);
            if (!shape || !NativeGetPureInline(nctor, shape, &v))
                return false;
        }
        if (v.isObject())
            protop.set(&v.toObject());
    }
    return true;
}








static bool
FindProto(ExclusiveContext *cx, const js::Class *clasp, MutableHandleObject proto)
{
    JSProtoKey protoKey = ClassProtoKeyOrAnonymousOrNull(clasp);
    if (protoKey != JSProto_Null)
        return GetBuiltinPrototype(cx, protoKey, proto);

    if (!FindClassPrototype(cx, proto, clasp))
        return false;

    if (!proto) {
        
        
        
        
        MOZ_ASSERT(JSCLASS_CACHED_PROTO_KEY(clasp) == JSProto_Null);
        return GetBuiltinPrototype(cx, JSProto_Object, proto);
    }
    return true;
}


JSObject *
js::NewObjectWithClassProtoCommon(ExclusiveContext *cxArg,
                                  const js::Class *clasp, JSObject *protoArg, JSObject *parentArg,
                                  gc::AllocKind allocKind, NewObjectKind newKind)
{
    if (protoArg)
        return NewObjectWithGivenProto(cxArg, clasp, TaggedProto(protoArg), parentArg, allocKind, newKind);

    if (CanBeFinalizedInBackground(allocKind, clasp))
        allocKind = GetBackgroundAllocKind(allocKind);

    if (!parentArg)
        parentArg = cxArg->global();

    








    JSProtoKey protoKey = ClassProtoKeyOrAnonymousOrNull(clasp);

    NewObjectCache::EntryIndex entry = -1;
    uint64_t gcNumber = 0;
    if (JSContext *cx = cxArg->maybeJSContext()) {
        JSRuntime *rt = cx->runtime();
        NewObjectCache &cache = rt->newObjectCache;
        if (parentArg->is<GlobalObject>() &&
            protoKey != JSProto_Null &&
            newKind == GenericObject &&
            clasp->isNative() &&
            !cx->compartment()->hasObjectMetadataCallback())
        {
            if (cache.lookupGlobal(clasp, &parentArg->as<GlobalObject>(), allocKind, &entry)) {
                JSObject *obj = cache.newObjectFromHit<NoGC>(cx, entry, GetInitialHeap(newKind, clasp));
                if (obj) {
                    return obj;
                } else {
                    RootedObject parent(cxArg, parentArg);
                    RootedObject proto(cxArg, protoArg);
                    obj = cache.newObjectFromHit<CanGC>(cx, entry, GetInitialHeap(newKind, clasp));
                    MOZ_ASSERT(!obj);
                    protoArg = proto;
                    parentArg = parent;
                }
            } else {
                gcNumber = rt->gc.gcNumber();
            }
        }
    }

    RootedObject parent(cxArg, parentArg);
    RootedObject proto(cxArg, protoArg);

    if (!FindProto(cxArg, clasp, &proto))
        return nullptr;

    Rooted<TaggedProto> taggedProto(cxArg, TaggedProto(proto));
    ObjectGroup *group = ObjectGroup::defaultNewGroup(cxArg, clasp, taggedProto);
    if (!group)
        return nullptr;

    JSObject *obj = NewObject(cxArg, group, parent, allocKind, newKind);
    if (!obj)
        return nullptr;

    if (entry != -1 && !obj->as<NativeObject>().hasDynamicSlots() &&
        cxArg->asJSContext()->runtime()->gc.gcNumber() == gcNumber)
    {
        cxArg->asJSContext()->runtime()->newObjectCache.fillGlobal(entry, clasp,
                                                                   &parent->as<GlobalObject>(),
                                                                   allocKind, &obj->as<NativeObject>());
    }

    return obj;
}





JSObject *
js::NewObjectWithGroupCommon(JSContext *cx, HandleObjectGroup group, JSObject *parent,
                             gc::AllocKind allocKind, NewObjectKind newKind)
{
    MOZ_ASSERT(parent);

    MOZ_ASSERT(allocKind <= gc::FINALIZE_OBJECT_LAST);
    if (CanBeFinalizedInBackground(allocKind, group->clasp()))
        allocKind = GetBackgroundAllocKind(allocKind);

    NewObjectCache &cache = cx->runtime()->newObjectCache;

    NewObjectCache::EntryIndex entry = -1;
    if (group->proto().isObject() &&
        parent == group->proto().toObject()->getParent() &&
        newKind == GenericObject &&
        group->clasp()->isNative() &&
        !cx->compartment()->hasObjectMetadataCallback())
    {
        if (cache.lookupGroup(group, allocKind, &entry)) {
            JSObject *obj = cache.newObjectFromHit<NoGC>(cx, entry, GetInitialHeap(newKind, group->clasp()));
            if (obj) {
                return obj;
            } else {
                obj = cache.newObjectFromHit<CanGC>(cx, entry, GetInitialHeap(newKind, group->clasp()));
                parent = group->proto().toObject()->getParent();
            }
        }
    }

    JSObject *obj = NewObject(cx, group, parent, allocKind, newKind);
    if (!obj)
        return nullptr;

    if (entry != -1 && !obj->as<NativeObject>().hasDynamicSlots())
        cache.fillGroup(entry, group, allocKind, &obj->as<NativeObject>());

    return obj;
}

bool
js::NewObjectScriptedCall(JSContext *cx, MutableHandleObject pobj)
{
    jsbytecode *pc;
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
js::CreateThis(JSContext *cx, const Class *newclasp, HandleObject callee)
{
    RootedValue protov(cx);
    if (!GetProperty(cx, callee, callee, cx->names().prototype, &protov))
        return nullptr;

    JSObject *proto = protov.isObjectOrNull() ? protov.toObjectOrNull() : nullptr;
    JSObject *parent = callee->getParent();
    gc::AllocKind kind = NewObjectGCKind(newclasp);
    return NewObjectWithClassProto(cx, newclasp, proto, parent, kind);
}

static inline JSObject *
CreateThisForFunctionWithGroup(JSContext *cx, HandleObjectGroup group, JSObject *parent,
                               NewObjectKind newKind)
{
    if (group->maybeUnboxedLayout() && newKind != SingletonObject)
        return UnboxedPlainObject::create(cx, group, newKind);

    if (types::TypeNewScript *newScript = group->newScript()) {
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
            newKind = MaybeSingletonObject;

        
        
        
        gc::AllocKind allocKind = GuessObjectGCKind(NativeObject::MAX_FIXED_SLOTS);
        PlainObject *res = NewObjectWithGroup<PlainObject>(cx, group, parent, allocKind, newKind);
        if (!res)
            return nullptr;

        if (newKind != SingletonObject)
            newScript->registerNewObject(res);

        return res;
    }

    gc::AllocKind allocKind = NewObjectGCKind(&PlainObject::class_);
    return NewObjectWithGroup<PlainObject>(cx, group, parent, allocKind, newKind);
}

JSObject *
js::CreateThisForFunctionWithProto(JSContext *cx, HandleObject callee, JSObject *proto,
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

        res = CreateThisForFunctionWithGroup(cx, group, callee->getParent(), newKind);
    } else {
        gc::AllocKind allocKind = NewObjectGCKind(&PlainObject::class_);
        res = NewObjectWithProto<PlainObject>(cx, proto, callee->getParent(), allocKind, newKind);
    }

    if (res) {
        JSScript *script = callee->as<JSFunction>().getOrCreateScript(cx);
        if (!script)
            return nullptr;
        TypeScript::SetThis(cx, script, types::Type::ObjectType(res));
    }

    return res;
}

JSObject *
js::CreateThisForFunction(JSContext *cx, HandleObject callee, NewObjectKind newKind)
{
    RootedValue protov(cx);
    if (!GetProperty(cx, callee, callee, cx->names().prototype, &protov))
        return nullptr;
    JSObject *proto;
    if (protov.isObject())
        proto = &protov.toObject();
    else
        proto = nullptr;
    JSObject *obj = CreateThisForFunctionWithProto(cx, callee, proto, newKind);

    if (obj && newKind == SingletonObject) {
        RootedPlainObject nobj(cx, &obj->as<PlainObject>());

        
        NativeObject::clear(cx, nobj);

        JSScript *calleeScript = callee->as<JSFunction>().nonLazyScript();
        TypeScript::SetThis(cx, calleeScript, types::Type::ObjectType(nobj));

        return nobj;
    }

    return obj;
}

 bool
JSObject::nonNativeSetProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                               HandleId id, MutableHandleValue vp, bool strict)
{
    if (MOZ_UNLIKELY(obj->watched())) {
        WatchpointMap *wpmap = cx->compartment()->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }
    if (obj->is<ProxyObject>())
        return Proxy::set(cx, obj, receiver, id, strict, vp);
    return obj->getOps()->setProperty(cx, obj, id, vp, strict);
}

 bool
JSObject::nonNativeSetElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                              uint32_t index, MutableHandleValue vp, bool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return nonNativeSetProperty(cx, obj, receiver, id, vp, strict);
}

JS_FRIEND_API(bool)
JS_CopyPropertyFrom(JSContext *cx, HandleId id, HandleObject target,
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

    bool ignored;
    return StandardDefineProperty(cx, target, wrappedId, desc, &ignored);
}

JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext *cx, HandleObject target, HandleObject obj)
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
CopyProxyObject(JSContext *cx, Handle<ProxyObject *> from, Handle<ProxyObject *> to)
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

JSObject *
js::CloneObject(JSContext *cx, HandleObject obj, Handle<js::TaggedProto> proto, HandleObject parent)
{
    if (!obj->isNative() && !obj->is<ProxyObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_CANT_CLONE_OBJECT);
        return nullptr;
    }

    RootedObject clone(cx);
    if (obj->isNative()) {
        clone = NewObjectWithGivenProto(cx, obj->getClass(), proto, parent);
        if (!clone)
            return nullptr;

        if (clone->is<JSFunction>() && (obj->compartment() != clone->compartment())) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_CANT_CLONE_OBJECT);
            return nullptr;
        }

        if (obj->as<NativeObject>().hasPrivate())
            clone->as<NativeObject>().setPrivate(obj->as<NativeObject>().getPrivate());
    } else {
        ProxyOptions options;
        options.setClass(obj->getClass());

        clone = ProxyObject::New(cx, GetProxyHandler(obj), JS::NullHandleValue, proto, parent, options);
        if (!clone)
            return nullptr;

        if (!CopyProxyObject(cx, obj.as<ProxyObject>(), clone.as<ProxyObject>()))
            return nullptr;
    }

    return clone;
}

NativeObject *
js::DeepCloneObjectLiteral(JSContext *cx, HandleNativeObject obj, NewObjectKind newKind)
{
    
    MOZ_ASSERT_IF(obj->isSingleton(),
                  JS::CompartmentOptionsRef(cx).getSingletonsAsTemplates());
    MOZ_ASSERT(obj->is<PlainObject>() || obj->is<ArrayObject>());

    
    RootedNativeObject clone(cx);

    
    RootedValue v(cx);
    RootedNativeObject deepObj(cx);

    if (obj->is<ArrayObject>()) {
        clone = NewDenseUnallocatedArray(cx, obj->as<ArrayObject>().length(), nullptr, newKind);
    } else {
        
        MOZ_ASSERT(obj->isTenured());
        AllocKind kind = obj->asTenured().getAllocKind();
        RootedObjectGroup group(cx, obj->getGroup(cx));
        if (!group)
            return nullptr;
        RootedObject parent(cx, obj->getParent());
        clone = NewNativeObjectWithGivenProto(cx, &PlainObject::class_,
                                              TaggedProto(group->proto().toObject()),
                                              parent, kind, newKind);
    }

    
    if (!clone || !clone->ensureElements(cx, obj->getDenseCapacity()))
        return nullptr;

    
    uint32_t initialized = obj->getDenseInitializedLength();
    for (uint32_t i = 0; i < initialized; ++i) {
        v = obj->getDenseElement(i);
        if (v.isObject()) {
            deepObj = &v.toObject().as<NativeObject>();
            deepObj = js::DeepCloneObjectLiteral(cx, deepObj, newKind);
            if (!deepObj) {
                JS_ReportOutOfMemory(cx);
                return nullptr;
            }
            v.setObject(*deepObj);
        }
        clone->setDenseInitializedLength(i + 1);
        clone->initDenseElement(i, v);
    }

    MOZ_ASSERT(obj->compartment() == clone->compartment());
    MOZ_ASSERT(!obj->hasPrivate());
    RootedShape shape(cx, obj->lastProperty());
    size_t span = shape->slotSpan();
    clone->setLastProperty(cx, clone, shape);
    for (size_t i = 0; i < span; i++) {
        v = obj->getSlot(i);
        if (v.isObject()) {
            deepObj = &v.toObject().as<NativeObject>();
            deepObj = js::DeepCloneObjectLiteral(cx, deepObj, newKind);
            if (!deepObj)
                return nullptr;
            v.setObject(*deepObj);
        }
        clone->setSlot(i, v);
    }

    if (obj->isSingleton()) {
        if (!JSObject::setSingleton(cx, clone))
            return nullptr;
    } else if (obj->is<ArrayObject>()) {
        ObjectGroup::fixArrayGroup(cx, &clone->as<ArrayObject>());
    } else {
        ObjectGroup::fixPlainObjectGroup(cx, &clone->as<PlainObject>());
    }

    if (obj->is<ArrayObject>() && obj->denseElementsAreCopyOnWrite()) {
        if (!ObjectElements::MakeElementsCopyOnWrite(cx, clone))
            return nullptr;
    }

    return clone;
}

template<XDRMode mode>
bool
js::XDRObjectLiteral(XDRState<mode> *xdr, MutableHandleNativeObject obj)
{
    

    JSContext *cx = xdr->cx();
    MOZ_ASSERT_IF(mode == XDR_ENCODE && obj->isSingleton(),
                  JS::CompartmentOptionsRef(cx).getSingletonsAsTemplates());

    
    uint32_t isArray = 0;
    {
        if (mode == XDR_ENCODE) {
            MOZ_ASSERT(obj->is<PlainObject>() || obj->is<ArrayObject>());
            isArray = obj->getClass() == &ArrayObject::class_ ? 1 : 0;
        }

        if (!xdr->codeUint32(&isArray))
            return false;
    }

    if (isArray) {
        uint32_t length;

        if (mode == XDR_ENCODE)
            length = obj->as<ArrayObject>().length();

        if (!xdr->codeUint32(&length))
            return false;

        if (mode == XDR_DECODE)
            obj.set(NewDenseUnallocatedArray(cx, length, NULL, js::MaybeSingletonObject));

    } else {
        
        AllocKind kind;
        {
            if (mode == XDR_ENCODE) {
                MOZ_ASSERT(obj->is<PlainObject>());
                MOZ_ASSERT(obj->isTenured());
                kind = obj->asTenured().getAllocKind();
            }

            if (!xdr->codeEnum32(&kind))
                return false;

            if (mode == XDR_DECODE) {
                obj.set(NewBuiltinClassInstance<PlainObject>(cx, kind, MaybeSingletonObject));
                if (!obj)
                    return false;
            }
        }
    }

    {
        uint32_t capacity;
        if (mode == XDR_ENCODE)
            capacity = obj->getDenseCapacity();
        if (!xdr->codeUint32(&capacity))
            return false;
        if (mode == XDR_DECODE) {
            if (!obj->ensureElements(cx, capacity)) {
                JS_ReportOutOfMemory(cx);
                return false;
            }
        }
    }

    uint32_t initialized;
    {
        if (mode == XDR_ENCODE)
            initialized = obj->getDenseInitializedLength();
        if (!xdr->codeUint32(&initialized))
            return false;
        if (mode == XDR_DECODE) {
            if (initialized)
                obj->setDenseInitializedLength(initialized);
        }
    }

    RootedValue tmpValue(cx);

    
    {
        for (unsigned i = 0; i < initialized; i++) {
            if (mode == XDR_ENCODE)
                tmpValue = obj->getDenseElement(i);

            if (!xdr->codeConstValue(&tmpValue))
                return false;

            if (mode == XDR_DECODE)
                obj->initDenseElement(i, tmpValue);
        }
    }

    MOZ_ASSERT(!obj->hasPrivate());
    RootedShape shape(cx, obj->lastProperty());

    
    unsigned nslot = 0;

    
    
    
    
    js::AutoIdVector ids(cx);
    {
        if (mode == XDR_ENCODE && !shape->isEmptyShape()) {
            nslot = shape->slotSpan();
            if (!ids.reserve(nslot))
                return false;

            for (unsigned i = 0; i < nslot; i++)
                ids.infallibleAppend(JSID_VOID);

            for (Shape::Range<NoGC> it(shape); !it.empty(); it.popFront()) {
                
                
                if (!it.front().hasSlot()) {
                    MOZ_ASSERT(isArray);
                    break;
                }

                MOZ_ASSERT(it.front().hasDefaultGetter());
                ids[it.front().slot()].set(it.front().propid());
            }
        }

        if (!xdr->codeUint32(&nslot))
            return false;

        RootedAtom atom(cx);
        RootedId id(cx);
        uint32_t idType = 0;
        for (unsigned i = 0; i < nslot; i++) {
            if (mode == XDR_ENCODE) {
                id = ids[i];
                if (JSID_IS_INT(id))
                    idType = JSID_TYPE_INT;
                else if (JSID_IS_ATOM(id))
                    idType = JSID_TYPE_STRING;
                else
                    MOZ_CRASH("Symbol property is not yet supported by XDR.");

                tmpValue = obj->getSlot(i);
            }

            if (!xdr->codeUint32(&idType))
                return false;

            if (idType == JSID_TYPE_STRING) {
                if (mode == XDR_ENCODE)
                    atom = JSID_TO_ATOM(id);
                if (!XDRAtom(xdr, &atom))
                    return false;
                if (mode == XDR_DECODE)
                    id = AtomToId(atom);
            } else {
                MOZ_ASSERT(idType == JSID_TYPE_INT);
                uint32_t indexVal;
                if (mode == XDR_ENCODE)
                    indexVal = uint32_t(JSID_TO_INT(id));
                if (!xdr->codeUint32(&indexVal))
                    return false;
                if (mode == XDR_DECODE)
                    id = INT_TO_JSID(int32_t(indexVal));
            }

            if (!xdr->codeConstValue(&tmpValue))
                return false;

            if (mode == XDR_DECODE) {
                if (!NativeDefineProperty(cx, obj, id, tmpValue, nullptr, nullptr,
                                          JSPROP_ENUMERATE))
                {
                    return false;
                }
            }
        }

        MOZ_ASSERT_IF(mode == XDR_DECODE, !obj->inDictionaryMode());
    }

    
    uint32_t isSingletonTyped;
    if (mode == XDR_ENCODE)
        isSingletonTyped = obj->isSingleton() ? 1 : 0;
    if (!xdr->codeUint32(&isSingletonTyped))
        return false;

    if (mode == XDR_DECODE) {
        if (isSingletonTyped) {
            if (!JSObject::setSingleton(cx, obj))
                return false;
        } else if (isArray) {
            ObjectGroup::fixArrayGroup(cx, &obj->as<ArrayObject>());
        } else {
            ObjectGroup::fixPlainObjectGroup(cx, &obj->as<PlainObject>());
        }
    }

    {
        uint32_t frozen;
        bool extensible;
        if (mode == XDR_ENCODE) {
            if (!IsExtensible(cx, obj, &extensible))
                return false;
            frozen = extensible ? 0 : 1;
        }
        if (!xdr->codeUint32(&frozen))
            return false;
        if (mode == XDR_DECODE && frozen == 1) {
            if (!FreezeObject(cx, obj))
                return false;
        }
    }

    if (isArray) {
        uint32_t copyOnWrite;
        if (mode == XDR_ENCODE)
            copyOnWrite = obj->denseElementsAreCopyOnWrite();
        if (!xdr->codeUint32(&copyOnWrite))
            return false;
        if (mode == XDR_DECODE && copyOnWrite) {
            if (!ObjectElements::MakeElementsCopyOnWrite(cx, obj))
                return false;
        }
    }

    return true;
}

template bool
js::XDRObjectLiteral(XDRState<XDR_ENCODE> *xdr, MutableHandleNativeObject obj);

template bool
js::XDRObjectLiteral(XDRState<XDR_DECODE> *xdr, MutableHandleNativeObject obj);

JSObject *
js::CloneObjectLiteral(JSContext *cx, HandleObject parent, HandleObject srcObj)
{
    if (srcObj->is<PlainObject>()) {
        AllocKind kind = GetBackgroundAllocKind(GuessObjectGCKind(srcObj->as<PlainObject>().numFixedSlots()));
        MOZ_ASSERT_IF(srcObj->isTenured(), kind == srcObj->asTenured().getAllocKind());

        JSObject *proto = cx->global()->getOrCreateObjectPrototype(cx);
        if (!proto)
            return nullptr;
        RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, &PlainObject::class_,
                                                                 TaggedProto(proto)));
        if (!group)
            return nullptr;

        RootedShape shape(cx, srcObj->lastProperty());
        return NewReshapedObject(cx, group, parent, kind, shape);
    }

    RootedArrayObject srcArray(cx, &srcObj->as<ArrayObject>());
    MOZ_ASSERT(srcArray->denseElementsAreCopyOnWrite());
    MOZ_ASSERT(srcArray->getElementsHeader()->ownerObject() == srcObj);

    size_t length = srcArray->as<ArrayObject>().length();
    RootedArrayObject res(cx, NewDenseFullyAllocatedArray(cx, length, nullptr, MaybeSingletonObject));
    if (!res)
        return nullptr;

    RootedId id(cx);
    RootedValue value(cx);
    for (size_t i = 0; i < length; i++) {
        
        
        value = srcArray->getDenseElement(i);
        MOZ_ASSERT_IF(value.isMarkable(),
                      value.toGCThing()->isTenured() &&
                      cx->runtime()->isAtomsZone(value.toGCThing()->asTenured().zone()));

        id = INT_TO_JSID(i);
        if (!DefineProperty(cx, res, id, value, nullptr, nullptr, JSPROP_ENUMERATE))
            return nullptr;
    }

    if (!ObjectElements::MakeElementsCopyOnWrite(cx, res))
        return nullptr;

    return res;
}

void
NativeObject::fillInAfterSwap(JSContext *cx, const Vector<Value> &values, void *priv)
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
        shape_->listp = &shape_;
}


bool
JSObject::swap(JSContext *cx, HandleObject a, HandleObject b)
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

        
        
        
        NativeObject *na = a->isNative() ? &a->as<NativeObject>() : nullptr;
        NativeObject *nb = b->isNative() ? &b->as<NativeObject>() : nullptr;

        
        Vector<Value> avals(cx);
        void *apriv = nullptr;
        if (na) {
            apriv = na->hasPrivate() ? na->getPrivate() : nullptr;
            for (size_t i = 0; i < na->slotSpan(); i++) {
                if (!avals.append(na->getSlot(i)))
                    CrashAtUnhandlableOOM("JSObject::swap");
            }
        }
        Vector<Value> bvals(cx);
        void *bpriv = nullptr;
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

    







    JS::Zone *zone = a->zone();
    if (zone->needsIncrementalBarrier()) {
        MarkChildren(zone->barrierTracer(), a);
        MarkChildren(zone->barrierTracer(), b);
    }

    NotifyGCPostSwap(a, b, r);
    return true;
}

static bool
DefineStandardSlot(JSContext *cx, HandleObject obj, JSProtoKey key, JSAtom *atom,
                   HandleValue v, uint32_t attrs, bool &named)
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
SetClassObject(JSObject *obj, JSProtoKey key, JSObject *cobj, JSObject *proto)
{
    MOZ_ASSERT(!obj->getParent());
    if (!obj->is<GlobalObject>())
        return;

    obj->as<GlobalObject>().setConstructor(key, ObjectOrNullValue(cobj));
    obj->as<GlobalObject>().setPrototype(key, ObjectOrNullValue(proto));
}

static void
ClearClassObject(JSObject *obj, JSProtoKey key)
{
    MOZ_ASSERT(!obj->getParent());
    if (!obj->is<GlobalObject>())
        return;

    obj->as<GlobalObject>().setConstructor(key, UndefinedValue());
    obj->as<GlobalObject>().setPrototype(key, UndefinedValue());
}

static NativeObject *
DefineConstructorAndPrototype(JSContext *cx, HandleObject obj, JSProtoKey key, HandleAtom atom,
                              JSObject *protoProto, const Class *clasp,
                              Native constructor, unsigned nargs,
                              const JSPropertySpec *ps, const JSFunctionSpec *fs,
                              const JSPropertySpec *static_ps, const JSFunctionSpec *static_fs,
                              NativeObject **ctorp, AllocKind ctorKind)
{
    





















    






    RootedNativeObject proto(cx, NewNativeObjectWithClassProto(cx, clasp, protoProto, obj, SingletonObject));
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
        





        RootedFunction fun(cx, NewFunction(cx, js::NullPtr(), constructor, nargs,
                                           JSFunction::NATIVE_CTOR, obj, atom, ctorKind));
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
        bool succeeded;
        RootedId id(cx, AtomToId(atom));
        DeleteProperty(cx, obj, id, &succeeded);
    }
    if (cached)
        ClearClassObject(obj, key);
    return nullptr;
}

NativeObject *
js_InitClass(JSContext *cx, HandleObject obj, JSObject *protoProto_,
             const Class *clasp, Native constructor, unsigned nargs,
             const JSPropertySpec *ps, const JSFunctionSpec *fs,
             const JSPropertySpec *static_ps, const JSFunctionSpec *static_fs,
             NativeObject **ctorp, AllocKind ctorKind)
{
    RootedObject protoProto(cx, protoProto_);

    
    MOZ_ASSERT(clasp->addProperty != JS_PropertyStub);
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
    




    if (is<NativeObject>() && as<NativeObject>().hasDynamicElements()) {
        ObjectElements *header = as<NativeObject>().getElementsHeader();
        if (header->isCopyOnWrite()) {
            NativeObject *owner = MaybeForwarded(header->ownerObject().get());
            as<NativeObject>().elements_ = owner->getElementsHeader()->elements();
        }
    }
}

bool
js::SetClassAndProto(JSContext *cx, HandleObject obj,
                     const Class *clasp, Handle<js::TaggedProto> proto)
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

    ObjectGroup *group = ObjectGroup::defaultNewGroup(cx, clasp, proto);
    if (!group)
        return false;

    






    MarkObjectGroupUnknownProperties(cx, obj->group());
    MarkObjectGroupUnknownProperties(cx, group);

    obj->setGroup(group);

    return true;
}

static bool
MaybeResolveConstructor(ExclusiveContext *cxArg, Handle<GlobalObject*> global, JSProtoKey key)
{
    if (global->isStandardClassResolved(key))
        return true;
    if (!cxArg->shouldBeJSContext())
        return false;

    JSContext *cx = cxArg->asJSContext();
    return GlobalObject::resolveConstructor(cx, global, key);
}

bool
js::GetBuiltinConstructor(ExclusiveContext *cx, JSProtoKey key, MutableHandleObject objp)
{
    MOZ_ASSERT(key != JSProto_Null);
    Rooted<GlobalObject*> global(cx, cx->global());
    if (!MaybeResolveConstructor(cx, global, key))
        return false;

    objp.set(&global->getConstructor(key).toObject());
    return true;
}

bool
js::GetBuiltinPrototype(ExclusiveContext *cx, JSProtoKey key, MutableHandleObject protop)
{
    MOZ_ASSERT(key != JSProto_Null);
    Rooted<GlobalObject*> global(cx, cx->global());
    if (!MaybeResolveConstructor(cx, global, key))
        return false;

    protop.set(&global->getPrototype(key).toObject());
    return true;
}

static bool
IsStandardPrototype(JSObject *obj, JSProtoKey key)
{
    GlobalObject &global = obj->global();
    Value v = global.getPrototype(key);
    return v.isObject() && obj == &v.toObject();
}

JSProtoKey
JS::IdentifyStandardInstance(JSObject *obj)
{
    
    MOZ_ASSERT(!obj->is<CrossCompartmentWrapperObject>());
    JSProtoKey key = StandardProtoKeyOrNull(obj);
    if (key != JSProto_Null && !IsStandardPrototype(obj, key))
        return key;
    return JSProto_Null;
}

JSProtoKey
JS::IdentifyStandardPrototype(JSObject *obj)
{
    
    MOZ_ASSERT(!obj->is<CrossCompartmentWrapperObject>());
    JSProtoKey key = StandardProtoKeyOrNull(obj);
    if (key != JSProto_Null && IsStandardPrototype(obj, key))
        return key;
    return JSProto_Null;
}

JSProtoKey
JS::IdentifyStandardInstanceOrPrototype(JSObject *obj)
{
    return StandardProtoKeyOrNull(obj);
}

JSProtoKey
JS::IdentifyStandardConstructor(JSObject *obj)
{
    
    
    
    
    if (!obj->is<JSFunction>() || !(obj->as<JSFunction>().flags() & JSFunction::NATIVE_CTOR))
        return JSProto_Null;

    GlobalObject &global = obj->global();
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
        const JSFunction &fun = as<JSFunction>();
        return fun.isNativeConstructor() || fun.isInterpretedConstructor();
    }
    return constructHook() != nullptr;
}

JSNative
JSObject::callHook() const
{
    const js::Class *clasp = getClass();

    if (clasp->call)
        return clasp->call;

    if (is<js::ProxyObject>()) {
        const js::ProxyObject &p = as<js::ProxyObject>();
        if (p.handler()->isCallable(const_cast<JSObject*>(this)))
            return js::proxy_Call;
    }
    return nullptr;
}

JSNative
JSObject::constructHook() const
{
    const js::Class *clasp = getClass();

    if (clasp->construct)
        return clasp->construct;

    if (is<js::ProxyObject>()) {
        const js::ProxyObject &p = as<js::ProxyObject>();
        if (p.handler()->isConstructor(const_cast<JSObject*>(this)))
            return js::proxy_Construct;
    }
    return nullptr;
}

bool
js::LookupProperty(JSContext *cx, HandleObject obj, js::HandleId id,
                   MutableHandleObject objp, MutableHandleShape propp)
{
    



    if (LookupPropertyOp op = obj->getOps()->lookupProperty)
        return op(cx, obj, id, objp, propp);
    return NativeLookupProperty<CanGC>(cx, obj.as<NativeObject>(), id, objp, propp);
}

bool
js::LookupName(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
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
js::LookupNameNoGC(JSContext *cx, PropertyName *name, JSObject *scopeChain,
                   JSObject **objp, JSObject **pobjp, Shape **propp)
{
    AutoAssertNoException nogc(cx);

    MOZ_ASSERT(!*objp && !*pobjp && !*propp);

    for (JSObject *scope = scopeChain; scope; scope = scope->enclosingScope()) {
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
js::LookupNameWithGlobalDefault(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
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
js::LookupNameUnqualified(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
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
js::HasOwnProperty(JSContext *cx, HandleObject obj, HandleId id, bool *result)
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
js::LookupPropertyPure(ExclusiveContext *cx, JSObject *obj, jsid id, JSObject **objp,
                       Shape **propp)
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

            if (Shape *shape = obj->as<NativeObject>().lookupPure(id)) {
                *objp = obj;
                *propp = shape;
                return true;
            }

            
            
            
            do {
                const Class *clasp = obj->getClass();
                if (!clasp->resolve)
                break;
                if (clasp->resolve == fun_resolve && !FunctionHasResolveHook(cx->names(), id))
                    break;
                if (clasp->resolve == str_resolve && !JSID_IS_INT(id))
                    break;
                return false;
            } while (0);
        } else {
            
            
            if (!obj->is<UnboxedPlainObject>())
                return false;
            if (obj->as<UnboxedPlainObject>().layout().lookup(id)) {
                *objp = obj;
                MarkNonNativePropertyFound<NoGC>(propp);
                return true;
            }
        }

        obj = obj->getProto();
    } while (obj);

    *objp = nullptr;
    *propp = nullptr;
    return true;
}

bool
JSObject::reportReadOnly(JSContext *cx, jsid id, unsigned report)
{
    RootedValue val(cx, IdToValue(id));
    return js_ReportValueErrorFlags(cx, report, JSMSG_READ_ONLY,
                                    JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                    nullptr, nullptr);
}

bool
JSObject::reportNotConfigurable(JSContext *cx, jsid id, unsigned report)
{
    RootedValue val(cx, IdToValue(id));
    return js_ReportValueErrorFlags(cx, report, JSMSG_CANT_DELETE,
                                    JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                    nullptr, nullptr);
}

bool
JSObject::reportNotExtensible(JSContext *cx, unsigned report)
{
    RootedValue val(cx, ObjectValue(*this));
    return js_ReportValueErrorFlags(cx, report, JSMSG_OBJECT_NOT_EXTENSIBLE,
                                    JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                    nullptr, nullptr);
}




bool
js::SetPrototype(JSContext *cx, HandleObject obj, HandleObject proto, bool *succeeded)
{
    





    if (obj->hasLazyPrototype()) {
        MOZ_ASSERT(obj->is<ProxyObject>());
        return Proxy::setPrototypeOf(cx, obj, proto, succeeded);
    }

    
    if (obj->nonLazyPrototypeIsImmutable()) {
        *succeeded = false;
        return true;
    }

    




    if (obj->is<ArrayBufferObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_SETPROTOTYPEOF_FAIL,
                             "incompatible ArrayBuffer");
        return false;
    }

    


    if (obj->is<TypedObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_SETPROTOTYPEOF_FAIL,
                             "incompatible TypedObject");
        return false;
    }

    



    if (!strcmp(obj->getClass()->name, "Location")) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_SETPROTOTYPEOF_FAIL,
                             "incompatible Location object");
        return false;
    }

    
    bool extensible;
    if (!IsExtensible(cx, obj, &extensible))
        return false;
    if (!extensible) {
        *succeeded = false;
        return true;
    }

    
    RootedObject obj2(cx);
    for (obj2 = proto; obj2; ) {
        if (obj2 == obj) {
            *succeeded = false;
            return true;
        }

        if (!GetPrototype(cx, obj2, &obj2))
            return false;
    }

    Rooted<TaggedProto> taggedProto(cx, TaggedProto(proto));
    *succeeded = SetClassAndProto(cx, obj, obj->getClass(), taggedProto);
    return *succeeded;
}

bool
js::PreventExtensions(JSContext *cx, HandleObject obj, bool *succeeded)
{
    if (obj->is<ProxyObject>())
        return js::Proxy::preventExtensions(cx, obj, succeeded);

    if (!obj->nonProxyIsExtensible()) {
        *succeeded = true;
        return true;
    }

    



    AutoIdVector props(cx);
    if (!js::GetPropertyKeys(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    





    if (obj->isNative() && !NativeObject::sparsifyDenseElements(cx, obj.as<NativeObject>()))
        return false;

    *succeeded = true;
    return obj->setFlag(cx, BaseShape::NOT_EXTENSIBLE, JSObject::GENERATE_SHAPE);
}

bool
js::GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                             MutableHandle<PropertyDescriptor> desc)
{
    if (GetOwnPropertyOp op = obj->getOps()->getOwnPropertyDescriptor)
        return op(cx, obj, id, desc);

    RootedShape shape(cx);
    if (!NativeLookupOwnProperty<CanGC>(cx, obj.as<NativeObject>(), id, &shape))
        return false;
    if (!shape) {
        desc.object().set(nullptr);
        return true;
    }

    bool doGet = true;
    desc.setAttributes(GetShapeAttributes(obj, shape));
    if (desc.hasGetterOrSetterObject()) {
        MOZ_ASSERT(desc.isShared());
        doGet = false;
        if (desc.hasGetterObject())
            desc.setGetterObject(shape->getterObject());
        if (desc.hasSetterObject())
            desc.setSetterObject(shape->setterObject());
    } else {
        
        
        
        
        desc.attributesRef() &= ~JSPROP_SHARED;
    }

    RootedValue value(cx);
    if (doGet && !GetProperty(cx, obj, obj, id, &value))
        return false;

    desc.value().set(value);
    desc.object().set(obj);
    return true;
}

bool
js::DefineProperty(ExclusiveContext *cx, HandleObject obj, HandleId id, HandleValue value,
                   JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);
    MOZ_ASSERT(!(attrs & JSPROP_PROPOP_ACCESSORS));

    DefinePropertyOp op = obj->getOps()->defineProperty;
    if (op) {
        if (!cx->shouldBeJSContext())
            return false;
        return op(cx->asJSContext(), obj, id, value, getter, setter, attrs);
    }
    return NativeDefineProperty(cx, obj.as<NativeObject>(), id, value, getter, setter, attrs);
}

bool
js::DefineProperty(ExclusiveContext *cx, HandleObject obj,
                 PropertyName *name, HandleValue value,
                 JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    RootedId id(cx, NameToId(name));
    return DefineProperty(cx, obj, id, value, getter, setter, attrs);
}

bool
js::DefineElement(ExclusiveContext *cx, HandleObject obj, uint32_t index, HandleValue value,
                  JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    MOZ_ASSERT(getter != JS_PropertyStub);
    MOZ_ASSERT(setter != JS_StrictPropertyStub);

    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return DefineProperty(cx, obj, id, value, getter, setter, attrs);
}




bool
js::SetImmutablePrototype(ExclusiveContext *cx, HandleObject obj, bool *succeeded)
{
    if (obj->hasLazyPrototype()) {
        if (!cx->shouldBeJSContext())
            return false;
        return Proxy::setImmutablePrototype(cx->asJSContext(), obj, succeeded);
    }

    if (!obj->setFlag(cx, BaseShape::IMMUTABLE_PROTOTYPE))
        return false;
    *succeeded = true;
    return true;
}

bool
js::GetPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                          MutableHandle<PropertyDescriptor> desc)
{
    RootedObject pobj(cx);

    for (pobj = obj; pobj;) {
        if (pobj->is<ProxyObject>())
            return Proxy::getPropertyDescriptor(cx, pobj, id, desc);

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
js::ToPrimitive(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp)
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
js::WatchGuts(JSContext *cx, JS::HandleObject origObj, JS::HandleId id, JS::HandleObject callable)
{
    RootedObject obj(cx, GetInnerObject(origObj));
    if (obj->isNative()) {
        
        
        if (!NativeObject::sparsifyDenseElements(cx, obj.as<NativeObject>()))
            return false;

        types::MarkTypePropertyNonData(cx, obj, id);
    }

    WatchpointMap *wpmap = cx->compartment()->watchpointMap;
    if (!wpmap) {
        wpmap = cx->runtime()->new_<WatchpointMap>();
        if (!wpmap || !wpmap->init()) {
            js_ReportOutOfMemory(cx);
            return false;
        }
        cx->compartment()->watchpointMap = wpmap;
    }

    return wpmap->watch(cx, obj, id, js::WatchHandler, callable);
}

bool
js::UnwatchGuts(JSContext *cx, JS::HandleObject origObj, JS::HandleId id)
{
    
    
    RootedObject obj(cx, GetInnerObject(origObj));
    if (WatchpointMap *wpmap = cx->compartment()->watchpointMap)
        wpmap->unwatch(obj, id, nullptr, nullptr);
    return true;
}

bool
js::WatchProperty(JSContext *cx, HandleObject obj, HandleId id, HandleObject callable)
{
    if (WatchOp op = obj->getOps()->watch)
        return op(cx, obj, id, callable);

    if (!obj->isNative() || IsAnyTypedArray(obj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_WATCH,
                             obj->getClass()->name);
        return false;
    }

    return WatchGuts(cx, obj, id, callable);
}

bool
js::UnwatchProperty(JSContext *cx, HandleObject obj, HandleId id)
{
    if (UnwatchOp op = obj->getOps()->unwatch)
        return op(cx, obj, id);

    return UnwatchGuts(cx, obj, id);
}

const char *
js::GetObjectClassName(JSContext *cx, HandleObject obj)
{
    assertSameCompartment(cx, obj);

    if (obj->is<ProxyObject>())
        return Proxy::className(cx, obj);

    return obj->getClass()->name;
}

bool
JSObject::callMethod(JSContext *cx, HandleId id, unsigned argc, Value *argv, MutableHandleValue vp)
{
    RootedValue fval(cx);
    RootedObject obj(cx, this);
    if (!GetProperty(cx, obj, obj, id, &fval))
        return false;
    return Invoke(cx, ObjectValue(*obj), fval, argc, argv, vp);
}




bool
js::HasDataProperty(JSContext *cx, NativeObject *obj, jsid id, Value *vp)
{
    if (JSID_IS_INT(id) && obj->containsDenseElement(JSID_TO_INT(id))) {
        *vp = obj->getDenseElement(JSID_TO_INT(id));
        return true;
    }

    if (Shape *shape = obj->lookup(cx, id)) {
        if (shape->hasDefaultGetter() && shape->hasSlot()) {
            *vp = obj->getSlot(shape->slot());
            return true;
        }
    }

    return false;
}









static bool
MaybeCallMethod(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
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
JS::OrdinaryToPrimitive(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp)
{
    MOZ_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);

    Rooted<jsid> id(cx);

    const Class *clasp = obj->getClass();
    if (hint == JSTYPE_STRING) {
        id = NameToId(cx->names().toString);

        
        if (clasp == &StringObject::class_) {
            StringObject *nobj = &obj->as<StringObject>();
            if (ClassMethodIsNative(cx, nobj, &StringObject::class_, id, js_str_toString)) {
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
            StringObject *nobj = &obj->as<StringObject>();
            if (ClassMethodIsNative(cx, nobj, &StringObject::class_, id, js_str_toString)) {
                vp.setString(nobj->unbox());
                return true;
            }
        }

        
        if (clasp == &NumberObject::class_) {
            id = NameToId(cx->names().valueOf);
            NumberObject *nobj = &obj->as<NumberObject>();
            if (ClassMethodIsNative(cx, nobj, &NumberObject::class_, id, js_num_valueOf)) {
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
    js_ReportValueError2(cx, JSMSG_CANT_CONVERT_TO, JSDVG_SEARCH_STACK, val, str,
                         hint == JSTYPE_VOID
                         ? "primitive type"
                         : hint == JSTYPE_STRING ? "string" : "number");
    return false;
}

bool
js::IsDelegate(JSContext *cx, HandleObject obj, const js::Value &v, bool *result)
{
    if (v.isPrimitive()) {
        *result = false;
        return true;
    }
    return IsDelegateOfObject(cx, obj, &v.toObject(), result);
}

bool
js::IsDelegateOfObject(JSContext *cx, HandleObject protoObj, JSObject* obj, bool *result)
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

JSObject *
js::GetBuiltinPrototypePure(GlobalObject *global, JSProtoKey protoKey)
{
    MOZ_ASSERT(JSProto_Null <= protoKey);
    MOZ_ASSERT(protoKey < JSProto_LIMIT);

    if (protoKey != JSProto_Null) {
        const Value &v = global->getPrototype(protoKey);
        if (v.isObject())
            return &v.toObject();
    }

    return nullptr;
}

JSObject *
js::PrimitiveToObject(JSContext *cx, const Value &v)
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
    return SymbolObject::create(cx, v.toSymbol());
}








JSObject *
js::ToObjectSlow(JSContext *cx, JS::HandleValue val, bool reportScanStack)
{
    MOZ_ASSERT(!val.isMagic());
    MOZ_ASSERT(!val.isObject());

    if (val.isNullOrUndefined()) {
        if (reportScanStack) {
            js_ReportIsNullOrUndefined(cx, JSDVG_SEARCH_STACK, val, NullPtr());
        } else {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CONVERT_TO,
                                 val.isNull() ? "null" : "undefined", "object");
        }
        return nullptr;
    }

    return PrimitiveToObject(cx, val);
}

void
js_GetObjectSlotName(JSTracer *trc, char *buf, size_t bufsize)
{
    MOZ_ASSERT(trc->debugPrinter() == js_GetObjectSlotName);

    JSObject *obj = (JSObject *)trc->debugPrintArg();
    uint32_t slot = uint32_t(trc->debugPrintIndex());

    Shape *shape;
    if (obj->isNative()) {
        shape = obj->lastProperty();
        while (shape && (!shape->hasSlot() || shape->slot() != slot))
            shape = shape->previous();
    } else {
        shape = nullptr;
    }

    if (!shape) {
        do {
            const char *slotname = nullptr;
            const char *pattern = nullptr;
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
js_ReportGetterOnlyAssignment(JSContext *cx, bool strict)
{
    return JS_ReportErrorFlagsAndNumber(cx,
                                        strict
                                        ? JSREPORT_ERROR
                                        : JSREPORT_WARNING | JSREPORT_STRICT,
                                        js_GetErrorMessage, nullptr,
                                        JSMSG_GETTER_ONLY);
}




#ifdef DEBUG







static void
dumpValue(const Value &v)
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
        JSFunction *fun = &v.toObject().as<JSFunction>();
        if (fun->displayAtom()) {
            fputs("<function ", stderr);
            FileEscapedString(stderr, fun->displayAtom(), 0);
        } else {
            fputs("<unnamed function", stderr);
        }
        if (fun->hasScript()) {
            JSScript *script = fun->nonLazyScript();
            fprintf(stderr, " (%s:%d)",
                    script->filename() ? script->filename() : "", (int) script->lineno());
        }
        fprintf(stderr, " at %p>", (void *) fun);
    } else if (v.isObject()) {
        JSObject *obj = &v.toObject();
        const Class *clasp = obj->getClass();
        fprintf(stderr, "<%s%s at %p>",
                clasp->name,
                (clasp == &PlainObject::class_) ? "" : " object",
                (void *) obj);
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
js_DumpValue(const Value &val)
{
    dumpValue(val);
    fputc('\n', stderr);
}

JS_FRIEND_API(void)
js_DumpId(jsid id)
{
    fprintf(stderr, "jsid %p = ", (void *) JSID_BITS(id));
    dumpValue(IdToValue(id));
    fputc('\n', stderr);
}

static void
DumpProperty(NativeObject *obj, Shape &shape)
{
    jsid id = shape.propid();
    uint8_t attrs = shape.attributes();

    fprintf(stderr, "    ((js::Shape *) %p) ", (void *) &shape);
    if (attrs & JSPROP_ENUMERATE) fprintf(stderr, "enumerate ");
    if (attrs & JSPROP_READONLY) fprintf(stderr, "readonly ");
    if (attrs & JSPROP_PERMANENT) fprintf(stderr, "permanent ");
    if (attrs & JSPROP_SHARED) fprintf(stderr, "shared ");

    if (shape.hasGetterValue())
        fprintf(stderr, "getterValue=%p ", (void *) shape.getterObject());
    else if (!shape.hasDefaultGetter())
        fprintf(stderr, "getterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.getterOp()));

    if (shape.hasSetterValue())
        fprintf(stderr, "setterValue=%p ", (void *) shape.setterObject());
    else if (!shape.hasDefaultSetter())
        fprintf(stderr, "setterOp=%p ", JS_FUNC_TO_DATA_PTR(void *, shape.setterOp()));

    if (JSID_IS_ATOM(id) || JSID_IS_INT(id) || JSID_IS_SYMBOL(id))
        dumpValue(js::IdToValue(id));
    else
        fprintf(stderr, "unknown jsid %p", (void *) JSID_BITS(id));

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
    JSObject *obj = this;
    fprintf(stderr, "object %p\n", (void *) obj);
    const Class *clasp = obj->getClass();
    fprintf(stderr, "class %p %s\n", (const void *)clasp, clasp->name);

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
        NativeObject *nobj = &obj->as<NativeObject>();
        if (nobj->inDictionaryMode())
            fprintf(stderr, " inDictionaryMode");
        if (nobj->hasShapeTable())
            fprintf(stderr, " hasShapeTable");
    }
    fprintf(stderr, "\n");

    if (obj->isNative()) {
        NativeObject *nobj = &obj->as<NativeObject>();
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

    fprintf(stderr, "parent ");
    dumpValue(ObjectOrNullValue(obj->getParent()));
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
        Vector<Shape *, 8, SystemAllocPolicy> props;
        for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront())
            props.append(&r.front());
        for (size_t i = props.length(); i-- != 0;)
            DumpProperty(&obj->as<NativeObject>(), *props[i]);
    }
    fputc('\n', stderr);
}

static void
MaybeDumpObject(const char *name, JSObject *obj)
{
    if (obj) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(ObjectValue(*obj));
        fputc('\n', stderr);
    }
}

static void
MaybeDumpValue(const char *name, const Value &v)
{
    if (!v.isNull()) {
        fprintf(stderr, "  %s: ", name);
        dumpValue(v);
        fputc('\n', stderr);
    }
}

JS_FRIEND_API(void)
js_DumpInterpreterFrame(JSContext *cx, InterpreterFrame *start)
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
                    (void *)start, (void *)cx);
            return;
        }
    }

    for (; !i.done(); ++i) {
        if (i.isJit())
            fprintf(stderr, "JIT frame\n");
        else
            fprintf(stderr, "InterpreterFrame at %p\n", (void *) i.interpFrame());

        if (i.isFunctionFrame()) {
            fprintf(stderr, "callee fun: ");
            RootedValue v(cx);
            JSObject *fun = i.callee(cx);
            v.setObject(*fun);
            dumpValue(v);
        } else {
            fprintf(stderr, "global frame, no callee");
        }
        fputc('\n', stderr);

        fprintf(stderr, "file %s line %u\n",
                i.script()->filename(), (unsigned) i.script()->lineno());

        if (jsbytecode *pc = i.pc()) {
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

        fprintf(stderr, "  scopeChain: (JSObject *) %p\n", (void *) i.scopeChain(cx));

        fputc('\n', stderr);
    }
}

#endif 

JS_FRIEND_API(void)
js_DumpBacktrace(JSContext *cx)
{
    Sprinter sprinter(cx);
    sprinter.init();
    size_t depth = 0;
    for (AllFramesIter i(cx); !i.done(); ++i, ++depth) {
        const char *filename = JS_GetScriptFilename(i.script());
        unsigned line = PCToLineNumber(i.script(), i.pc());
        JSScript *script = i.script();
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




void
JSObject::addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::ClassInfo *info)
{
    if (is<NativeObject>() && as<NativeObject>().hasDynamicSlots())
        info->objectsMallocHeapSlots += mallocSizeOf(as<NativeObject>().slots_);

    if (is<NativeObject>() && as<NativeObject>().hasDynamicElements()) {
        js::ObjectElements *elements = as<NativeObject>().getElementsHeader();
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
            js::SizeOfDataIfCDataObject(mallocSizeOf, const_cast<JSObject *>(this));
#endif
    }
}

bool
JSObject::hasIdempotentProtoChain() const
{
    
    
    JSObject *obj = const_cast<JSObject *>(this);
    while (true) {
        if (!obj->isNative())
            return false;

        JSResolveOp resolve = obj->getClass()->resolve;
        if (resolve && resolve != js::fun_resolve && resolve != js::str_resolve)
            return false;

        if (obj->getOps()->lookupProperty)
            return false;

        obj = obj->getProto();
        if (!obj)
            return true;
    }
}

void
JSObject::markChildren(JSTracer *trc)
{
    MarkObjectGroup(trc, &group_, "group");

    MarkShape(trc, &shape_, "shape");

    const Class *clasp = group_->clasp();
    if (clasp->trace)
        clasp->trace(trc, this);

    if (shape_->isNative()) {
        NativeObject *nobj = &as<NativeObject>();
        MarkObjectSlots(trc, nobj, 0, nobj->slotSpan());

        do {
            if (nobj->denseElementsAreCopyOnWrite()) {
                HeapPtrNativeObject &owner = nobj->getElementsHeader()->ownerObject();
                if (owner != nobj) {
                    MarkObject(trc, &owner, "objectElementsOwner");
                    break;
                }
            }

            gc::MarkArraySlots(trc,
                               nobj->getDenseInitializedLength(),
                               nobj->getDenseElementsAllowCopyOnWrite(),
                               "objectElements");
        } while (false);
    }
}
