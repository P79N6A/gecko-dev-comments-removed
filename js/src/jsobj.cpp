









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

#include "builtin/Eval.h"
#include "builtin/Object.h"
#include "frontend/BytecodeCompiler.h"
#include "gc/Marking.h"
#include "jit/AsmJSModule.h"
#include "jit/BaselineJIT.h"
#include "js/MemoryMetrics.h"
#include "js/OldDebugAPI.h"
#include "vm/ArgumentsObject.h"
#include "vm/Interpreter.h"
#include "vm/ProxyObject.h"
#include "vm/RegExpStaticsObject.h"
#include "vm/Shape.h"

#include "jsatominlines.h"
#include "jsboolinlines.h"
#include "jscntxtinlines.h"
#include "jscompartmentinlines.h"

#include "vm/ArrayObject-inl.h"
#include "vm/BooleanObject-inl.h"
#include "vm/NumberObject-inl.h"
#include "vm/ObjectImpl-inl.h"
#include "vm/Runtime-inl.h"
#include "vm/Shape-inl.h"
#include "vm/StringObject-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::Maybe;
using mozilla::RoundUpPow2;

JS_STATIC_ASSERT(int32_t((JSObject::NELEMENTS_LIMIT - 1) * sizeof(Value)) == int64_t((JSObject::NELEMENTS_LIMIT - 1) * sizeof(Value)));

static JSObject *
CreateObjectConstructor(JSContext *cx, JSProtoKey key)
{
    Rooted<GlobalObject*> self(cx, cx->global());
    if (!GlobalObject::ensureConstructor(cx, self, JSProto_Function))
        return nullptr;

    RootedObject functionProto(cx, &self->getPrototype(JSProto_Function).toObject());

    
    RootedObject ctor(cx, NewObjectWithGivenProto(cx, &JSFunction::class_, functionProto,
                                                  self, SingletonObject));
    if (!ctor)
        return nullptr;
    return NewFunction(cx, ctor, obj_construct, 1, JSFunction::NATIVE_CTOR, self,
                       HandlePropertyName(cx->names().Object));
}

static JSObject *
CreateObjectPrototype(JSContext *cx, JSProtoKey key)
{
    Rooted<GlobalObject*> self(cx, cx->global());
    cx->setDefaultCompartmentObjectIfUnset(self);

    JS_ASSERT(!cx->runtime()->isAtomsCompartment(cx->compartment()));
    JS_ASSERT(self->isNative());

    



    RootedObject objectProto(cx, NewObjectWithGivenProto(cx, &JSObject::class_, nullptr,
                                                         self, SingletonObject));
    if (!objectProto)
        return nullptr;

    




    if (!JSObject::setNewTypeUnknown(cx, &JSObject::class_, objectProto))
        return nullptr;

    return objectProto;
}

static bool
ThrowTypeError(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage, nullptr,
                                 JSMSG_THROW_TYPE_ERROR);
    return false;
}

static bool
FinishObjectClassInit(JSContext *cx, JS::HandleObject ctor, JS::HandleObject proto)
{
    Rooted<GlobalObject*> self(cx, cx->global());

    
    RootedId evalId(cx, NameToId(cx->names().eval));
    JSObject *evalobj = DefineFunction(cx, self, evalId, IndirectEval, 1, JSFUN_STUB_GSOPS);
    if (!evalobj)
        return false;
    self->setOriginalEval(evalobj);

    
    RootedFunction throwTypeError(cx, NewFunction(cx, js::NullPtr(), ThrowTypeError, 0,
                                                  JSFunction::NATIVE_FUN, self, js::NullPtr()));
    if (!throwTypeError)
        return false;
    if (!JSObject::preventExtensions(cx, throwTypeError))
        return false;
    self->setThrowTypeError(throwTypeError);

    RootedObject intrinsicsHolder(cx);
    if (cx->runtime()->isSelfHostingGlobal(self)) {
        intrinsicsHolder = self;
    } else {
        intrinsicsHolder = NewObjectWithGivenProto(cx, &JSObject::class_, proto, self,
                                                   TenuredObject);
        if (!intrinsicsHolder)
            return false;
    }
    self->setIntrinsicsHolder(intrinsicsHolder);
    
    RootedValue global(cx, ObjectValue(*self));
    if (!JSObject::defineProperty(cx, intrinsicsHolder, cx->names().global,
                                  global, JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return false;
    }

    







    Rooted<TaggedProto> tagged(cx, TaggedProto(proto));
    if (self->shouldSplicePrototype(cx) && !self->splicePrototype(cx, self->getClass(), tagged))
        return false;
    return true;
}


const Class JSObject::class_ = {
    js_Object_str,
    JSCLASS_HAS_CACHED_PROTO(JSProto_Object),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    {
        CreateObjectConstructor,
        CreateObjectPrototype,
        object_static_methods,
        object_methods,
        object_properties,
        FinishObjectClassInit
    }
};

const Class* const js::ObjectClassPtr = &JSObject::class_;

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
    if (!d.makeObject(cx))
        return false;
    vp.set(d.descriptorValue());
    return true;
}

void
PropDesc::initFromPropertyDescriptor(Handle<PropertyDescriptor> desc)
{
    MOZ_ASSERT(isUndefined());

    if (!desc.object())
        return;

    isUndefined_ = false;
    descObj_ = nullptr;
    attrs = uint8_t(desc.attributes());
    JS_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
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
        hasValue_ = true;
        value_ = desc.value();
        hasWritable_ = true;
    }
    hasEnumerable_ = true;
    hasConfigurable_ = true;
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
    desc.setAttributes(attributes());
    desc.object().set(obj);
}

bool
PropDesc::makeObject(JSContext *cx)
{
    MOZ_ASSERT(!isUndefined());

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &JSObject::class_));
    if (!obj)
        return false;

    const JSAtomState &names = cx->names();
    RootedValue configurableVal(cx, BooleanValue((attrs & JSPROP_PERMANENT) == 0));
    RootedValue enumerableVal(cx, BooleanValue((attrs & JSPROP_ENUMERATE) != 0));
    RootedValue writableVal(cx, BooleanValue((attrs & JSPROP_READONLY) == 0));
    if ((hasConfigurable() &&
         !JSObject::defineProperty(cx, obj, names.configurable, configurableVal)) ||
        (hasEnumerable() &&
         !JSObject::defineProperty(cx, obj, names.enumerable, enumerableVal)) ||
        (hasGet() &&
         !JSObject::defineProperty(cx, obj, names.get, getterValue())) ||
        (hasSet() &&
         !JSObject::defineProperty(cx, obj, names.set, setterValue())) ||
        (hasValue() &&
         !JSObject::defineProperty(cx, obj, names.value, value())) ||
        (hasWritable() &&
         !JSObject::defineProperty(cx, obj, names.writable, writableVal)))
    {
        return false;
    }

    descObj_ = obj;
    return true;
}

bool
js::GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                             MutableHandle<PropertyDescriptor> desc)
{
    if (obj->is<ProxyObject>())
        return Proxy::getOwnPropertyDescriptor(cx, obj, id, desc);

    RootedObject pobj(cx);
    RootedShape shape(cx);
    if (!HasOwnProperty<CanGC>(cx, obj->getOps()->lookupGeneric, obj, id, &pobj, &shape))
        return false;
    if (!shape) {
        desc.object().set(nullptr);
        return true;
    }

    bool doGet = true;
    if (pobj->isNative()) {
        desc.setAttributes(GetShapeAttributes(pobj, shape));
        if (desc.hasGetterOrSetterObject()) {
            doGet = false;
            if (desc.hasGetterObject())
                desc.setGetterObject(shape->getterObject());
            if (desc.hasSetterObject())
                desc.setSetterObject(shape->setterObject());
        }
    } else {
        if (!JSObject::getGenericAttributes(cx, pobj, id, &desc.attributesRef()))
            return false;
    }

    RootedValue value(cx);
    if (doGet && !JSObject::getGeneric(cx, obj, obj, id, &value))
        return false;

    desc.value().set(value);
    desc.object().set(obj);
    return true;
}

bool
js::GetOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    Rooted<PropertyDescriptor> desc(cx);
    return GetOwnPropertyDescriptor(cx, obj, id, &desc) &&
           NewPropertyDescriptorObject(cx, desc, vp);
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
HasProperty(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp, bool *foundp)
{
    if (!JSObject::hasProperty(cx, obj, id, foundp))
        return false;
    if (!*foundp) {
        vp.setUndefined();
        return true;
    }

    





    return !!JSObject::getGeneric(cx, obj, obj, id, vp);
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

    
    descObj_ = desc;

    isUndefined_ = false;

    



    attrs = JSPROP_PERMANENT | JSPROP_READONLY;

    bool found = false;
    RootedId id(cx);

    
    id = NameToId(cx->names().enumerable);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasEnumerable_ = true;
        if (ToBoolean(v))
            attrs |= JSPROP_ENUMERATE;
    }

    
    id = NameToId(cx->names().configurable);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasConfigurable_ = true;
        if (ToBoolean(v))
            attrs &= ~JSPROP_PERMANENT;
    }

    
    id = NameToId(cx->names().value);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasValue_ = true;
        value_ = v;
    }

    
    id = NameToId(cx->names().writable);
    if (!HasProperty(cx, desc, id, &v, &found))
        return false;
    if (found) {
        hasWritable_ = true;
        if (ToBoolean(v))
            attrs &= ~JSPROP_READONLY;
    }

    
    id = NameToId(cx->names().get);
    if (!HasProperty(cx, desc, id, &v, &found))
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
    if (!HasProperty(cx, desc, id, &v, &found))
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

    JS_ASSERT_IF(attrs & JSPROP_READONLY, !(attrs & (JSPROP_GETTER | JSPROP_SETTER)));

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
    JS_ASSERT(js_ErrorFormatString[errorNumber].argCount == 1);

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
        JS_ASSERT(js_ErrorFormatString[errorNumber].argCount == 0);
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





JS_FRIEND_API(bool)
js::CheckDefineProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue value,
                        unsigned attrs, PropertyOp getter, StrictPropertyOp setter)
{
    if (!obj->isNative())
        return true;

    
    
    Rooted<PropertyDescriptor> desc(cx);
    if (!GetOwnPropertyDescriptor(cx, obj, id, &desc))
        return false;

    
    
    
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
DefinePropertyOnObject(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                       bool throwError, bool *rval)
{
    
    RootedShape shape(cx);
    RootedObject obj2(cx);
    JS_ASSERT(!obj->getOps()->lookupGeneric);
    if (!HasOwnProperty<CanGC>(cx, nullptr, obj, id, &obj2, &shape))
        return false;

    JS_ASSERT(!obj->getOps()->defineProperty);

    
    if (!shape) {
        bool extensible;
        if (!JSObject::isExtensible(cx, obj, &extensible))
            return false;
        if (!extensible)
            return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);

        *rval = true;

        if (desc.isGenericDescriptor() || desc.isDataDescriptor()) {
            JS_ASSERT(!obj->getOps()->defineProperty);
            RootedValue v(cx, desc.hasValue() ? desc.value() : UndefinedValue());
            return baseops::DefineGeneric(cx, obj, id, v,
                                          JS_PropertyStub, JS_StrictPropertyStub,
                                          desc.attributes());
        }

        JS_ASSERT(desc.isAccessorDescriptor());

        return baseops::DefineGeneric(cx, obj, id, UndefinedHandleValue,
                                      desc.getter(), desc.setter(), desc.attributes());
    }

    
    RootedValue v(cx);

    JS_ASSERT(obj == obj2);

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

                if (!NativeGet(cx, obj, obj2, shape, &v))
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
                
                JS_ASSERT(desc.isGenericDescriptor());
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
        
        JS_ASSERT(shapeDataDescriptor);
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
        
        JS_ASSERT(desc.isAccessorDescriptor() && shape->isAccessorDescriptor());
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
        getter = IsImplicitDenseOrTypedArrayElement(shape) ? JS_PropertyStub : shape->getter();
        setter = IsImplicitDenseOrTypedArrayElement(shape) ? JS_StrictPropertyStub : shape->setter();
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
        getter = JS_PropertyStub;
        setter = JS_StrictPropertyStub;
    } else {
        JS_ASSERT(desc.isAccessorDescriptor());

        
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
                     ? JS_PropertyStub
                     : shape->getter();
        }
        if (desc.hasSet()) {
            setter = desc.setter();
        } else {
            setter = (shapeHasDefaultSetter && !shapeHasSetterValue)
                     ? JS_StrictPropertyStub
                     : shape->setter();
        }
    }

    *rval = true;

    








    if (callDelProperty) {
        bool succeeded;
        if (!CallJSDeletePropertyOp(cx, obj2->getClass()->delProperty, obj2, id, &succeeded))
            return false;
    }

    return baseops::DefineGeneric(cx, obj, id, v, getter, setter, attrs);
}


static bool
DefinePropertyOnArray(JSContext *cx, Handle<ArrayObject*> arr, HandleId id, const PropDesc &desc,
                      bool throwError, bool *rval)
{
    
    if (id == NameToId(cx->names().length)) {
        
        
        
        
        
        
        
        RootedValue v(cx);
        if (desc.hasValue()) {
            uint32_t newLen;
            if (!CanonicalizeArrayLengthValue<SequentialExecution>(cx, desc.value(), &newLen))
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

        unsigned attrs = arr->nativeLookup(cx, id)->attributes();
        if (!arr->lengthIsWritable()) {
            if (desc.hasWritable() && desc.writable())
                return Reject(cx, id, JSMSG_CANT_REDEFINE_PROP, throwError, rval);
        } else {
            if (desc.hasWritable() && !desc.writable())
                attrs = attrs | JSPROP_READONLY;
        }

        return ArraySetLength<SequentialExecution>(cx, arr, id, attrs, v, throwError);
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

bool
js::DefineProperty(JSContext *cx, HandleObject obj, HandleId id, const PropDesc &desc,
                   bool throwError, bool *rval)
{
    if (obj->is<ArrayObject>()) {
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        return DefinePropertyOnArray(cx, arr, id, desc, throwError, rval);
    }

    if (obj->getOps()->lookupGeneric) {
        if (obj->is<ProxyObject>()) {
            Rooted<PropertyDescriptor> pd(cx);
            desc.populatePropertyDescriptor(obj, &pd);
            pd.object().set(obj);
            return Proxy::defineProperty(cx, obj, id, &pd);
        }
        return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, throwError, rval);
    }

    return DefinePropertyOnObject(cx, obj, id, desc, throwError, rval);
}

bool
js::DefineOwnProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue descriptor,
                      bool *bp)
{
    Rooted<PropDesc> desc(cx);
    if (!desc.initialize(cx, descriptor))
        return false;

    bool rval;
    if (!DefineProperty(cx, obj, id, desc, true, &rval))
        return false;
    *bp = !!rval;
    return true;
}

bool
js::DefineOwnProperty(JSContext *cx, HandleObject obj, HandleId id,
                      Handle<PropertyDescriptor> descriptor, bool *bp)
{
    Rooted<PropDesc> desc(cx);

    desc.initFromPropertyDescriptor(descriptor);

    bool rval;
    if (!DefineProperty(cx, obj, id, desc, true, &rval))
        return false;
    *bp = !!rval;
    return true;
}


bool
js::ReadPropertyDescriptors(JSContext *cx, HandleObject props, bool checkAccessors,
                            AutoIdVector *ids, AutoPropDescVector *descs)
{
    if (!GetPropertyNames(cx, props, JSITER_OWNONLY, ids))
        return false;

    RootedId id(cx);
    for (size_t i = 0, len = ids->length(); i < len; i++) {
        id = (*ids)[i];
        Rooted<PropDesc> desc(cx);
        RootedValue v(cx);
        if (!JSObject::getGeneric(cx, props, props, id, &v) ||
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

    if (obj->is<ArrayObject>()) {
        bool dummy;
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        for (size_t i = 0, len = ids.length(); i < len; i++) {
            if (!DefinePropertyOnArray(cx, arr, ids[i], descs[i], true, &dummy))
                return false;
        }
        return true;
    }

    if (obj->getOps()->lookupGeneric) {
        if (obj->is<ProxyObject>()) {
            Rooted<PropertyDescriptor> pd(cx);
            for (size_t i = 0, len = ids.length(); i < len; i++) {
                descs[i].populatePropertyDescriptor(obj, &pd);
                if (!Proxy::defineProperty(cx, obj, ids[i], &pd))
                    return false;
            }
            return true;
        }
        bool dummy;
        return Reject(cx, obj, JSMSG_OBJECT_NOT_EXTENSIBLE, true, &dummy);
    }

    bool dummy;
    for (size_t i = 0, len = ids.length(); i < len; i++) {
        if (!DefinePropertyOnObject(cx, obj, ids[i], descs[i], true, &dummy))
            return false;
    }

    return true;
}

extern bool
js_PopulateObject(JSContext *cx, HandleObject newborn, HandleObject props)
{
    return DefineProperties(cx, newborn, props);
}

js::types::TypeObject*
JSObject::uninlinedGetType(JSContext *cx)
{
    return getType(cx);
}

void
JSObject::uninlinedSetType(js::types::TypeObject *newType)
{
    setType(newType);
}

 inline unsigned
JSObject::getSealedOrFrozenAttributes(unsigned attrs, ImmutabilityType it)
{
    
    if (it == FREEZE && !(attrs & (JSPROP_GETTER | JSPROP_SETTER)))
        return JSPROP_PERMANENT | JSPROP_READONLY;
    return JSPROP_PERMANENT;
}

 bool
JSObject::sealOrFreeze(JSContext *cx, HandleObject obj, ImmutabilityType it)
{
    assertSameCompartment(cx, obj);
    JS_ASSERT(it == SEAL || it == FREEZE);

    bool extensible;
    if (!JSObject::isExtensible(cx, obj, &extensible))
        return false;
    if (extensible && !JSObject::preventExtensions(cx, obj))
        return false;

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    
    JS_ASSERT_IF(obj->isNative(), obj->getDenseCapacity() == 0);

    if (obj->isNative() && !obj->inDictionaryMode() && !obj->is<TypedArrayObject>()) {
        






        RootedShape last(cx, EmptyShape::getInitialShape(cx, obj->getClass(),
                                                         obj->getTaggedProto(),
                                                         obj->getParent(),
                                                         obj->getMetadata(),
                                                         obj->numFixedSlots(),
                                                         obj->lastProperty()->getObjectFlags()));
        if (!last)
            return false;

        
        AutoShapeVector shapes(cx);
        for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront()) {
            if (!shapes.append(&r.front()))
                return false;
        }
        Reverse(shapes.begin(), shapes.end());

        for (size_t i = 0; i < shapes.length(); i++) {
            StackShape unrootedChild(shapes[i]);
            RootedGeneric<StackShape*> child(cx, &unrootedChild);
            child->attrs |= getSealedOrFrozenAttributes(child->attrs, it);

            if (!JSID_IS_EMPTY(child->propid) && it == FREEZE)
                MarkTypePropertyNonWritable(cx, obj, child->propid);

            last = cx->compartment()->propertyTree.getChild(cx, last, *child);
            if (!last)
                return false;
        }

        JS_ASSERT(obj->lastProperty()->slotSpan() == last->slotSpan());
        JS_ALWAYS_TRUE(setLastProperty(cx, obj, last));
    } else {
        RootedId id(cx);
        for (size_t i = 0; i < props.length(); i++) {
            id = props[i];

            unsigned attrs;
            if (!getGenericAttributes(cx, obj, id, &attrs))
                return false;

            unsigned new_attrs = getSealedOrFrozenAttributes(attrs, it);

            
            if ((attrs | new_attrs) == attrs)
                continue;

            attrs |= new_attrs;
            if (!setGenericAttributes(cx, obj, id, &attrs))
                return false;
        }
    }

    
    
    
    
    
    
    
    
    
    if (it == FREEZE && obj->is<ArrayObject>())
        obj->getElementsHeader()->setNonwritableArrayLength();

    return true;
}

 bool
JSObject::isSealedOrFrozen(JSContext *cx, HandleObject obj, ImmutabilityType it, bool *resultp)
{
    bool extensible;
    if (!JSObject::isExtensible(cx, obj, &extensible))
        return false;
    if (extensible) {
        *resultp = false;
        return true;
    }

    if (obj->is<TypedArrayObject>()) {
        if (it == SEAL) {
            
            *resultp = true;
        } else {
            
            
            *resultp = (obj->as<TypedArrayObject>().length() == 0);
        }
        return true;
    }

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_HIDDEN | JSITER_OWNONLY, &props))
        return false;

    RootedId id(cx);
    for (size_t i = 0, len = props.length(); i < len; i++) {
        id = props[i];

        unsigned attrs;
        if (!getGenericAttributes(cx, obj, id, &attrs))
            return false;

        




        if (!(attrs & JSPROP_PERMANENT) ||
            (it == FREEZE && !(attrs & (JSPROP_READONLY | JSPROP_GETTER | JSPROP_SETTER))))
        {
            *resultp = false;
            return true;
        }
    }

    
    *resultp = true;
    return true;
}


const char *
JSObject::className(JSContext *cx, HandleObject obj)
{
    assertSameCompartment(cx, obj);

    if (obj->is<ProxyObject>())
        return Proxy::className(cx, obj);

    return obj->getClass()->name;
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
NewObject(ExclusiveContext *cx, types::TypeObject *type_, JSObject *parent, gc::AllocKind kind,
          NewObjectKind newKind)
{
    const Class *clasp = type_->clasp();

    JS_ASSERT(clasp != &ArrayObject::class_);
    JS_ASSERT_IF(clasp == &JSFunction::class_,
                 kind == JSFunction::FinalizeKind || kind == JSFunction::ExtendedFinalizeKind);
    JS_ASSERT_IF(parent, &parent->global() == cx->global());

    RootedTypeObject type(cx, type_);

    JSObject *metadata = nullptr;
    if (!NewObjectMetadata(cx, &metadata))
        return nullptr;

    
    
    
    size_t nfixed = ClassCanHaveFixedData(clasp)
                    ? GetGCKindSlots(gc::GetGCObjectKind(clasp), clasp)
                    : GetGCKindSlots(kind, clasp);

    RootedShape shape(cx, EmptyShape::getInitialShape(cx, clasp, type->proto(),
                                                      parent, metadata, nfixed));
    if (!shape)
        return nullptr;

    gc::InitialHeap heap = GetInitialHeap(newKind, clasp);
    JSObject *obj = JSObject::create(cx, kind, heap, shape, type);
    if (!obj)
        return nullptr;

    if (newKind == SingletonObject) {
        RootedObject nobj(cx, obj);
        if (!JSObject::setSingletonType(cx, nobj))
            return nullptr;
        obj = nobj;
    }

    



    bool globalWithoutCustomTrace = clasp->trace == JS_GlobalObjectTraceHook &&
                                    !cx->compartment()->options().getTrace();
    if (clasp->trace &&
        !globalWithoutCustomTrace &&
        !(clasp->flags & JSCLASS_IMPLEMENTS_BARRIERS))
    {
        if (!cx->shouldBeJSContext())
            return nullptr;
        JSRuntime *rt = cx->asJSContext()->runtime();
        rt->gc.incrementalEnabled = false;

#ifdef DEBUG
        if (rt->gcMode() == JSGC_MODE_INCREMENTAL) {
            fprintf(stderr,
                    "The class %s has a trace hook but does not declare the\n"
                    "JSCLASS_IMPLEMENTS_BARRIERS flag. Please ensure that it correctly\n"
                    "implements write barriers and then set the flag.\n",
                    clasp->name);
            MOZ_CRASH();
        }
#endif
    }

    probes::CreateObject(cx, obj);
    return obj;
}

void
NewObjectCache::fillProto(EntryIndex entry, const Class *clasp, js::TaggedProto proto,
                          gc::AllocKind kind, JSObject *obj)
{
    JS_ASSERT_IF(proto.isObject(), !proto.toObject()->is<GlobalObject>());
    JS_ASSERT(obj->getTaggedProto() == proto);
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
    if (JSContext *cx = cxArg->maybeJSContext()) {
        NewObjectCache &cache = cx->runtime()->newObjectCache;
        if (protoArg.isObject() &&
            newKind == GenericObject &&
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
                    JS_ASSERT(!obj);
                    parentArg = parent;
                    protoArg = proto;
                }
            }
        }
    }

    Rooted<TaggedProto> proto(cxArg, protoArg);
    RootedObject parent(cxArg, parentArg);

    types::TypeObject *type = cxArg->getNewType(clasp, proto, nullptr);
    if (!type)
        return nullptr;

    



    if (!parent && proto.isObject())
        parent = proto.toObject()->getParent();

    RootedObject obj(cxArg, NewObject(cxArg, type, parent, allocKind, newKind));
    if (!obj)
        return nullptr;

    if (entry != -1 && !obj->hasDynamicSlots()) {
        cxArg->asJSContext()->runtime()->newObjectCache.fillProto(entry, clasp,
                                                                  proto, allocKind, obj);
    }

    return obj;
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

    








    JSProtoKey protoKey = GetClassProtoKey(clasp);

    NewObjectCache::EntryIndex entry = -1;
    if (JSContext *cx = cxArg->maybeJSContext()) {
        NewObjectCache &cache = cx->runtime()->newObjectCache;
        if (parentArg->is<GlobalObject>() &&
            protoKey != JSProto_Null &&
            newKind == GenericObject &&
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
                    JS_ASSERT(!obj);
                    protoArg = proto;
                    parentArg = parent;
                }
            }
        }
    }

    RootedObject parent(cxArg, parentArg);
    RootedObject proto(cxArg, protoArg);

    if (!FindProto(cxArg, clasp, &proto))
        return nullptr;

    Rooted<TaggedProto> taggedProto(cxArg, TaggedProto(proto));
    types::TypeObject *type = cxArg->getNewType(clasp, taggedProto);
    if (!type)
        return nullptr;

    JSObject *obj = NewObject(cxArg, type, parent, allocKind, newKind);
    if (!obj)
        return nullptr;

    if (entry != -1 && !obj->hasDynamicSlots()) {
        cxArg->asJSContext()->runtime()->newObjectCache.fillGlobal(entry, clasp,
                                                                   &parent->as<GlobalObject>(),
                                                                   allocKind, obj);
    }

    return obj;
}





JSObject *
js::NewObjectWithType(JSContext *cx, HandleTypeObject type, JSObject *parent, gc::AllocKind allocKind,
                      NewObjectKind newKind)
{
    JS_ASSERT(parent);

    JS_ASSERT(allocKind <= gc::FINALIZE_OBJECT_LAST);
    if (CanBeFinalizedInBackground(allocKind, type->clasp()))
        allocKind = GetBackgroundAllocKind(allocKind);

    NewObjectCache &cache = cx->runtime()->newObjectCache;

    NewObjectCache::EntryIndex entry = -1;
    if (parent == type->proto().toObject()->getParent() &&
        newKind == GenericObject &&
        !cx->compartment()->hasObjectMetadataCallback())
    {
        if (cache.lookupType(type, allocKind, &entry)) {
            JSObject *obj = cache.newObjectFromHit<NoGC>(cx, entry, GetInitialHeap(newKind, type->clasp()));
            if (obj) {
                return obj;
            } else {
                obj = cache.newObjectFromHit<CanGC>(cx, entry, GetInitialHeap(newKind, type->clasp()));
                parent = type->proto().toObject()->getParent();
            }
        }
    }

    JSObject *obj = NewObject(cx, type, parent, allocKind, newKind);
    if (!obj)
        return nullptr;

    if (entry != -1 && !obj->hasDynamicSlots())
        cache.fillType(entry, type, allocKind, obj);

    return obj;
}

bool
js::NewObjectScriptedCall(JSContext *cx, MutableHandleObject pobj)
{
    jsbytecode *pc;
    RootedScript script(cx, cx->currentScript(&pc));
    gc::AllocKind allocKind = NewObjectGCKind(&JSObject::class_);
    NewObjectKind newKind = script
                            ? UseNewTypeForInitializer(script, pc, &JSObject::class_)
                            : GenericObject;
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &JSObject::class_, allocKind, newKind));
    if (!obj)
        return false;

    if (script) {
        
        if (!types::SetInitializerObjectType(cx, script, pc, obj, newKind))
            return false;
    }

    pobj.set(obj);
    return true;
}

JSObject*
js::CreateThis(JSContext *cx, const Class *newclasp, HandleObject callee)
{
    RootedValue protov(cx);
    if (!JSObject::getProperty(cx, callee, callee, cx->names().prototype, &protov))
        return nullptr;

    JSObject *proto = protov.isObjectOrNull() ? protov.toObjectOrNull() : nullptr;
    JSObject *parent = callee->getParent();
    gc::AllocKind kind = NewObjectGCKind(newclasp);
    return NewObjectWithClassProto(cx, newclasp, proto, parent, kind);
}

static inline JSObject *
CreateThisForFunctionWithType(JSContext *cx, HandleTypeObject type, JSObject *parent,
                              NewObjectKind newKind)
{
    if (type->hasNewScript()) {
        




        RootedObject templateObject(cx, type->newScript()->templateObject);
        JS_ASSERT(templateObject->type() == type);

        RootedObject res(cx, CopyInitializerObject(cx, templateObject, newKind));
        if (!res)
            return nullptr;
        if (newKind == SingletonObject) {
            Rooted<TaggedProto> proto(cx, TaggedProto(templateObject->getProto()));
            if (!res->splicePrototype(cx, &JSObject::class_, proto))
                return nullptr;
        } else {
            res->setType(type);
        }
        return res;
    }

    gc::AllocKind allocKind = NewObjectGCKind(&JSObject::class_);
    return NewObjectWithType(cx, type, parent, allocKind, newKind);
}

JSObject *
js::CreateThisForFunctionWithProto(JSContext *cx, HandleObject callee, JSObject *proto,
                                   NewObjectKind newKind )
{
    RootedObject res(cx);

    if (proto) {
        RootedTypeObject type(cx, cx->getNewType(&JSObject::class_, TaggedProto(proto), &callee->as<JSFunction>()));
        if (!type)
            return nullptr;
        res = CreateThisForFunctionWithType(cx, type, callee->getParent(), newKind);
    } else {
        gc::AllocKind allocKind = NewObjectGCKind(&JSObject::class_);
        res = NewObjectWithClassProto(cx, &JSObject::class_, proto, callee->getParent(), allocKind, newKind);
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
    if (!JSObject::getProperty(cx, callee, callee, cx->names().prototype, &protov))
        return nullptr;
    JSObject *proto;
    if (protov.isObject())
        proto = &protov.toObject();
    else
        proto = nullptr;
    JSObject *obj = CreateThisForFunctionWithProto(cx, callee, proto, newKind);

    if (obj && newKind == SingletonObject) {
        RootedObject nobj(cx, obj);

        
        JSObject::clear(cx, nobj);

        JSScript *calleeScript = callee->as<JSFunction>().nonLazyScript();
        TypeScript::SetThis(cx, calleeScript, types::Type::ObjectType(nobj));

        return nobj;
    }

    return obj;
}






static bool
Detecting(JSContext *cx, JSScript *script, jsbytecode *pc)
{
    JS_ASSERT(script->containsPC(pc));

    
    JSOp op = JSOp(*pc);
    if (js_CodeSpec[op].format & JOF_DETECTING)
        return true;

    jsbytecode *endpc = script->codeEnd();

    if (op == JSOP_NULL) {
        



        if (++pc < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE;
        }
        return false;
    }

    if (op == JSOP_GETGNAME || op == JSOP_NAME) {
        




        JSAtom *atom = script->getAtom(GET_UINT32_INDEX(pc));
        if (atom == cx->names().undefined &&
            (pc += js_CodeSpec[op].length) < endpc) {
            op = JSOp(*pc);
            return op == JSOP_EQ || op == JSOP_NE || op == JSOP_STRICTEQ || op == JSOP_STRICTNE;
        }
    }

    return false;
}

 bool
JSObject::nonNativeSetProperty(JSContext *cx, HandleObject obj,
                               HandleId id, MutableHandleValue vp, bool strict)
{
    if (MOZ_UNLIKELY(obj->watched())) {
        WatchpointMap *wpmap = cx->compartment()->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }
    return obj->getOps()->setGeneric(cx, obj, id, vp, strict);
}

 bool
JSObject::nonNativeSetElement(JSContext *cx, HandleObject obj,
                              uint32_t index, MutableHandleValue vp, bool strict)
{
    if (MOZ_UNLIKELY(obj->watched())) {
        RootedId id(cx);
        if (!IndexToId(cx, index, &id))
            return false;

        WatchpointMap *wpmap = cx->compartment()->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }
    return obj->getOps()->setElement(cx, obj, index, vp, strict);
}

JS_FRIEND_API(bool)
JS_CopyPropertyFrom(JSContext *cx, HandleId id, HandleObject target,
                    HandleObject obj)
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

    JSAutoCompartment ac(cx, target);
    RootedId wrappedId(cx, id);
    if (!cx->compartment()->wrap(cx, &desc))
        return false;
    if (!cx->compartment()->wrapId(cx, wrappedId.address()))
        return false;

    bool ignored;
    return DefineOwnProperty(cx, target, wrappedId, desc, &ignored);
}

JS_FRIEND_API(bool)
JS_CopyPropertiesFrom(JSContext *cx, HandleObject target, HandleObject obj)
{
    JSAutoCompartment ac(cx, obj);

    AutoIdVector props(cx);
    if (!GetPropertyNames(cx, obj, JSITER_OWNONLY | JSITER_HIDDEN, &props))
        return false;

    for (size_t i = 0; i < props.length(); ++i) {
        if (!JS_CopyPropertyFrom(cx, props[i], target, obj))
            return false;
    }

    return true;
}

static bool
CopySlots(JSContext *cx, HandleObject from, HandleObject to)
{
    JS_ASSERT(!from->isNative() && !to->isNative());
    JS_ASSERT(from->getClass() == to->getClass());

    size_t n = 0;
    if (from->is<WrapperObject>() &&
        (Wrapper::wrapperHandler(from)->flags() &
         Wrapper::CROSS_COMPARTMENT)) {
        to->setSlot(0, from->getSlot(0));
        to->setSlot(1, from->getSlot(1));
        n = 2;
    }

    size_t span = JSCLASS_RESERVED_SLOTS(from->getClass());
    RootedValue v(cx);
    for (; n < span; ++n) {
        v = from->getSlot(n);
        if (!cx->compartment()->wrap(cx, &v))
            return false;
        to->setSlot(n, v);
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

    RootedObject clone(cx, NewObjectWithGivenProto(cx, obj->getClass(), proto, parent));
    if (!clone)
        return nullptr;
    if (obj->isNative()) {
        if (clone->is<JSFunction>() && (obj->compartment() != clone->compartment())) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_CANT_CLONE_OBJECT);
            return nullptr;
        }

        if (obj->hasPrivate())
            clone->setPrivate(obj->getPrivate());
    } else {
        JS_ASSERT(obj->is<ProxyObject>());
        if (!CopySlots(cx, obj, clone))
            return nullptr;
    }

    return clone;
}

JSObject *
js::DeepCloneObjectLiteral(JSContext *cx, HandleObject obj, NewObjectKind newKind)
{
    
    JS_ASSERT(JS::CompartmentOptionsRef(cx).getSingletonsAsTemplates());
    JS_ASSERT(obj->is<JSObject>() || obj->is<ArrayObject>());

    
    RootedObject clone(cx);

    
    RootedValue v(cx);
    RootedObject deepObj(cx);

    if (obj->getClass() == &ArrayObject::class_) {
        clone = NewDenseUnallocatedArray(cx, obj->as<ArrayObject>().length(), nullptr, newKind);
    } else {
        
        JS_ASSERT(obj->isTenured());
        AllocKind kind = obj->tenuredGetAllocKind();
        Rooted<TypeObject*> typeObj(cx, obj->getType(cx));
        if (!typeObj)
            return nullptr;
        RootedObject parent(cx, obj->getParent());
        clone = NewObjectWithGivenProto(cx, &JSObject::class_, TaggedProto(typeObj->proto().toObject()),
                                        parent, kind, newKind);
    }

    
    if (!clone || !clone->ensureElements(cx, obj->getDenseCapacity()))
        return nullptr;

    
    uint32_t initialized = obj->getDenseInitializedLength();
    if (initialized)
        clone->setDenseInitializedLength(initialized);

    
    for (uint32_t i = 0; i < initialized; ++i) {
        v = obj->getDenseElement(i);
        if (v.isObject()) {
            deepObj = &v.toObject();
            deepObj = js::DeepCloneObjectLiteral(cx, deepObj, newKind);
            if (!deepObj) {
                JS_ReportOutOfMemory(cx);
                return nullptr;
            }
            v.setObject(*deepObj);
        }
        clone->initDenseElement(i, v);
    }

    JS_ASSERT(obj->compartment() == clone->compartment());
    JS_ASSERT(!obj->hasPrivate());
    RootedShape shape(cx, obj->lastProperty());
    size_t span = shape->slotSpan();
    clone->setLastProperty(cx, clone, shape);
    for (size_t i = 0; i < span; i++) {
        v = obj->getSlot(i);
        if (v.isObject()) {
            deepObj = &v.toObject();
            deepObj = js::DeepCloneObjectLiteral(cx, deepObj, newKind);
            if (!deepObj)
                return nullptr;
            v.setObject(*deepObj);
        }
        clone->setSlot(i, v);
    }

    if (obj->getClass() == &ArrayObject::class_)
        FixArrayType(cx, clone);
    else
        FixObjectType(cx, clone);

#ifdef DEBUG
    Rooted<TypeObject*> typeObj(cx, obj->getType(cx));
    Rooted<TypeObject*> cloneTypeObj(cx, clone->getType(cx));
    if (!typeObj || !cloneTypeObj)
        return nullptr;
    JS_ASSERT(typeObj == cloneTypeObj);
#endif

    return clone;
}

template<XDRMode mode>
bool
js::XDRObjectLiteral(XDRState<mode> *xdr, MutableHandleObject obj)
{
    

    JSContext *cx = xdr->cx();
    JS_ASSERT_IF(mode == XDR_ENCODE, JS::CompartmentOptionsRef(cx).getSingletonsAsTemplates());

    
    uint32_t isArray = 0;
    {
        if (mode == XDR_ENCODE) {
            JS_ASSERT(obj->is<JSObject>() || obj->is<ArrayObject>());
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
                JS_ASSERT(obj->getClass() == &JSObject::class_);
                JS_ASSERT(obj->isTenured());
                kind = obj->tenuredGetAllocKind();
            }

            if (!xdr->codeEnum32(&kind))
                return false;

            if (mode == XDR_DECODE)
                obj.set(NewBuiltinClassInstance(cx, &JSObject::class_, kind, js::MaybeSingletonObject));
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

    JS_ASSERT(!obj->hasPrivate());
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
                    JS_ASSERT(isArray);
                    break;
                }

                JS_ASSERT(it.front().hasDefaultGetter());
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
                    MOZ_ASSUME_UNREACHABLE("Object property is not yet supported by XDR.");

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
                JS_ASSERT(idType == JSID_TYPE_INT);
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
                if (!DefineNativeProperty(cx, obj, id, tmpValue, NULL, NULL, JSPROP_ENUMERATE))
                    return false;
            }
        }

        JS_ASSERT_IF(mode == XDR_DECODE, !obj->inDictionaryMode());
    }

    if (mode == XDR_DECODE) {
        if (isArray)
            FixArrayType(cx, obj);
        else
            FixObjectType(cx, obj);
    }

    return true;
}

template bool
js::XDRObjectLiteral(XDRState<XDR_ENCODE> *xdr, MutableHandleObject obj);

template bool
js::XDRObjectLiteral(XDRState<XDR_DECODE> *xdr, MutableHandleObject obj);

JSObject *
js::CloneObjectLiteral(JSContext *cx, HandleObject parent, HandleObject srcObj)
{
    Rooted<TypeObject*> typeObj(cx);
    typeObj = cx->getNewType(&JSObject::class_, TaggedProto(cx->global()->getOrCreateObjectPrototype(cx)));

    JS_ASSERT(srcObj->getClass() == &JSObject::class_);
    AllocKind kind = GetBackgroundAllocKind(GuessObjectGCKind(srcObj->numFixedSlots()));
    JS_ASSERT_IF(srcObj->isTenured(), kind == srcObj->tenuredGetAllocKind());

    RootedShape shape(cx, srcObj->lastProperty());
    return NewReshapedObject(cx, typeObj, parent, kind, shape);
}

struct JSObject::TradeGutsReserved {
    Vector<Value> avals;
    Vector<Value> bvals;
    int newafixed;
    int newbfixed;
    RootedShape newashape;
    RootedShape newbshape;
    HeapSlot *newaslots;
    HeapSlot *newbslots;

    explicit TradeGutsReserved(JSContext *cx)
        : avals(cx), bvals(cx),
          newafixed(0), newbfixed(0),
          newashape(cx), newbshape(cx),
          newaslots(nullptr), newbslots(nullptr)
    {}

    ~TradeGutsReserved()
    {
        js_free(newaslots);
        js_free(newbslots);
    }
};

bool
JSObject::ReserveForTradeGuts(JSContext *cx, JSObject *aArg, JSObject *bArg,
                              TradeGutsReserved &reserved)
{
    



    AutoSuppressGC suppress(cx);

    RootedObject a(cx, aArg);
    RootedObject b(cx, bArg);
    JS_ASSERT(a->compartment() == b->compartment());
    AutoCompartment ac(cx, a);

    





    



    const Class *aClass = a->getClass();
    const Class *bClass = b->getClass();
    Rooted<TaggedProto> aProto(cx, a->getTaggedProto());
    Rooted<TaggedProto> bProto(cx, b->getTaggedProto());
    bool success;
    if (!SetClassAndProto(cx, a, bClass, bProto, &success) || !success)
        return false;
    if (!SetClassAndProto(cx, b, aClass, aProto, &success) || !success)
        return false;

    if (a->tenuredSizeOfThis() == b->tenuredSizeOfThis())
        return true;

    





    if (a->isNative()) {
        if (!a->generateOwnShape(cx))
            return false;
    } else {
        reserved.newbshape = EmptyShape::getInitialShape(cx, aClass, aProto, a->getParent(), a->getMetadata(),
                                                         b->tenuredGetAllocKind());
        if (!reserved.newbshape)
            return false;
    }
    if (b->isNative()) {
        if (!b->generateOwnShape(cx))
            return false;
    } else {
        reserved.newashape = EmptyShape::getInitialShape(cx, bClass, bProto, b->getParent(), b->getMetadata(),
                                                         a->tenuredGetAllocKind());
        if (!reserved.newashape)
            return false;
    }

    

    if (!reserved.avals.reserve(a->slotSpan()))
        return false;
    if (!reserved.bvals.reserve(b->slotSpan()))
        return false;

    





    reserved.newafixed = a->numFixedSlots();
    reserved.newbfixed = b->numFixedSlots();

    if (aClass->hasPrivate()) {
        reserved.newafixed++;
        reserved.newbfixed--;
    }
    if (bClass->hasPrivate()) {
        reserved.newbfixed++;
        reserved.newafixed--;
    }

    JS_ASSERT(reserved.newafixed >= 0);
    JS_ASSERT(reserved.newbfixed >= 0);

    





    unsigned adynamic = dynamicSlotsCount(reserved.newafixed, b->slotSpan(), b->getClass());
    unsigned bdynamic = dynamicSlotsCount(reserved.newbfixed, a->slotSpan(), a->getClass());

    if (adynamic) {
        reserved.newaslots = cx->pod_malloc<HeapSlot>(adynamic);
        if (!reserved.newaslots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(reserved.newaslots, adynamic);
    }
    if (bdynamic) {
        reserved.newbslots = cx->pod_malloc<HeapSlot>(bdynamic);
        if (!reserved.newbslots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(reserved.newbslots, bdynamic);
    }

    return true;
}

void
JSObject::TradeGuts(JSContext *cx, JSObject *a, JSObject *b, TradeGutsReserved &reserved)
{
    JS_ASSERT(a->compartment() == b->compartment());
    JS_ASSERT(a->is<JSFunction>() == b->is<JSFunction>());

    



    TypeObject *tmp = a->type_;
    a->type_ = b->type_;
    b->type_ = tmp;

    
    JS_ASSERT_IF(a->is<JSFunction>(), a->tenuredSizeOfThis() == b->tenuredSizeOfThis());

    




    JS_ASSERT(!a->is<RegExpObject>() && !b->is<RegExpObject>());

    
    JS_ASSERT(!a->is<ArrayObject>() && !b->is<ArrayObject>());

    



    JS_ASSERT(!a->is<ArrayBufferObject>() && !b->is<ArrayBufferObject>());

    
    const size_t size = a->tenuredSizeOfThis();
    if (size == b->tenuredSizeOfThis()) {
        




        char tmp[mozilla::tl::Max<sizeof(JSFunction), sizeof(JSObject_Slots16)>::value];
        JS_ASSERT(size <= sizeof(tmp));

        js_memcpy(tmp, a, size);
        js_memcpy(a, b, size);
        js_memcpy(b, tmp, size);

#ifdef JSGC_GENERATIONAL
        



        for (size_t i = 0; i < a->numFixedSlots(); ++i) {
            HeapSlot::writeBarrierPost(a, HeapSlot::Slot, i, a->getSlot(i));
            HeapSlot::writeBarrierPost(b, HeapSlot::Slot, i, b->getSlot(i));
        }
#endif
    } else {
        





        uint32_t acap = a->slotSpan();
        uint32_t bcap = b->slotSpan();

        for (size_t i = 0; i < acap; i++)
            reserved.avals.infallibleAppend(a->getSlot(i));

        for (size_t i = 0; i < bcap; i++)
            reserved.bvals.infallibleAppend(b->getSlot(i));

        
        if (a->hasDynamicSlots())
            js_free(a->slots);
        if (b->hasDynamicSlots())
            js_free(b->slots);

        void *apriv = a->hasPrivate() ? a->getPrivate() : nullptr;
        void *bpriv = b->hasPrivate() ? b->getPrivate() : nullptr;

        char tmp[sizeof(JSObject)];
        js_memcpy(&tmp, a, sizeof tmp);
        js_memcpy(a, b, sizeof tmp);
        js_memcpy(b, &tmp, sizeof tmp);

        if (a->isNative())
            a->shape_->setNumFixedSlots(reserved.newafixed);
        else
            a->shape_ = reserved.newashape;

        a->slots = reserved.newaslots;
        a->initSlotRange(0, reserved.bvals.begin(), bcap);
        if (a->hasPrivate())
            a->initPrivate(bpriv);

        if (b->isNative())
            b->shape_->setNumFixedSlots(reserved.newbfixed);
        else
            b->shape_ = reserved.newbshape;

        b->slots = reserved.newbslots;
        b->initSlotRange(0, reserved.avals.begin(), acap);
        if (b->hasPrivate())
            b->initPrivate(apriv);

        
        reserved.newaslots = nullptr;
        reserved.newbslots = nullptr;
    }

#ifdef JSGC_GENERATIONAL
    Shape::writeBarrierPost(a->shape_, &a->shape_);
    Shape::writeBarrierPost(b->shape_, &b->shape_);
    types::TypeObject::writeBarrierPost(a->type_, &a->type_);
    types::TypeObject::writeBarrierPost(b->type_, &b->type_);
#endif

    if (a->inDictionaryMode())
        a->lastProperty()->listp = &a->shape_;
    if (b->inDictionaryMode())
        b->lastProperty()->listp = &b->shape_;

#ifdef JSGC_INCREMENTAL
    









    JS::Zone *zone = a->zone();
    if (zone->needsBarrier()) {
        MarkChildren(zone->barrierTracer(), a);
        MarkChildren(zone->barrierTracer(), b);
    }
#endif
}


bool
JSObject::swap(JSContext *cx, HandleObject a, HandleObject b)
{
    AutoMarkInDeadZone adc1(a->zone());
    AutoMarkInDeadZone adc2(b->zone());

    
    JS_ASSERT(IsBackgroundFinalized(a->tenuredGetAllocKind()) ==
              IsBackgroundFinalized(b->tenuredGetAllocKind()));
    JS_ASSERT(a->compartment() == b->compartment());

    unsigned r = NotifyGCPreSwap(a, b);

    TradeGutsReserved reserved(cx);
    if (!ReserveForTradeGuts(cx, a, b, reserved)) {
        NotifyGCPostSwap(b, a, r);
        return false;
    }
    TradeGuts(cx, a, b, reserved);

    NotifyGCPostSwap(a, b, r);
    return true;
}

static bool
DefineStandardSlot(JSContext *cx, HandleObject obj, JSProtoKey key, JSAtom *atom,
                   HandleValue v, uint32_t attrs, bool &named)
{
    RootedId id(cx, AtomToId(atom));

    if (key != JSProto_Null) {
        




        JS_ASSERT(obj->is<GlobalObject>());
        JS_ASSERT(obj->isNative());

        if (!obj->nativeLookup(cx, id)) {
            obj->as<GlobalObject>().setConstructorPropertySlot(key, v);

            uint32_t slot = GlobalObject::constructorPropertySlot(key);
            if (!JSObject::addProperty(cx, obj, id, JS_PropertyStub, JS_StrictPropertyStub, slot, attrs, 0))
                return false;

            named = true;
            return true;
        }
    }

    named = JSObject::defineGeneric(cx, obj, id,
                                    v, JS_PropertyStub, JS_StrictPropertyStub, attrs);
    return named;
}

static void
SetClassObject(JSObject *obj, JSProtoKey key, JSObject *cobj, JSObject *proto)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->is<GlobalObject>())
        return;

    obj->as<GlobalObject>().setConstructor(key, ObjectOrNullValue(cobj));
    obj->as<GlobalObject>().setPrototype(key, ObjectOrNullValue(proto));
}

static void
ClearClassObject(JSObject *obj, JSProtoKey key)
{
    JS_ASSERT(!obj->getParent());
    if (!obj->is<GlobalObject>())
        return;

    obj->as<GlobalObject>().setConstructor(key, UndefinedValue());
    obj->as<GlobalObject>().setPrototype(key, UndefinedValue());
}

static JSObject *
DefineConstructorAndPrototype(JSContext *cx, HandleObject obj, JSProtoKey key, HandleAtom atom,
                              JSObject *protoProto, const Class *clasp,
                              Native constructor, unsigned nargs,
                              const JSPropertySpec *ps, const JSFunctionSpec *fs,
                              const JSPropertySpec *static_ps, const JSFunctionSpec *static_fs,
                              JSObject **ctorp, AllocKind ctorKind)
{
    





















    






    RootedObject proto(cx, NewObjectWithClassProto(cx, clasp, protoProto, obj, SingletonObject));
    if (!proto)
        return nullptr;

    
    RootedObject ctor(cx);
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

    if (!DefinePropertiesAndBrand(cx, proto, ps, fs) ||
        (ctor != proto && !DefinePropertiesAndBrand(cx, ctor, static_ps, static_fs)))
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
        JSObject::deleteGeneric(cx, obj, id, &succeeded);
    }
    if (cached)
        ClearClassObject(obj, key);
    return nullptr;
}

JSObject *
js_InitClass(JSContext *cx, HandleObject obj, JSObject *protoProto_,
             const Class *clasp, Native constructor, unsigned nargs,
             const JSPropertySpec *ps, const JSFunctionSpec *fs,
             const JSPropertySpec *static_ps, const JSFunctionSpec *static_fs,
             JSObject **ctorp, AllocKind ctorKind)
{
    RootedObject protoProto(cx, protoProto_);

    
    JS_ASSERT(clasp->addProperty);
    JS_ASSERT(clasp->delProperty);
    JS_ASSERT(clasp->getProperty);
    JS_ASSERT(clasp->setProperty);
    JS_ASSERT(clasp->enumerate);
    JS_ASSERT(clasp->resolve);
    JS_ASSERT(clasp->convert);

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

 inline bool
JSObject::updateSlotsForSpan(ThreadSafeContext *cx,
                             HandleObject obj, size_t oldSpan, size_t newSpan)
{
    JS_ASSERT(cx->isThreadLocal(obj));
    JS_ASSERT(oldSpan != newSpan);

    size_t oldCount = dynamicSlotsCount(obj->numFixedSlots(), oldSpan, obj->getClass());
    size_t newCount = dynamicSlotsCount(obj->numFixedSlots(), newSpan, obj->getClass());

    if (oldSpan < newSpan) {
        if (oldCount < newCount && !JSObject::growSlots(cx, obj, oldCount, newCount))
            return false;

        if (newSpan == oldSpan + 1)
            obj->initSlotUnchecked(oldSpan, UndefinedValue());
        else
            obj->initializeSlotRange(oldSpan, newSpan - oldSpan);
    } else {
        
        obj->prepareSlotRangeForOverwrite(newSpan, oldSpan);
        obj->invalidateSlotRange(newSpan, oldSpan - newSpan);

        if (oldCount > newCount)
            JSObject::shrinkSlots(cx, obj, oldCount, newCount);
    }

    return true;
}

 bool
JSObject::setLastProperty(ThreadSafeContext *cx, HandleObject obj, HandleShape shape)
{
    JS_ASSERT(cx->isThreadLocal(obj));
    JS_ASSERT(!obj->inDictionaryMode());
    JS_ASSERT(!shape->inDictionary());
    JS_ASSERT(shape->compartment() == obj->compartment());
    JS_ASSERT(shape->numFixedSlots() == obj->numFixedSlots());

    size_t oldSpan = obj->lastProperty()->slotSpan();
    size_t newSpan = shape->slotSpan();

    if (oldSpan == newSpan) {
        obj->shape_ = shape;
        return true;
    }

    if (!updateSlotsForSpan(cx, obj, oldSpan, newSpan))
        return false;

    obj->shape_ = shape;
    return true;
}

 bool
JSObject::setSlotSpan(ThreadSafeContext *cx, HandleObject obj, uint32_t span)
{
    JS_ASSERT(cx->isThreadLocal(obj));
    JS_ASSERT(obj->inDictionaryMode());

    size_t oldSpan = obj->lastProperty()->base()->slotSpan();
    if (oldSpan == span)
        return true;

    if (!JSObject::updateSlotsForSpan(cx, obj, oldSpan, span))
        return false;

    obj->lastProperty()->base()->setSlotSpan(span);
    return true;
}

static HeapSlot *
AllocateSlots(ThreadSafeContext *cx, JSObject *obj, uint32_t nslots)
{
#ifdef JSGC_GENERATIONAL
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.allocateSlots(cx->asJSContext(), obj, nslots);
#endif
    return cx->pod_malloc<HeapSlot>(nslots);
}

static HeapSlot *
ReallocateSlots(ThreadSafeContext *cx, JSObject *obj, HeapSlot *oldSlots,
                uint32_t oldCount, uint32_t newCount)
{
#ifdef JSGC_GENERATIONAL
    if (cx->isJSContext()) {
        return cx->asJSContext()->runtime()->gc.nursery.reallocateSlots(cx->asJSContext(),
                                                                          obj, oldSlots,
                                                                          oldCount, newCount);
    }
#endif
    return (HeapSlot *)cx->realloc_(oldSlots, oldCount * sizeof(HeapSlot),
                                    newCount * sizeof(HeapSlot));
}

 bool
JSObject::growSlots(ThreadSafeContext *cx, HandleObject obj, uint32_t oldCount, uint32_t newCount)
{
    JS_ASSERT(cx->isThreadLocal(obj));
    JS_ASSERT(newCount > oldCount);
    JS_ASSERT_IF(!obj->is<ArrayObject>(), newCount >= SLOT_CAPACITY_MIN);

    




    JS_ASSERT(newCount < NELEMENTS_LIMIT);

    





    if (!obj->hasLazyType() && !oldCount && obj->type()->hasNewScript()) {
        JSObject *oldTemplate = obj->type()->newScript()->templateObject;
        gc::AllocKind kind = gc::GetGCObjectFixedSlotsKind(oldTemplate->numFixedSlots());
        uint32_t newScriptSlots = gc::GetGCKindSlots(kind);
        if (newScriptSlots == obj->numFixedSlots() &&
            gc::TryIncrementAllocKind(&kind) &&
            cx->isJSContext())
        {
            JSContext *ncx = cx->asJSContext();
            AutoEnterAnalysis enter(ncx);

            Rooted<TypeObject*> typeObj(cx, obj->type());
            RootedShape shape(cx, oldTemplate->lastProperty());
            JSObject *reshapedObj = NewReshapedObject(ncx, typeObj, obj->getParent(), kind, shape,
                                                      MaybeSingletonObject);
            if (!reshapedObj)
                return false;

            typeObj->newScript()->templateObject = reshapedObj;
            typeObj->markStateChange(ncx);
        }
    }

    if (!oldCount) {
        obj->slots = AllocateSlots(cx, obj, newCount);
        if (!obj->slots)
            return false;
        Debug_SetSlotRangeToCrashOnTouch(obj->slots, newCount);
        return true;
    }

    HeapSlot *newslots = ReallocateSlots(cx, obj, obj->slots, oldCount, newCount);
    if (!newslots)
        return false;  

    obj->slots = newslots;

    Debug_SetSlotRangeToCrashOnTouch(obj->slots + oldCount, newCount - oldCount);

    return true;
}

static void
FreeSlots(ThreadSafeContext *cx, HeapSlot *slots)
{
    
#ifdef JSGC_GENERATIONAL
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.freeSlots(cx->asJSContext(), slots);
#endif
    js_free(slots);
}

 void
JSObject::shrinkSlots(ThreadSafeContext *cx, HandleObject obj, uint32_t oldCount, uint32_t newCount)
{
    JS_ASSERT(cx->isThreadLocal(obj));
    JS_ASSERT(newCount < oldCount);

    if (newCount == 0) {
        FreeSlots(cx, obj->slots);
        obj->slots = nullptr;
        return;
    }

    JS_ASSERT_IF(!obj->is<ArrayObject>(), newCount >= SLOT_CAPACITY_MIN);

    HeapSlot *newslots = ReallocateSlots(cx, obj, obj->slots, oldCount, newCount);
    if (!newslots)
        return;  

    obj->slots = newslots;
}

 bool
JSObject::sparsifyDenseElement(ExclusiveContext *cx, HandleObject obj, uint32_t index)
{
    RootedValue value(cx, obj->getDenseElement(index));
    JS_ASSERT(!value.isMagic(JS_ELEMENTS_HOLE));

    JSObject::removeDenseElementForSparseIndex(cx, obj, index);

    uint32_t slot = obj->slotSpan();
    if (!obj->addDataProperty(cx, INT_TO_JSID(index), slot, JSPROP_ENUMERATE)) {
        obj->setDenseElement(index, value);
        return false;
    }

    JS_ASSERT(slot == obj->slotSpan() - 1);
    obj->initSlot(slot, value);

    return true;
}

 bool
JSObject::sparsifyDenseElements(js::ExclusiveContext *cx, HandleObject obj)
{
    uint32_t initialized = obj->getDenseInitializedLength();

    
    for (uint32_t i = 0; i < initialized; i++) {
        if (obj->getDenseElement(i).isMagic(JS_ELEMENTS_HOLE))
            continue;

        if (!sparsifyDenseElement(cx, obj, i))
            return false;
    }

    if (initialized)
        obj->setDenseInitializedLength(0);

    




    if (obj->getDenseCapacity()) {
        obj->shrinkElements(cx, 0);
        obj->getElementsHeader()->capacity = 0;
    }

    return true;
}

bool
JSObject::willBeSparseElements(uint32_t requiredCapacity, uint32_t newElementsHint)
{
    JS_ASSERT(isNative());
    JS_ASSERT(requiredCapacity > MIN_SPARSE_INDEX);

    uint32_t cap = getDenseCapacity();
    JS_ASSERT(requiredCapacity >= cap);

    if (requiredCapacity >= NELEMENTS_LIMIT)
        return true;

    uint32_t minimalDenseCount = requiredCapacity / SPARSE_DENSITY_RATIO;
    if (newElementsHint >= minimalDenseCount)
        return false;
    minimalDenseCount -= newElementsHint;

    if (minimalDenseCount > cap)
        return true;

    uint32_t len = getDenseInitializedLength();
    const Value *elems = getDenseElements();
    for (uint32_t i = 0; i < len; i++) {
        if (!elems[i].isMagic(JS_ELEMENTS_HOLE) && !--minimalDenseCount)
            return false;
    }
    return true;
}

 JSObject::EnsureDenseResult
JSObject::maybeDensifySparseElements(js::ExclusiveContext *cx, HandleObject obj)
{
    




    if (!obj->inDictionaryMode())
        return ED_SPARSE;

    



    uint32_t slotSpan = obj->slotSpan();
    if (slotSpan != RoundUpPow2(slotSpan))
        return ED_SPARSE;

    
    if (!obj->nonProxyIsExtensible() || obj->watched())
        return ED_SPARSE;

    



    uint32_t numDenseElements = 0;
    uint32_t newInitializedLength = 0;

    RootedShape shape(cx, obj->lastProperty());
    while (!shape->isEmptyShape()) {
        uint32_t index;
        if (js_IdIsIndex(shape->propid(), &index)) {
            if (shape->attributes() == JSPROP_ENUMERATE &&
                shape->hasDefaultGetter() &&
                shape->hasDefaultSetter())
            {
                numDenseElements++;
                newInitializedLength = Max(newInitializedLength, index + 1);
            } else {
                



                return ED_SPARSE;
            }
        }
        shape = shape->previous();
    }

    if (numDenseElements * SPARSE_DENSITY_RATIO < newInitializedLength)
        return ED_SPARSE;

    if (newInitializedLength >= NELEMENTS_LIMIT)
        return ED_SPARSE;

    




    if (newInitializedLength > obj->getDenseCapacity()) {
        if (!obj->growElements(cx, newInitializedLength))
            return ED_FAILED;
    }

    obj->ensureDenseInitializedLength(cx, newInitializedLength, 0);

    RootedValue value(cx);

    shape = obj->lastProperty();
    while (!shape->isEmptyShape()) {
        jsid id = shape->propid();
        uint32_t index;
        if (js_IdIsIndex(id, &index)) {
            value = obj->getSlot(shape->slot());

            






            if (shape != obj->lastProperty()) {
                shape = shape->previous();
                if (!obj->removeProperty(cx, id))
                    return ED_FAILED;
            } else {
                if (!obj->removeProperty(cx, id))
                    return ED_FAILED;
                shape = obj->lastProperty();
            }

            obj->setDenseElement(index, value);
        } else {
            shape = shape->previous();
        }
    }

    




    if (!obj->clearFlag(cx, BaseShape::INDEXED))
        return ED_FAILED;

    return ED_OK;
}

static ObjectElements *
AllocateElements(ThreadSafeContext *cx, JSObject *obj, uint32_t nelems)
{
#ifdef JSGC_GENERATIONAL
    if (cx->isJSContext())
        return cx->asJSContext()->runtime()->gc.nursery.allocateElements(cx->asJSContext(), obj, nelems);
#endif

    return static_cast<js::ObjectElements *>(cx->malloc_(nelems * sizeof(HeapValue)));
}

static ObjectElements *
ReallocateElements(ThreadSafeContext *cx, JSObject *obj, ObjectElements *oldHeader,
                   uint32_t oldCount, uint32_t newCount)
{
#ifdef JSGC_GENERATIONAL
    if (cx->isJSContext()) {
        return cx->asJSContext()->runtime()->gc.nursery.reallocateElements(cx->asJSContext(), obj,
                                                                             oldHeader, oldCount,
                                                                             newCount);
    }
#endif

    return static_cast<js::ObjectElements *>(cx->realloc_(oldHeader, oldCount * sizeof(HeapSlot),
                                                          newCount * sizeof(HeapSlot)));
}

bool
JSObject::growElements(ThreadSafeContext *cx, uint32_t newcap)
{
    JS_ASSERT(nonProxyIsExtensible());
    JS_ASSERT(canHaveNonEmptyElements());

    






    static const size_t CAPACITY_DOUBLING_MAX = 1024 * 1024;
    static const size_t CAPACITY_CHUNK = CAPACITY_DOUBLING_MAX / sizeof(Value);

    uint32_t oldcap = getDenseCapacity();
    JS_ASSERT(oldcap <= newcap);

    uint32_t nextsize = (oldcap <= CAPACITY_DOUBLING_MAX)
                      ? oldcap * 2
                      : oldcap + (oldcap >> 3);

    uint32_t actualCapacity;
    if (is<ArrayObject>() && !as<ArrayObject>().lengthIsWritable()) {
        JS_ASSERT(newcap <= as<ArrayObject>().length());
        
        
        
        actualCapacity = newcap;
    } else {
        actualCapacity = Max(newcap, nextsize);
        if (actualCapacity >= CAPACITY_CHUNK)
            actualCapacity = JS_ROUNDUP(actualCapacity, CAPACITY_CHUNK);
        else if (actualCapacity < SLOT_CAPACITY_MIN)
            actualCapacity = SLOT_CAPACITY_MIN;

        
        if (actualCapacity >= NELEMENTS_LIMIT || actualCapacity < oldcap || actualCapacity < newcap)
            return false;
    }

    uint32_t initlen = getDenseInitializedLength();
    uint32_t oldAllocated = oldcap + ObjectElements::VALUES_PER_HEADER;
    uint32_t newAllocated = actualCapacity + ObjectElements::VALUES_PER_HEADER;

    ObjectElements *newheader;
    if (hasDynamicElements()) {
        newheader = ReallocateElements(cx, this, getElementsHeader(), oldAllocated, newAllocated);
        if (!newheader)
            return false; 
    } else {
        newheader = AllocateElements(cx, this, newAllocated);
        if (!newheader)
            return false; 
        js_memcpy(newheader, getElementsHeader(),
                  (ObjectElements::VALUES_PER_HEADER + initlen) * sizeof(Value));
    }

    newheader->capacity = actualCapacity;
    elements = newheader->elements();

    Debug_SetSlotRangeToCrashOnTouch(elements + initlen, actualCapacity - initlen);

    return true;
}

void
JSObject::shrinkElements(ThreadSafeContext *cx, uint32_t newcap)
{
    JS_ASSERT(cx->isThreadLocal(this));
    JS_ASSERT(canHaveNonEmptyElements());

    uint32_t oldcap = getDenseCapacity();
    JS_ASSERT(newcap <= oldcap);

    
    if (oldcap <= SLOT_CAPACITY_MIN || !hasDynamicElements())
        return;

    newcap = Max(newcap, SLOT_CAPACITY_MIN);

    uint32_t oldAllocated = oldcap + ObjectElements::VALUES_PER_HEADER;
    uint32_t newAllocated = newcap + ObjectElements::VALUES_PER_HEADER;

    ObjectElements *newheader = ReallocateElements(cx, this, getElementsHeader(),
                                                   oldAllocated, newAllocated);
    if (!newheader) {
        cx->recoverFromOutOfMemory();
        return;  
    }

    newheader->capacity = newcap;
    elements = newheader->elements();
}

bool
js::SetClassAndProto(JSContext *cx, HandleObject obj,
                     const Class *clasp, Handle<js::TaggedProto> proto,
                     bool *succeeded)
{
    


















    *succeeded = false;
    RootedObject oldproto(cx, obj);
    while (oldproto && oldproto->isNative()) {
        if (oldproto->hasSingletonType()) {
            if (!oldproto->generateOwnShape(cx))
                return false;
        } else {
            if (!oldproto->setUncacheableProto(cx))
                return false;
        }
        oldproto = oldproto->getProto();
    }

    if (obj->hasSingletonType()) {
        



        if (!obj->splicePrototype(cx, clasp, proto))
            return false;
        MarkTypeObjectUnknownProperties(cx, obj->type());
        *succeeded = true;
        return true;
    }

    if (proto.isObject()) {
        RootedObject protoObj(cx, proto.toObject());
        if (!JSObject::setNewTypeUnknown(cx, clasp, protoObj))
            return false;
    }

    TypeObject *type = cx->getNewType(clasp, proto);
    if (!type)
        return false;

    







    MarkTypeObjectUnknownProperties(cx, obj->type(), true);
    MarkTypeObjectUnknownProperties(cx, type, true);

    obj->setType(type);

    *succeeded = true;
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
    
    JS_ASSERT(!obj->is<CrossCompartmentWrapperObject>());
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(obj->getClass());
    if (key != JSProto_Null && !IsStandardPrototype(obj, key))
        return key;
    return JSProto_Null;
}

JSProtoKey
JS::IdentifyStandardPrototype(JSObject *obj)
{
    
    JS_ASSERT(!obj->is<CrossCompartmentWrapperObject>());
    JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(obj->getClass());
    if (key != JSProto_Null && IsStandardPrototype(obj, key))
        return key;
    return JSProto_Null;
}

JSProtoKey
JS::IdentifyStandardInstanceOrPrototype(JSObject *obj)
{
    return JSCLASS_CACHED_PROTO_KEY(obj->getClass());
}

bool
js::FindClassObject(ExclusiveContext *cx, MutableHandleObject protop, const Class *clasp)
{
    JSProtoKey protoKey = GetClassProtoKey(clasp);
    if (protoKey != JSProto_Null) {
        JS_ASSERT(JSProto_Null < protoKey);
        JS_ASSERT(protoKey < JSProto_LIMIT);
        return GetBuiltinConstructor(cx, protoKey, protop);
    }

    JSAtom *atom = Atomize(cx, clasp->name, strlen(clasp->name));
    if (!atom)
        return false;
    RootedId id(cx, AtomToId(atom));

    RootedObject pobj(cx);
    RootedShape shape(cx);
    if (!LookupNativeProperty(cx, cx->global(), id, &pobj, &shape))
        return false;
    RootedValue v(cx);
    if (shape && pobj->isNative()) {
        if (shape->hasSlot())
            v = pobj->nativeGetSlot(shape->slot());
    }
    if (v.isObject())
        protop.set(&v.toObject());
    return true;
}

bool
JSObject::isConstructor() const
{
    if (is<JSFunction>()) {
        const JSFunction &fun = as<JSFunction>();
        return fun.isNativeConstructor() || fun.isInterpretedConstructor();
    }
    return getClass()->construct != nullptr;
}

 bool
JSObject::allocSlot(ThreadSafeContext *cx, HandleObject obj, uint32_t *slotp)
{
    JS_ASSERT(cx->isThreadLocal(obj));

    uint32_t slot = obj->slotSpan();
    JS_ASSERT(slot >= JSSLOT_FREE(obj->getClass()));

    



    if (obj->inDictionaryMode()) {
        ShapeTable &table = obj->lastProperty()->table();
        uint32_t last = table.freelist;
        if (last != SHAPE_INVALID_SLOT) {
#ifdef DEBUG
            JS_ASSERT(last < slot);
            uint32_t next = obj->getSlot(last).toPrivateUint32();
            JS_ASSERT_IF(next != SHAPE_INVALID_SLOT, next < slot);
#endif

            *slotp = last;

            const Value &vref = obj->getSlot(last);
            table.freelist = vref.toPrivateUint32();
            obj->setSlot(last, UndefinedValue());
            return true;
        }
    }

    if (slot >= SHAPE_MAXIMUM_SLOT) {
        js_ReportOutOfMemory(cx);
        return false;
    }

    *slotp = slot;

    if (obj->inDictionaryMode() && !setSlotSpan(cx, obj, slot + 1))
        return false;

    return true;
}

void
JSObject::freeSlot(uint32_t slot)
{
    JS_ASSERT(slot < slotSpan());

    if (inDictionaryMode()) {
        uint32_t &last = lastProperty()->table().freelist;

        
        JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan() && last != slot);

        



        if (JSSLOT_FREE(getClass()) <= slot) {
            JS_ASSERT_IF(last != SHAPE_INVALID_SLOT, last < slotSpan());
            setSlot(slot, PrivateUint32Value(last));
            last = slot;
            return;
        }
    }
    setSlot(slot, UndefinedValue());
}

static bool
PurgeProtoChain(ExclusiveContext *cx, JSObject *objArg, HandleId id)
{
    
    RootedObject obj(cx, objArg);

    RootedShape shape(cx);
    while (obj) {
        
        if (!obj->isNative())
            break;

        shape = obj->nativeLookup(cx, id);
        if (shape) {
            if (!obj->shadowingShapeChange(cx, *shape))
                return false;

            obj->shadowingShapeChange(cx, *shape);
            return true;
        }
        obj = obj->getProto();
    }

    return true;
}

static bool
PurgeScopeChainHelper(ExclusiveContext *cx, HandleObject objArg, HandleId id)
{
    
    RootedObject obj(cx, objArg);

    JS_ASSERT(obj->isNative());
    JS_ASSERT(obj->isDelegate());

    
    if (JSID_IS_INT(id))
        return true;

    PurgeProtoChain(cx, obj->getProto(), id);

    





    if (obj->is<CallObject>()) {
        while ((obj = obj->enclosingScope()) != nullptr) {
            if (!PurgeProtoChain(cx, obj, id))
                return false;
        }
    }

    return true;
}







static inline bool
PurgeScopeChain(ExclusiveContext *cx, JS::HandleObject obj, JS::HandleId id)
{
    if (obj->isDelegate())
        return PurgeScopeChainHelper(cx, obj, id);
    return true;
}

bool
baseops::DefineGeneric(ExclusiveContext *cx, HandleObject obj, HandleId id, HandleValue value,
                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs);
}

 bool
JSObject::defineGeneric(ExclusiveContext *cx, HandleObject obj,
                        HandleId id, HandleValue value,
                        JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    JS_ASSERT(!(attrs & JSPROP_NATIVE_ACCESSORS));
    js::DefineGenericOp op = obj->getOps()->defineGeneric;
    if (op) {
        if (!cx->shouldBeJSContext())
            return false;
        return op(cx->asJSContext(), obj, id, value, getter, setter, attrs);
    }
    return baseops::DefineGeneric(cx, obj, id, value, getter, setter, attrs);
}

 bool
JSObject::defineProperty(ExclusiveContext *cx, HandleObject obj,
                         PropertyName *name, HandleValue value,
                         JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    RootedId id(cx, NameToId(name));
    return defineGeneric(cx, obj, id, value, getter, setter, attrs);
}

bool
baseops::DefineElement(ExclusiveContext *cx, HandleObject obj, uint32_t index, HandleValue value,
                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    RootedId id(cx);
    if (index <= JSID_INT_MAX) {
        id = INT_TO_JSID(index);
        return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs);
    }

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    if (!IndexToId(cx, index, &id))
        return false;

    return DefineNativeProperty(cx, obj, id, value, getter, setter, attrs);
}

 bool
JSObject::defineElement(ExclusiveContext *cx, HandleObject obj,
                        uint32_t index, HandleValue value,
                        JSPropertyOp getter, JSStrictPropertyOp setter, unsigned attrs)
{
    js::DefineElementOp op = obj->getOps()->defineElement;
    if (op) {
        if (!cx->shouldBeJSContext())
            return false;
        return op(cx->asJSContext(), obj, index, value, getter, setter, attrs);
    }
    return baseops::DefineElement(cx, obj, index, value, getter, setter, attrs);
}

Shape *
JSObject::addDataProperty(ExclusiveContext *cx, jsid idArg, uint32_t slot, unsigned attrs)
{
    JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    RootedObject self(cx, this);
    RootedId id(cx, idArg);
    return addProperty(cx, self, id, nullptr, nullptr, slot, attrs, 0);
}

Shape *
JSObject::addDataProperty(ExclusiveContext *cx, HandlePropertyName name,
                          uint32_t slot, unsigned attrs)
{
    JS_ASSERT(!(attrs & (JSPROP_GETTER | JSPROP_SETTER)));
    RootedObject self(cx, this);
    RootedId id(cx, NameToId(name));
    return addProperty(cx, self, id, nullptr, nullptr, slot, attrs, 0);
}







template <ExecutionMode mode>
static inline bool
CallAddPropertyHook(typename ExecutionModeTraits<mode>::ExclusiveContextType cxArg,
                    const Class *clasp, HandleObject obj, HandleShape shape,
                    HandleValue nominal)
{
    if (clasp->addProperty != JS_PropertyStub) {
        if (mode == ParallelExecution)
            return false;

        ExclusiveContext *cx = cxArg->asExclusiveContext();
        if (!cx->shouldBeJSContext())
            return false;

        
        RootedValue value(cx, nominal);

        Rooted<jsid> id(cx, shape->propid());
        if (!CallJSPropertyOp(cx->asJSContext(), clasp->addProperty, obj, id, &value)) {
            obj->removeProperty(cx, shape->propid());
            return false;
        }
        if (value.get() != nominal) {
            if (shape->hasSlot())
                obj->nativeSetSlotWithType(cx, shape, value);
        }
    }
    return true;
}

template <ExecutionMode mode>
static inline bool
CallAddPropertyHookDense(typename ExecutionModeTraits<mode>::ExclusiveContextType cxArg,
                         const Class *clasp, HandleObject obj, uint32_t index,
                         HandleValue nominal)
{
    
    if (obj->is<ArrayObject>()) {
        ArrayObject *arr = &obj->as<ArrayObject>();
        uint32_t length = arr->length();
        if (index >= length) {
            if (mode == ParallelExecution) {
                
                if (length > INT32_MAX)
                    return false;
                arr->setLengthInt32(index + 1);
            } else {
                arr->setLength(cxArg->asExclusiveContext(), index + 1);
            }
        }
        return true;
    }

    if (clasp->addProperty != JS_PropertyStub) {
        if (mode == ParallelExecution)
            return false;

        ExclusiveContext *cx = cxArg->asExclusiveContext();
        if (!cx->shouldBeJSContext())
            return false;

        
        RootedValue value(cx, nominal);

        Rooted<jsid> id(cx, INT_TO_JSID(index));
        if (!CallJSPropertyOp(cx->asJSContext(), clasp->addProperty, obj, id, &value)) {
            obj->setDenseElementHole(cx, index);
            return false;
        }
        if (value.get() != nominal)
            obj->setDenseElementWithType(cx, index, value);
    }

    return true;
}

template <ExecutionMode mode>
static bool
UpdateShapeTypeAndValue(typename ExecutionModeTraits<mode>::ExclusiveContextType cx,
                        JSObject *obj, Shape *shape, const Value &value)
{
    jsid id = shape->propid();
    if (shape->hasSlot()) {
        if (mode == ParallelExecution) {
            if (!obj->nativeSetSlotIfHasType(shape, value))
                return false;
        } else {
            obj->nativeSetSlotWithType(cx->asExclusiveContext(), shape, value);
        }
    }
    if (!shape->hasSlot() || !shape->hasDefaultGetter() || !shape->hasDefaultSetter()) {
        if (mode == ParallelExecution) {
            if (!IsTypePropertyIdMarkedNonData(obj, id))
                return false;
        } else {
            MarkTypePropertyNonData(cx->asExclusiveContext(), obj, id);
        }
    }
    if (!shape->writable()) {
        if (mode == ParallelExecution) {
            if (!IsTypePropertyIdMarkedNonWritable(obj, id))
                return false;
        } else {
            MarkTypePropertyNonWritable(cx->asExclusiveContext(), obj, id);
        }
    }
    return true;
}

template <ExecutionMode mode>
static inline bool
DefinePropertyOrElement(typename ExecutionModeTraits<mode>::ExclusiveContextType cx,
                        HandleObject obj, HandleId id,
                        PropertyOp getter, StrictPropertyOp setter,
                        unsigned attrs, HandleValue value,
                        bool callSetterAfterwards, bool setterIsStrict)
{
    
    if (JSID_IS_INT(id) &&
        getter == JS_PropertyStub &&
        setter == JS_StrictPropertyStub &&
        attrs == JSPROP_ENUMERATE &&
        (!obj->isIndexed() || !obj->nativeContainsPure(id)) &&
        !obj->is<TypedArrayObject>())
    {
        uint32_t index = JSID_TO_INT(id);
        bool definesPast;
        if (!WouldDefinePastNonwritableLength(cx, obj, index, setterIsStrict, &definesPast))
            return false;
        if (definesPast)
            return true;

        JSObject::EnsureDenseResult result;
        if (mode == ParallelExecution) {
            if (obj->writeToIndexWouldMarkNotPacked(index))
                return false;
            result = obj->ensureDenseElementsPreservePackedFlag(cx, index, 1);
        } else {
            result = obj->ensureDenseElements(cx->asExclusiveContext(), index, 1);
        }

        if (result == JSObject::ED_FAILED)
            return false;
        if (result == JSObject::ED_OK) {
            if (mode == ParallelExecution) {
                if (!obj->setDenseElementIfHasType(index, value))
                    return false;
            } else {
                obj->setDenseElementWithType(cx->asExclusiveContext(), index, value);
            }
            return CallAddPropertyHookDense<mode>(cx, obj->getClass(), obj, index, value);
        }
    }

    if (obj->is<ArrayObject>()) {
        Rooted<ArrayObject*> arr(cx, &obj->as<ArrayObject>());
        if (id == NameToId(cx->names().length)) {
            if (mode == SequentialExecution && !cx->shouldBeJSContext())
                return false;
            return ArraySetLength<mode>(ExecutionModeTraits<mode>::toContextType(cx), arr, id,
                                        attrs, value, setterIsStrict);
        }

        uint32_t index;
        if (js_IdIsIndex(id, &index)) {
            bool definesPast;
            if (!WouldDefinePastNonwritableLength(cx, arr, index, setterIsStrict, &definesPast))
                return false;
            if (definesPast)
                return true;
        }
    }

    
    if (obj->is<TypedArrayObject>()) {
        uint64_t index;
        if (IsTypedArrayIndex(id, &index))
            return true;
    }

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    RootedShape shape(cx, JSObject::putProperty<mode>(cx, obj, id, getter, setter,
                                                      SHAPE_INVALID_SLOT, attrs, 0));
    if (!shape)
        return false;

    if (!UpdateShapeTypeAndValue<mode>(cx, obj, shape, value))
        return false;

    



    if (JSID_IS_INT(id)) {
        if (mode == ParallelExecution)
            return false;

        ExclusiveContext *ncx = cx->asExclusiveContext();
        uint32_t index = JSID_TO_INT(id);
        JSObject::removeDenseElementForSparseIndex(ncx, obj, index);
        JSObject::EnsureDenseResult result = JSObject::maybeDensifySparseElements(ncx, obj);
        if (result == JSObject::ED_FAILED)
            return false;
        if (result == JSObject::ED_OK) {
            JS_ASSERT(setter == JS_StrictPropertyStub);
            return CallAddPropertyHookDense<mode>(cx, obj->getClass(), obj, index, value);
        }
    }

    if (!CallAddPropertyHook<mode>(cx, obj->getClass(), obj, shape, value))
        return false;

    if (callSetterAfterwards && setter != JS_StrictPropertyStub) {
        if (!cx->shouldBeJSContext())
            return false;
        RootedValue nvalue(cx, value);
        return NativeSet<mode>(ExecutionModeTraits<mode>::toContextType(cx),
                               obj, obj, shape, setterIsStrict, &nvalue);
    }
    return true;
}

static bool
NativeLookupOwnProperty(ExclusiveContext *cx, HandleObject obj, HandleId id,
                        MutableHandle<Shape*> shapep);

bool
js::DefineNativeProperty(ExclusiveContext *cx, HandleObject obj, HandleId id, HandleValue value,
                         PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    JS_ASSERT(!(attrs & JSPROP_NATIVE_ACCESSORS));

    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    




    RootedShape shape(cx);
    if (attrs & (JSPROP_GETTER | JSPROP_SETTER)) {
        



        if (!NativeLookupOwnProperty(cx, obj, id, &shape))
            return false;
        if (shape) {
            if (IsImplicitDenseOrTypedArrayElement(shape)) {
                if (obj->is<TypedArrayObject>()) {
                    
                    return true;
                }
                if (!JSObject::sparsifyDenseElement(cx, obj, JSID_TO_INT(id)))
                    return false;
                shape = obj->nativeLookup(cx, id);
            }
            if (shape->isAccessorDescriptor()) {
                shape = JSObject::changeProperty<SequentialExecution>(cx, obj, shape, attrs,
                                                                      JSPROP_GETTER | JSPROP_SETTER,
                                                                      (attrs & JSPROP_GETTER)
                                                                      ? getter
                                                                      : shape->getter(),
                                                                      (attrs & JSPROP_SETTER)
                                                                      ? setter
                                                                      : shape->setter());
                if (!shape)
                    return false;
            } else {
                shape = nullptr;
            }
        }
    }

    



    if (!PurgeScopeChain(cx, obj, id))
        return false;

    
    const Class *clasp = obj->getClass();
    if (!getter && !(attrs & JSPROP_GETTER))
        getter = clasp->getProperty;
    if (!setter && !(attrs & JSPROP_SETTER))
        setter = clasp->setProperty;

    if (!shape) {
        return DefinePropertyOrElement<SequentialExecution>(cx, obj, id, getter, setter,
                                                            attrs, value, false, false);
    }

    JS_ALWAYS_TRUE(UpdateShapeTypeAndValue<SequentialExecution>(cx, obj, shape, value));

    return CallAddPropertyHook<SequentialExecution>(cx, clasp, obj, shape, value);
}





















static MOZ_ALWAYS_INLINE bool
CallResolveOp(JSContext *cx, HandleObject obj, HandleId id, MutableHandleObject objp,
              MutableHandleShape propp, bool *recursedp)
{
    const Class *clasp = obj->getClass();
    JSResolveOp resolve = clasp->resolve;

    







    AutoResolving resolving(cx, obj, id);
    if (resolving.alreadyStarted()) {
        
        *recursedp = true;
        return true;
    }
    *recursedp = false;

    propp.set(nullptr);

    if (clasp->flags & JSCLASS_NEW_RESOLVE) {
        JSNewResolveOp newresolve = reinterpret_cast<JSNewResolveOp>(resolve);
        RootedObject obj2(cx, nullptr);
        if (!newresolve(cx, obj, id, &obj2))
            return false;

        





        if (!obj2)
            return true;

        if (!obj2->isNative()) {
            
            JS_ASSERT(obj2 != obj);
            return JSObject::lookupGeneric(cx, obj2, id, objp, propp);
        }

        objp.set(obj2);
    } else {
        if (!resolve(cx, obj, id))
            return false;

        objp.set(obj);
    }

    if (JSID_IS_INT(id) && objp->containsDenseElement(JSID_TO_INT(id))) {
        MarkDenseOrTypedArrayElementFound<CanGC>(propp);
        return true;
    }

    Shape *shape;
    if (!objp->nativeEmpty() && (shape = objp->nativeLookup(cx, id)))
        propp.set(shape);
    else
        objp.set(nullptr);

    return true;
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
LookupOwnPropertyInline(ExclusiveContext *cx,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                        typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp,
                        bool *donep)
{
    
    if (JSID_IS_INT(id) && obj->containsDenseElement(JSID_TO_INT(id))) {
        objp.set(obj);
        MarkDenseOrTypedArrayElementFound<allowGC>(propp);
        *donep = true;
        return true;
    }

    
    
    
    if (obj->template is<TypedArrayObject>()) {
        uint64_t index;
        if (IsTypedArrayIndex(id, &index)) {
            if (index < obj->template as<TypedArrayObject>().length()) {
                objp.set(obj);
                MarkDenseOrTypedArrayElementFound<allowGC>(propp);
            } else {
                objp.set(nullptr);
                propp.set(nullptr);
            }
            *donep = true;
            return true;
        }
    }

    
    if (Shape *shape = obj->nativeLookup(cx, id)) {
        objp.set(obj);
        propp.set(shape);
        *donep = true;
        return true;
    }

    
    if (obj->getClass()->resolve != JS_ResolveStub) {
        if (!cx->shouldBeJSContext() || !allowGC)
            return false;

        bool recursed;
        if (!CallResolveOp(cx->asJSContext(),
                           MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                           MaybeRooted<jsid, allowGC>::toHandle(id),
                           MaybeRooted<JSObject*, allowGC>::toMutableHandle(objp),
                           MaybeRooted<Shape*, allowGC>::toMutableHandle(propp),
                           &recursed))
        {
            return false;
        }

        if (recursed) {
            objp.set(nullptr);
            propp.set(nullptr);
            *donep = true;
            return true;
        }

        if (propp) {
            *donep = true;
            return true;
        }
    }

    *donep = false;
    return true;
}

static bool
NativeLookupOwnProperty(ExclusiveContext *cx, HandleObject obj, HandleId id,
                        MutableHandle<Shape*> shapep)
{
    RootedObject pobj(cx);
    bool done;

    if (!LookupOwnPropertyInline<CanGC>(cx, obj, id, &pobj, shapep, &done))
        return false;
    if (!done || pobj != obj)
        shapep.set(nullptr);
    return true;
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
LookupPropertyInline(ExclusiveContext *cx,
                     typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                     typename MaybeRooted<jsid, allowGC>::HandleType id,
                     typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                     typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    




    
    typename MaybeRooted<JSObject*, allowGC>::RootType current(cx, obj);

    while (true) {
        bool done;
        if (!LookupOwnPropertyInline<allowGC>(cx, current, id, objp, propp, &done))
            return false;
        if (done)
            return true;

        typename MaybeRooted<JSObject*, allowGC>::RootType proto(cx, current->getProto());

        if (!proto)
            break;
        if (!proto->isNative()) {
            if (!cx->shouldBeJSContext() || !allowGC)
                return false;
            return JSObject::lookupGeneric(cx->asJSContext(),
                                           MaybeRooted<JSObject*, allowGC>::toHandle(proto),
                                           MaybeRooted<jsid, allowGC>::toHandle(id),
                                           MaybeRooted<JSObject*, allowGC>::toMutableHandle(objp),
                                           MaybeRooted<Shape*, allowGC>::toMutableHandle(propp));
        }

        current = proto;
    }

    objp.set(nullptr);
    propp.set(nullptr);
    return true;
}

template <AllowGC allowGC>
bool
baseops::LookupProperty(ExclusiveContext *cx,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                        typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    return LookupPropertyInline<allowGC>(cx, obj, id, objp, propp);
}

template bool
baseops::LookupProperty<CanGC>(ExclusiveContext *cx, HandleObject obj, HandleId id,
                               MutableHandleObject objp, MutableHandleShape propp);

template bool
baseops::LookupProperty<NoGC>(ExclusiveContext *cx, JSObject *obj, jsid id,
                              FakeMutableHandle<JSObject*> objp,
                              FakeMutableHandle<Shape*> propp);

 bool
JSObject::lookupGeneric(JSContext *cx, HandleObject obj, js::HandleId id,
                        MutableHandleObject objp, MutableHandleShape propp)
{
    



    LookupGenericOp op = obj->getOps()->lookupGeneric;
    if (op)
        return op(cx, obj, id, objp, propp);
    return baseops::LookupProperty<js::CanGC>(cx, obj, id, objp, propp);
}

bool
baseops::LookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                       MutableHandleObject objp, MutableHandleShape propp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;

    return LookupPropertyInline<CanGC>(cx, obj, id, objp, propp);
}

bool
js::LookupNativeProperty(ExclusiveContext *cx, HandleObject obj, HandleId id,
                         MutableHandleObject objp, MutableHandleShape propp)
{
    return LookupPropertyInline<CanGC>(cx, obj, id, objp, propp);
}

bool
js::LookupName(JSContext *cx, HandlePropertyName name, HandleObject scopeChain,
               MutableHandleObject objp, MutableHandleObject pobjp, MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));

    for (RootedObject scope(cx, scopeChain); scope; scope = scope->enclosingScope()) {
        if (!JSObject::lookupGeneric(cx, scope, id, pobjp, propp))
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

    JS_ASSERT(!*objp && !*pobjp && !*propp);

    for (JSObject *scope = scopeChain; scope; scope = scope->enclosingScope()) {
        if (scope->getOps()->lookupGeneric)
            return false;
        if (!LookupPropertyInline<NoGC>(cx, scope, NameToId(name), pobjp, propp))
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
    RootedShape prop(cx);

    RootedObject scope(cx, scopeChain);
    for (; !scope->is<GlobalObject>(); scope = scope->enclosingScope()) {
        if (!JSObject::lookupGeneric(cx, scope, id, &pobj, &prop))
            return false;
        if (prop)
            break;
    }

    objp.set(scope);
    return true;
}

template <AllowGC allowGC>
bool
js::HasOwnProperty(JSContext *cx, LookupGenericOp lookup,
                   typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                   typename MaybeRooted<jsid, allowGC>::HandleType id,
                   typename MaybeRooted<JSObject*, allowGC>::MutableHandleType objp,
                   typename MaybeRooted<Shape*, allowGC>::MutableHandleType propp)
{
    if (lookup) {
        if (!allowGC)
            return false;
        if (!lookup(cx,
                    MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                    MaybeRooted<jsid, allowGC>::toHandle(id),
                    MaybeRooted<JSObject*, allowGC>::toMutableHandle(objp),
                    MaybeRooted<Shape*, allowGC>::toMutableHandle(propp)))
        {
            return false;
        }
    } else {
        bool done;
        if (!LookupOwnPropertyInline<allowGC>(cx, obj, id, objp, propp, &done))
            return false;
        if (!done) {
            objp.set(nullptr);
            propp.set(nullptr);
            return true;
        }
    }

    if (!propp)
        return true;

    if (objp == obj)
        return true;

    JSObject *outer = nullptr;
    if (js::ObjectOp op = objp->getClass()->ext.outerObject) {
        if (!allowGC)
            return false;
        RootedObject inner(cx, objp);
        outer = op(cx, inner);
        if (!outer)
            return false;
    }

    if (outer != objp)
        propp.set(nullptr);
    return true;
}

template bool
js::HasOwnProperty<CanGC>(JSContext *cx, LookupGenericOp lookup,
                          HandleObject obj, HandleId id,
                          MutableHandleObject objp, MutableHandleShape propp);

template bool
js::HasOwnProperty<NoGC>(JSContext *cx, LookupGenericOp lookup,
                         JSObject *obj, jsid id,
                         FakeMutableHandle<JSObject*> objp, FakeMutableHandle<Shape*> propp);

bool
js::HasOwnProperty(JSContext *cx, HandleObject obj, HandleId id, bool *resultp)
{
    RootedObject pobj(cx);
    RootedShape shape(cx);
    if (!HasOwnProperty<CanGC>(cx, obj->getOps()->lookupGeneric, obj, id, &pobj, &shape))
        return false;
    *resultp = (shape != nullptr);
    return true;
}

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
NativeGetInline(JSContext *cx,
                typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                typename MaybeRooted<JSObject*, allowGC>::HandleType receiver,
                typename MaybeRooted<JSObject*, allowGC>::HandleType pobj,
                typename MaybeRooted<Shape*, allowGC>::HandleType shape,
                typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    JS_ASSERT(pobj->isNative());

    if (shape->hasSlot()) {
        vp.set(pobj->nativeGetSlot(shape->slot()));
        JS_ASSERT(!vp.isMagic());
        JS_ASSERT_IF(!pobj->hasSingletonType() &&
                     !pobj->template is<ScopeObject>() &&
                     shape->hasDefaultGetter(),
                     js::types::TypeHasProperty(cx, pobj->type(), shape->propid(), vp));
    } else {
        vp.setUndefined();
    }
    if (shape->hasDefaultGetter())
        return true;

    {
        jsbytecode *pc;
        JSScript *script = cx->currentScript(&pc);
#ifdef JS_ION
        if (script && script->hasBaselineScript()) {
            switch (JSOp(*pc)) {
              case JSOP_GETPROP:
              case JSOP_CALLPROP:
              case JSOP_LENGTH:
                script->baselineScript()->noteAccessedGetter(script->pcToOffset(pc));
                break;
              default:
                break;
            }
        }
#endif
    }

    if (!allowGC)
        return false;

    if (!shape->get(cx,
                    MaybeRooted<JSObject*, allowGC>::toHandle(receiver),
                    MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                    MaybeRooted<JSObject*, allowGC>::toHandle(pobj),
                    MaybeRooted<Value, allowGC>::toMutableHandle(vp)))
    {
        return false;
    }

    
    if (shape->hasSlot() && pobj->nativeContains(cx, shape))
        pobj->nativeSetSlot(shape->slot(), vp);

    return true;
}

bool
js::NativeGet(JSContext *cx, Handle<JSObject*> obj, Handle<JSObject*> pobj, Handle<Shape*> shape,
              MutableHandle<Value> vp)
{
    return NativeGetInline<CanGC>(cx, obj, obj, pobj, shape, vp);
}

template <ExecutionMode mode>
bool
js::NativeSet(typename ExecutionModeTraits<mode>::ContextType cxArg,
              Handle<JSObject*> obj, Handle<JSObject*> receiver,
              HandleShape shape, bool strict, MutableHandleValue vp)
{
    JS_ASSERT(cxArg->isThreadLocal(obj));
    JS_ASSERT(obj->isNative());

    if (shape->hasSlot()) {
        
        if (shape->hasDefaultSetter()) {
            if (mode == ParallelExecution) {
                if (!obj->nativeSetSlotIfHasType(shape, vp))
                    return false;
            } else {
                obj->nativeSetSlotWithType(cxArg->asExclusiveContext(), shape, vp);
            }

            return true;
        }
    }

    if (mode == ParallelExecution)
        return false;
    JSContext *cx = cxArg->asJSContext();

    if (!shape->hasSlot()) {
        





        if (!shape->hasGetterValue() && shape->hasDefaultSetter())
            return js_ReportGetterOnlyAssignment(cx, strict);
    }

    RootedValue ovp(cx, vp);

    uint32_t sample = cx->runtime()->propertyRemovals;
    if (!shape->set(cx, obj, receiver, strict, vp))
        return false;

    



    if (shape->hasSlot() &&
        (MOZ_LIKELY(cx->runtime()->propertyRemovals == sample) ||
         obj->nativeContains(cx, shape)))
    {
        obj->setSlot(shape->slot(), vp);
    }

    return true;
}

template bool
js::NativeSet<SequentialExecution>(JSContext *cx,
                                   Handle<JSObject*> obj, Handle<JSObject*> receiver,
                                   HandleShape shape, bool strict, MutableHandleValue vp);
template bool
js::NativeSet<ParallelExecution>(ForkJoinContext *cx,
                                 Handle<JSObject*> obj, Handle<JSObject*> receiver,
                                 HandleShape shape, bool strict, MutableHandleValue vp);

template <AllowGC allowGC>
static MOZ_ALWAYS_INLINE bool
GetPropertyHelperInline(JSContext *cx,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType obj,
                        typename MaybeRooted<JSObject*, allowGC>::HandleType receiver,
                        typename MaybeRooted<jsid, allowGC>::HandleType id,
                        typename MaybeRooted<Value, allowGC>::MutableHandleType vp)
{
    
    typename MaybeRooted<JSObject*, allowGC>::RootType obj2(cx);
    typename MaybeRooted<Shape*, allowGC>::RootType shape(cx);
    if (!LookupPropertyInline<allowGC>(cx, obj, id, &obj2, &shape))
        return false;

    if (!shape) {
        if (!allowGC)
            return false;

        vp.setUndefined();

        if (!CallJSPropertyOp(cx, obj->getClass()->getProperty,
                              MaybeRooted<JSObject*, allowGC>::toHandle(obj),
                              MaybeRooted<jsid, allowGC>::toHandle(id),
                              MaybeRooted<Value, allowGC>::toMutableHandle(vp)))
        {
            return false;
        }

        



        if (vp.isUndefined()) {
            jsbytecode *pc = nullptr;
            RootedScript script(cx, cx->currentScript(&pc));
            if (!pc)
                return true;
            JSOp op = (JSOp) *pc;

            if (op == JSOP_GETXPROP) {
                
                JSAutoByteString printable;
                if (js_ValueToPrintable(cx, IdToValue(id), &printable))
                    js_ReportIsNotDefined(cx, printable.ptr());
                return false;
            }

            
            if (!cx->options().extraWarnings() || (op != JSOP_GETPROP && op != JSOP_GETELEM))
                return true;

            
            if (!script || script->warnedAboutUndefinedProp())
                return true;

            




            if (script->selfHosted())
                return true;

            
            if (JSID_IS_ATOM(id, cx->names().iteratorIntrinsic))
                return true;

            
            pc += js_CodeSpec[op].length;
            if (Detecting(cx, script, pc))
                return true;

            unsigned flags = JSREPORT_WARNING | JSREPORT_STRICT;
            script->setWarnedAboutUndefinedProp();

            
            RootedValue val(cx, IdToValue(id));
            if (!js_ReportValueErrorFlags(cx, flags, JSMSG_UNDEFINED_PROP,
                                          JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                          nullptr, nullptr))
            {
                return false;
            }
        }
        return true;
    }

    if (!obj2->isNative()) {
        if (!allowGC)
            return false;
        HandleObject obj2Handle = MaybeRooted<JSObject*, allowGC>::toHandle(obj2);
        HandleObject receiverHandle = MaybeRooted<JSObject*, allowGC>::toHandle(receiver);
        HandleId idHandle = MaybeRooted<jsid, allowGC>::toHandle(id);
        MutableHandleValue vpHandle = MaybeRooted<Value, allowGC>::toMutableHandle(vp);
        return obj2->template is<ProxyObject>()
               ? Proxy::get(cx, obj2Handle, receiverHandle, idHandle, vpHandle)
               : JSObject::getGeneric(cx, obj2Handle, obj2Handle, idHandle, vpHandle);
    }

    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        vp.set(obj2->getDenseOrTypedArrayElement(JSID_TO_INT(id)));
        return true;
    }

    
    if (!NativeGetInline<allowGC>(cx, obj, receiver, obj2, shape, vp))
        return false;

    return true;
}

bool
baseops::GetProperty(JSContext *cx, HandleObject obj, HandleObject receiver, HandleId id, MutableHandleValue vp)
{
    
    return GetPropertyHelperInline<CanGC>(cx, obj, receiver, id, vp);
}

bool
baseops::GetPropertyNoGC(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    AutoAssertNoException nogc(cx);
    return GetPropertyHelperInline<NoGC>(cx, obj, receiver, id, vp);
}

static MOZ_ALWAYS_INLINE bool
LookupPropertyPureInline(JSObject *obj, jsid id, JSObject **objp, Shape **propp)
{
    if (!obj->isNative())
        return false;

    JSObject *current = obj;
    while (true) {
        

        if (JSID_IS_INT(id) && current->containsDenseElement(JSID_TO_INT(id))) {
            *objp = current;
            MarkDenseOrTypedArrayElementFound<NoGC>(propp);
            return true;
        }

        if (current->is<TypedArrayObject>()) {
            uint64_t index;
            if (IsTypedArrayIndex(id, &index)) {
                if (index < obj->as<TypedArrayObject>().length()) {
                    *objp = current;
                    MarkDenseOrTypedArrayElementFound<NoGC>(propp);
                } else {
                    *objp = nullptr;
                    *propp = nullptr;
                }
                return true;
            }
        }

        if (Shape *shape = current->nativeLookupPure(id)) {
            *objp = current;
            *propp = shape;
            return true;
        }

        
        if (current->getClass()->resolve != JS_ResolveStub)
            return false;

        JSObject *proto = current->getProto();

        if (!proto)
            break;
        if (!proto->isNative())
            return false;

        current = proto;
    }

    *objp = nullptr;
    *propp = nullptr;
    return true;
}

static MOZ_ALWAYS_INLINE bool
NativeGetPureInline(JSObject *pobj, Shape *shape, Value *vp)
{
    JS_ASSERT(pobj->isNative());

    if (shape->hasSlot()) {
        *vp = pobj->nativeGetSlot(shape->slot());
        JS_ASSERT(!vp->isMagic());
    } else {
        vp->setUndefined();
    }

    
    return shape->hasDefaultGetter();
}

bool
js::LookupPropertyPure(JSObject *obj, jsid id, JSObject **objp, Shape **propp)
{
    return LookupPropertyPureInline(obj, id, objp, propp);
}

static inline bool
IdIsLength(ThreadSafeContext *cx, jsid id)
{
    return JSID_IS_ATOM(id) && cx->names().length == JSID_TO_ATOM(id);
}











bool
js::GetPropertyPure(ThreadSafeContext *cx, JSObject *obj, jsid id, Value *vp)
{
    
    JSObject *obj2;
    Shape *shape;
    if (!LookupPropertyPureInline(obj, id, &obj2, &shape))
        return false;

    if (!shape) {
        
        if (obj->getClass()->getProperty && obj->getClass()->getProperty != JS_PropertyStub)
            return false;

        if (obj->getOps()->getElement)
            return false;

        
        vp->setUndefined();
        return true;
    }

    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        *vp = obj2->getDenseOrTypedArrayElement(JSID_TO_INT(id));
        return true;
    }

    
    if (IdIsLength(cx, id)) {
        if (obj->is<ArrayObject>()) {
            vp->setNumber(obj->as<ArrayObject>().length());
            return true;
        }

        if (obj->is<TypedArrayObject>()) {
            vp->setNumber(obj->as<TypedArrayObject>().length());
            return true;
        }
    }

    return NativeGetPureInline(obj2, shape, vp);
}

static bool
MOZ_ALWAYS_INLINE
GetElementPure(ThreadSafeContext *cx, JSObject *obj, uint32_t index, Value *vp)
{
    if (index <= JSID_INT_MAX)
        return GetPropertyPure(cx, obj, INT_TO_JSID(index), vp);
    return false;
}






bool
js::GetObjectElementOperationPure(ThreadSafeContext *cx, JSObject *obj, const Value &prop,
                                  Value *vp)
{
    uint32_t index;
    if (IsDefinitelyIndex(prop, &index))
        return GetElementPure(cx, obj, index, vp);

    
    if (!prop.isString() || !prop.toString()->isAtom())
        return false;

    JSAtom *name = &prop.toString()->asAtom();
    if (name->isIndex(&index))
        return GetElementPure(cx, obj, index, vp);

    return GetPropertyPure(cx, obj, NameToId(name->asPropertyName()), vp);
}

bool
baseops::GetElement(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
                    MutableHandleValue vp)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;

    
    return GetPropertyHelperInline<CanGC>(cx, obj, receiver, id, vp);
}

static bool
MaybeReportUndeclaredVarAssignment(JSContext *cx, JSString *propname)
{
    {
        JSScript *script = cx->currentScript(nullptr, JSContext::ALLOW_CROSS_COMPARTMENT);
        if (!script)
            return true;

        
        
        if (!script->strict() && !cx->options().extraWarnings())
            return true;
    }

    JSAutoByteString bytes(cx, propname);
    return !!bytes &&
           JS_ReportErrorFlagsAndNumber(cx,
                                        (JSREPORT_WARNING | JSREPORT_STRICT
                                         | JSREPORT_STRICT_MODE_ERROR),
                                        js_GetErrorMessage, nullptr,
                                        JSMSG_UNDECLARED_VAR, bytes.ptr());
}

bool
js::ReportIfUndeclaredVarAssignment(JSContext *cx, HandleString propname)
{
    {
        jsbytecode *pc;
        JSScript *script = cx->currentScript(&pc, JSContext::ALLOW_CROSS_COMPARTMENT);
        if (!script)
            return true;

        
        
        if (!script->strict() && !cx->options().extraWarnings())
            return true;

        













        MOZ_ASSERT(*pc != JSOP_NAME);
        MOZ_ASSERT(*pc != JSOP_GETGNAME);
        if (*pc != JSOP_SETNAME && *pc != JSOP_SETGNAME)
            return true;
    }

    JSAutoByteString bytes(cx, propname);
    return !!bytes &&
           JS_ReportErrorFlagsAndNumber(cx,
                                        JSREPORT_WARNING | JSREPORT_STRICT |
                                        JSREPORT_STRICT_MODE_ERROR,
                                        js_GetErrorMessage, nullptr,
                                        JSMSG_UNDECLARED_VAR, bytes.ptr());
}

bool
JSObject::reportReadOnly(ThreadSafeContext *cxArg, jsid id, unsigned report)
{
    if (cxArg->isForkJoinContext())
        return cxArg->asForkJoinContext()->reportError(ParallelBailoutUnsupportedVM, report);

    if (!cxArg->isJSContext())
        return true;

    JSContext *cx = cxArg->asJSContext();
    RootedValue val(cx, IdToValue(id));
    return js_ReportValueErrorFlags(cx, report, JSMSG_READ_ONLY,
                                    JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                    nullptr, nullptr);
}

bool
JSObject::reportNotConfigurable(ThreadSafeContext *cxArg, jsid id, unsigned report)
{
    if (cxArg->isForkJoinContext())
        return cxArg->asForkJoinContext()->reportError(ParallelBailoutUnsupportedVM, report);

    if (!cxArg->isJSContext())
        return true;

    JSContext *cx = cxArg->asJSContext();
    RootedValue val(cx, IdToValue(id));
    return js_ReportValueErrorFlags(cx, report, JSMSG_CANT_DELETE,
                                    JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                    nullptr, nullptr);
}

bool
JSObject::reportNotExtensible(ThreadSafeContext *cxArg, unsigned report)
{
    if (cxArg->isForkJoinContext())
        return cxArg->asForkJoinContext()->reportError(ParallelBailoutUnsupportedVM, report);

    if (!cxArg->isJSContext())
        return true;

    JSContext *cx = cxArg->asJSContext();
    RootedValue val(cx, ObjectValue(*this));
    return js_ReportValueErrorFlags(cx, report, JSMSG_OBJECT_NOT_EXTENSIBLE,
                                    JSDVG_IGNORE_STACK, val, js::NullPtr(),
                                    nullptr, nullptr);
}

bool
JSObject::callMethod(JSContext *cx, HandleId id, unsigned argc, Value *argv, MutableHandleValue vp)
{
    RootedValue fval(cx);
    RootedObject obj(cx, this);
    if (!JSObject::getGeneric(cx, obj, obj, id, &fval))
        return false;
    return Invoke(cx, ObjectValue(*obj), fval, argc, argv, vp);
}

template <ExecutionMode mode>
bool
baseops::SetPropertyHelper(typename ExecutionModeTraits<mode>::ContextType cxArg,
                           HandleObject obj, HandleObject receiver, HandleId id,
                           QualifiedBool qualified, MutableHandleValue vp, bool strict)
{
    JS_ASSERT(cxArg->isThreadLocal(obj));

    if (MOZ_UNLIKELY(obj->watched())) {
        if (mode == ParallelExecution)
            return false;

        
        JSContext *cx = cxArg->asJSContext();
        WatchpointMap *wpmap = cx->compartment()->watchpointMap;
        if (wpmap && !wpmap->triggerWatchpoint(cx, obj, id, vp))
            return false;
    }

    RootedObject pobj(cxArg);
    RootedShape shape(cxArg);
    if (mode == ParallelExecution) {
        if (!LookupPropertyPure(obj, id, pobj.address(), shape.address()))
            return false;
    } else {
        JSContext *cx = cxArg->asJSContext();
        if (!LookupNativeProperty(cx, obj, id, &pobj, &shape))
            return false;
    }
    if (shape) {
        if (!pobj->isNative()) {
            if (pobj->is<ProxyObject>()) {
                if (mode == ParallelExecution)
                    return false;

                JSContext *cx = cxArg->asJSContext();
                Rooted<PropertyDescriptor> pd(cx);
                if (!Proxy::getPropertyDescriptor(cx, pobj, id, &pd))
                    return false;

                if ((pd.attributes() & (JSPROP_SHARED | JSPROP_SHADOWABLE)) == JSPROP_SHARED) {
                    return !pd.setter() ||
                           CallSetter(cx, receiver, id, pd.setter(), pd.attributes(), strict, vp);
                }

                if (pd.isReadonly()) {
                    if (strict)
                        return JSObject::reportReadOnly(cx, id, JSREPORT_ERROR);
                    if (cx->options().extraWarnings())
                        return JSObject::reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                    return true;
                }
            }

            shape = nullptr;
        }
    } else {
        
        JS_ASSERT(!obj->is<BlockObject>());

        if (obj->is<GlobalObject>() && !qualified) {
            if (mode == ParallelExecution)
                return false;

            if (!MaybeReportUndeclaredVarAssignment(cxArg->asJSContext(), JSID_TO_STRING(id)))
                return false;
        }
    }

    



    unsigned attrs = JSPROP_ENUMERATE;
    const Class *clasp = obj->getClass();
    PropertyOp getter = clasp->getProperty;
    StrictPropertyOp setter = clasp->setProperty;

    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        
        if (pobj != obj)
            shape = nullptr;
    } else if (shape) {
        
        if (shape->isAccessorDescriptor()) {
            if (shape->hasDefaultSetter()) {
                
                if (mode == ParallelExecution)
                    return !strict;

                return js_ReportGetterOnlyAssignment(cxArg->asJSContext(), strict);
            }
        } else {
            JS_ASSERT(shape->isDataDescriptor());

            if (!shape->writable()) {
                





                if (mode == ParallelExecution)
                    return !strict;

                JSContext *cx = cxArg->asJSContext();
                if (strict)
                    return JSObject::reportReadOnly(cx, id, JSREPORT_ERROR);
                if (cx->options().extraWarnings())
                    return JSObject::reportReadOnly(cx, id, JSREPORT_STRICT | JSREPORT_WARNING);
                return true;
            }
        }

        attrs = shape->attributes();
        if (pobj != obj) {
            


            if (!shape->shadowable()) {
                if (shape->hasDefaultSetter() && !shape->hasGetterValue())
                    return true;

                if (mode == ParallelExecution)
                    return false;

                return shape->set(cxArg->asJSContext(), obj, receiver, strict, vp);
            }

            








            if (!shape->hasSlot()) {
                attrs &= ~JSPROP_SHARED;
                getter = shape->getter();
                setter = shape->setter();
            } else {
                
                attrs = JSPROP_ENUMERATE;
            }

            



            shape = nullptr;
        }
    }

    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        uint32_t index = JSID_TO_INT(id);

        if (obj->is<TypedArrayObject>()) {
            double d;
            if (mode == ParallelExecution) {
                
                
                if (vp.isObject())
                    return false;
                if (!NonObjectToNumber(cxArg, vp, &d))
                    return false;
            } else {
                if (!ToNumber(cxArg->asJSContext(), vp, &d))
                    return false;
            }

            
            
            
            TypedArrayObject &tarray = obj->as<TypedArrayObject>();
            if (index < tarray.length())
                TypedArrayObject::setElement(tarray, index, d);
            return true;
        }

        bool definesPast;
        if (!WouldDefinePastNonwritableLength(cxArg, obj, index, strict, &definesPast))
            return false;
        if (definesPast) {
            
            if (mode == ParallelExecution)
                return !strict;
            return true;
        }

        if (mode == ParallelExecution)
            return obj->setDenseElementIfHasType(index, vp);

        obj->setDenseElementWithType(cxArg->asJSContext(), index, vp);
        return true;
    }

    if (obj->is<ArrayObject>() && id == NameToId(cxArg->names().length)) {
        Rooted<ArrayObject*> arr(cxArg, &obj->as<ArrayObject>());
        return ArraySetLength<mode>(cxArg, arr, id, attrs, vp, strict);
    }

    if (!shape) {
        bool extensible;
        if (mode == ParallelExecution) {
            if (obj->is<ProxyObject>())
                return false;
            extensible = obj->nonProxyIsExtensible();
        } else {
            if (!JSObject::isExtensible(cxArg->asJSContext(), obj, &extensible))
                return false;
        }

        if (!extensible) {
            
            if (strict)
                return obj->reportNotExtensible(cxArg);
            if (mode == SequentialExecution && cxArg->asJSContext()->options().extraWarnings())
                return obj->reportNotExtensible(cxArg, JSREPORT_STRICT | JSREPORT_WARNING);
            return true;
        }

        if (mode == ParallelExecution) {
            if (obj->isDelegate())
                return false;

            if (getter != JS_PropertyStub || !HasTypePropertyId(obj, id, vp))
                return false;
        } else {
            JSContext *cx = cxArg->asJSContext();

            
            if (!PurgeScopeChain(cx, obj, id))
                return false;
        }

        return DefinePropertyOrElement<mode>(cxArg, obj, id, getter, setter,
                                             attrs, vp, true, strict);
    }

    return NativeSet<mode>(cxArg, obj, receiver, shape, strict, vp);
}

template bool
baseops::SetPropertyHelper<SequentialExecution>(JSContext *cx, HandleObject obj,
                                                HandleObject receiver, HandleId id,
                                                QualifiedBool qualified,
                                                MutableHandleValue vp, bool strict);
template bool
baseops::SetPropertyHelper<ParallelExecution>(ForkJoinContext *cx, HandleObject obj,
                                              HandleObject receiver, HandleId id,
                                              QualifiedBool qualified,
                                              MutableHandleValue vp, bool strict);

bool
baseops::SetElementHelper(JSContext *cx, HandleObject obj, HandleObject receiver, uint32_t index,
                          MutableHandleValue vp, bool strict)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return baseops::SetPropertyHelper<SequentialExecution>(cx, obj, receiver, id, Qualified, vp,
                                                           strict);
}

bool
baseops::GetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject nobj(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &nobj, &shape))
        return false;
    if (!shape) {
        *attrsp = 0;
        return true;
    }
    if (!nobj->isNative())
        return JSObject::getGenericAttributes(cx, nobj, id, attrsp);

    *attrsp = GetShapeAttributes(nobj, shape);
    return true;
}

bool
baseops::SetAttributes(JSContext *cx, HandleObject obj, HandleId id, unsigned *attrsp)
{
    RootedObject nobj(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &nobj, &shape))
        return false;
    if (!shape)
        return true;
    if (nobj->isNative() && IsImplicitDenseOrTypedArrayElement(shape)) {
        if (nobj->is<TypedArrayObject>()) {
            if (*attrsp == (JSPROP_ENUMERATE | JSPROP_PERMANENT))
                return true;
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_SET_ARRAY_ATTRS);
            return false;
        }
        if (!JSObject::sparsifyDenseElement(cx, nobj, JSID_TO_INT(id)))
            return false;
        shape = obj->nativeLookup(cx, id);
    }
    if (nobj->isNative()) {
        if (!JSObject::changePropertyAttributes(cx, nobj, shape, *attrsp))
            return false;
        if (*attrsp & JSPROP_READONLY)
            MarkTypePropertyNonWritable(cx, obj, id);
        return true;
    } else {
        return JSObject::setGenericAttributes(cx, nobj, id, attrsp);
    }
}

bool
baseops::DeleteGeneric(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded)
{
    RootedObject proto(cx);
    RootedShape shape(cx);
    if (!baseops::LookupProperty<CanGC>(cx, obj, id, &proto, &shape))
        return false;
    if (!shape || proto != obj) {
        



        return CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, id, succeeded);
    }

    cx->runtime()->gc.poke();

    if (IsImplicitDenseOrTypedArrayElement(shape)) {
        if (obj->is<TypedArrayObject>()) {
            
            *succeeded = false;
            return true;
        }

        if (!CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, id, succeeded))
            return false;
        if (!succeeded)
            return true;

        obj->setDenseElementHole(cx, JSID_TO_INT(id));
        return js_SuppressDeletedProperty(cx, obj, id);
    }

    if (!shape->configurable()) {
        *succeeded = false;
        return true;
    }

    RootedId propid(cx, shape->propid());
    if (!CallJSDeletePropertyOp(cx, obj->getClass()->delProperty, obj, propid, succeeded))
        return false;
    if (!succeeded)
        return true;

    return obj->removeProperty(cx, id) && js_SuppressDeletedProperty(cx, obj, id);
}

bool
js::WatchGuts(JSContext *cx, JS::HandleObject origObj, JS::HandleId id, JS::HandleObject callable)
{
    RootedObject obj(cx, GetInnerObject(origObj));
    if (obj->isNative()) {
        
        
        if (!JSObject::sparsifyDenseElements(cx, obj))
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
baseops::Watch(JSContext *cx, JS::HandleObject obj, JS::HandleId id, JS::HandleObject callable)
{
    if (!obj->isNative() || obj->is<TypedArrayObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_WATCH,
                             obj->getClass()->name);
        return false;
    }

    return WatchGuts(cx, obj, id, callable);
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
baseops::Unwatch(JSContext *cx, JS::HandleObject obj, JS::HandleId id)
{
    return UnwatchGuts(cx, obj, id);
}

bool
js::HasDataProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    if (JSID_IS_INT(id) && obj->containsDenseElement(JSID_TO_INT(id))) {
        *vp = obj->getDenseElement(JSID_TO_INT(id));
        return true;
    }

    if (Shape *shape = obj->nativeLookup(cx, id)) {
        if (shape->hasDefaultGetter() && shape->hasSlot()) {
            *vp = obj->nativeGetSlot(shape->slot());
            return true;
        }
    }

    return false;
}









static bool
MaybeCallMethod(JSContext *cx, HandleObject obj, HandleId id, MutableHandleValue vp)
{
    if (!JSObject::getGeneric(cx, obj, obj, id, vp))
        return false;
    if (!IsCallable(vp)) {
        vp.setObject(*obj);
        return true;
    }
    return Invoke(cx, ObjectValue(*obj), vp, 0, nullptr, vp);
}

JS_FRIEND_API(bool)
js::DefaultValue(JSContext *cx, HandleObject obj, JSType hint, MutableHandleValue vp)
{
    JS_ASSERT(hint == JSTYPE_NUMBER || hint == JSTYPE_STRING || hint == JSTYPE_VOID);

    Rooted<jsid> id(cx);

    const Class *clasp = obj->getClass();
    if (hint == JSTYPE_STRING) {
        id = NameToId(cx->names().toString);

        
        if (clasp == &StringObject::class_) {
            if (ClassMethodIsNative(cx, obj, &StringObject::class_, id, js_str_toString)) {
                vp.setString(obj->as<StringObject>().unbox());
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
            if (ClassMethodIsNative(cx, obj, &StringObject::class_, id, js_str_toString)) {
                vp.setString(obj->as<StringObject>().unbox());
                return true;
            }
        }

        
        if (clasp == &NumberObject::class_) {
            id = NameToId(cx->names().valueOf);
            if (ClassMethodIsNative(cx, obj, &NumberObject::class_, id, js_num_valueOf)) {
                vp.setNumber(obj->as<NumberObject>().unbox());
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
                         (hint == JSTYPE_VOID) ? "primitive type" : TypeStrings[hint]);
    return false;
}

JS_FRIEND_API(bool)
JS_EnumerateState(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                  MutableHandleValue statep, JS::MutableHandleId idp)
{
    
    const Class *clasp = obj->getClass();
    JSEnumerateOp enumerate = clasp->enumerate;
    if (clasp->flags & JSCLASS_NEW_ENUMERATE) {
        JS_ASSERT(enumerate != JS_EnumerateStub);
        return ((JSNewEnumerateOp) enumerate)(cx, obj, enum_op, statep, idp);
    }

    if (!enumerate(cx, obj))
        return false;

    
    JS_ASSERT(enum_op == JSENUMERATE_INIT || enum_op == JSENUMERATE_INIT_ALL);
    statep.setMagic(JS_NATIVE_ENUMERATE);
    return true;
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
        if (!JSObject::getProto(cx, obj2, &obj2))
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
    JS_ASSERT(JSProto_Null <= protoKey);
    JS_ASSERT(protoKey < JSProto_LIMIT);

    if (protoKey != JSProto_Null) {
        const Value &v = global->getPrototype(protoKey);
        if (v.isObject())
            return &v.toObject();
    }

    return nullptr;
}





bool
js::FindClassPrototype(ExclusiveContext *cx, MutableHandleObject protop, const Class *clasp)
{
    protop.set(nullptr);
    JSProtoKey protoKey = GetClassProtoKey(clasp);
    if (protoKey != JSProto_Null)
        return GetBuiltinPrototype(cx, protoKey, protop);

    RootedObject ctor(cx);
    if (!FindClassObject(cx, &ctor, clasp))
        return false;

    if (ctor && ctor->is<JSFunction>()) {
        RootedValue v(cx);
        if (cx->isJSContext()) {
            if (!JSObject::getProperty(cx->asJSContext(),
                                       ctor, ctor, cx->names().prototype, &v))
            {
                return false;
            }
        } else {
            Shape *shape = ctor->nativeLookup(cx, cx->names().prototype);
            if (!shape || !NativeGetPureInline(ctor, shape, v.address()))
                return false;
        }
        if (v.isObject())
            protop.set(&v.toObject());
    }
    return true;
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

    JS_ASSERT(v.isBoolean());
    return BooleanObject::create(cx, v.toBoolean());
}


JSObject *
js::ToObjectSlow(JSContext *cx, HandleValue val, bool reportScanStack)
{
    JS_ASSERT(!val.isMagic());
    JS_ASSERT(!val.isObject());

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
    JS_ASSERT(trc->debugPrinter() == js_GetObjectSlotName);

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
        const char *slotname = nullptr;
        if (obj->is<GlobalObject>()) {
#define TEST_SLOT_MATCHES_PROTOTYPE(name,code,init,clasp)                     \
            if ((code) == slot) { slotname = js_##name##_str; goto found; }
            JS_FOR_EACH_PROTOTYPE(TEST_SLOT_MATCHES_PROTOTYPE)
#undef TEST_SLOT_MATCHES_PROTOTYPE
        }
      found:
        if (slotname)
            JS_snprintf(buf, bufsize, "CLASS_OBJECT(%s)", slotname);
        else
            JS_snprintf(buf, bufsize, "**UNKNOWN SLOT %ld**", (long)slot);
    } else {
        jsid propid = shape->propid();
        if (JSID_IS_INT(propid)) {
            JS_snprintf(buf, bufsize, "%ld", (long)JSID_TO_INT(propid));
        } else if (JSID_IS_ATOM(propid)) {
            PutEscapedString(buf, bufsize, JSID_TO_ATOM(propid), 0);
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

JS_FRIEND_API(bool)
js_GetterOnlyPropertyStub(JSContext *cx, HandleObject obj, HandleId id, bool strict,
                          MutableHandleValue vp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_GETTER_ONLY);
    return false;
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
                (clasp == &JSObject::class_) ? "" : " object",
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
          case JS_NATIVE_ENUMERATE:  fprintf(stderr, " native enumeration"); break;
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
DumpProperty(JSObject *obj, Shape &shape)
{
    jsid id = shape.propid();
    uint8_t attrs = shape.attributes();

    fprintf(stderr, "    ((JSShape *) %p) ", (void *) &shape);
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

    if (JSID_IS_ATOM(id))
        JSID_TO_STRING(id)->dump();
    else if (JSID_IS_INT(id))
        fprintf(stderr, "%d", (int) JSID_TO_INT(id));
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
    if (obj->isVarObj()) fprintf(stderr, " varobj");
    if (obj->watched()) fprintf(stderr, " watched");
    if (obj->isIteratedSingleton()) fprintf(stderr, " iterated_singleton");
    if (obj->isNewTypeUnknown()) fprintf(stderr, " new_type_unknown");
    if (obj->hasUncacheableProto()) fprintf(stderr, " has_uncacheable_proto");
    if (obj->hadElementsAccess()) fprintf(stderr, " had_elements_access");

    if (obj->isNative()) {
        if (obj->inDictionaryMode())
            fprintf(stderr, " inDictionaryMode");
        if (obj->hasShapeTable())
            fprintf(stderr, " hasShapeTable");
    }
    fprintf(stderr, "\n");

    if (obj->isNative()) {
        uint32_t slots = obj->getDenseInitializedLength();
        if (slots) {
            fprintf(stderr, "elements\n");
            for (uint32_t i = 0; i < slots; i++) {
                fprintf(stderr, " %3d: ", i);
                dumpValue(obj->getDenseElement(i));
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
        fprintf(stderr, "private %p\n", obj->getPrivate());

    if (!obj->isNative())
        fprintf(stderr, "not native\n");

    uint32_t reservedEnd = JSCLASS_RESERVED_SLOTS(clasp);
    uint32_t slots = obj->slotSpan();
    uint32_t stop = obj->isNative() ? reservedEnd : slots;
    if (stop > 0)
        fprintf(stderr, obj->isNative() ? "reserved slots:\n" : "slots:\n");
    for (uint32_t i = 0; i < stop; i++) {
        fprintf(stderr, " %3d ", i);
        if (i < reservedEnd)
            fprintf(stderr, "(reserved) ");
        fprintf(stderr, "= ");
        dumpValue(obj->getSlot(i));
        fputc('\n', stderr);
    }

    if (obj->isNative()) {
        fprintf(stderr, "properties:\n");
        Vector<Shape *, 8, SystemAllocPolicy> props;
        for (Shape::Range<NoGC> r(obj->lastProperty()); !r.empty(); r.popFront())
            props.append(&r.front());
        for (size_t i = props.length(); i-- != 0;)
            DumpProperty(obj, *props[i]);
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
            dumpValue(i.calleev());
        } else {
            fprintf(stderr, "global frame, no callee");
        }
        fputc('\n', stderr);

        fprintf(stderr, "file %s line %u\n",
                i.script()->filename(), (unsigned) i.script()->lineno());

        if (jsbytecode *pc = i.pc()) {
            fprintf(stderr, "  pc = %p\n", pc);
            fprintf(stderr, "  current op: %s\n", js_CodeName[*pc]);
            MaybeDumpObject("staticScope", i.script()->getStaticScope(pc));
        }
        MaybeDumpValue("this", i.thisv());
        if (!i.isJit()) {
            fprintf(stderr, "  rval: ");
            dumpValue(i.interpFrame()->returnValue());
            fputc('\n', stderr);
        }

        fprintf(stderr, "  flags:");
        if (i.isConstructing())
            fprintf(stderr, " constructing");
        if (!i.isJit() && i.interpFrame()->isDebuggerFrame())
            fprintf(stderr, " debugger");
        if (i.isEvalFrame())
            fprintf(stderr, " eval");
        if (!i.isJit() && i.interpFrame()->isYielding())
            fprintf(stderr, " yielding");
        if (!i.isJit() && i.interpFrame()->isGeneratorFrame())
            fprintf(stderr, " generator");
        fputc('\n', stderr);

        fprintf(stderr, "  scopeChain: (JSObject *) %p\n", (void *) i.scopeChain());

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
    for (ScriptFrameIter i(cx); !i.done(); ++i, ++depth) {
        const char *filename = JS_GetScriptFilename(i.script());
        unsigned line = JS_PCToLineNumber(cx, i.script(), i.pc());
        JSScript *script = i.script();
        sprinter.printf("#%d %14p   %s:%d (%p @ %d)\n",
                        depth, (i.isJit() ? 0 : i.interpFrame()), filename, line,
                        script, script->pcToOffset(i.pc()));
    }
    fprintf(stdout, "%s", sprinter.string());
}

void
JSObject::addSizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf, JS::ObjectsExtraSizes *sizes)
{
    if (hasDynamicSlots())
        sizes->mallocHeapSlots += mallocSizeOf(slots);

    if (hasDynamicElements()) {
        js::ObjectElements *elements = getElementsHeader();
        sizes->mallocHeapElementsNonAsmJS += mallocSizeOf(elements);
    }

    
    if (is<JSFunction>() ||
        is<JSObject>() ||
        is<ArrayObject>() ||
        is<CallObject>() ||
        is<RegExpObject>() ||
        is<ProxyObject>())
    {
        
        
        
        
        
        
        
        
        

    } else if (is<ArgumentsObject>()) {
        sizes->mallocHeapArgumentsData += as<ArgumentsObject>().sizeOfMisc(mallocSizeOf);
    } else if (is<RegExpStaticsObject>()) {
        sizes->mallocHeapRegExpStatics += as<RegExpStaticsObject>().sizeOfData(mallocSizeOf);
    } else if (is<PropertyIteratorObject>()) {
        sizes->mallocHeapPropertyIteratorData += as<PropertyIteratorObject>().sizeOfMisc(mallocSizeOf);
    } else if (is<ArrayBufferObject>() || is<SharedArrayBufferObject>()) {
        ArrayBufferObject::addSizeOfExcludingThis(this, mallocSizeOf, sizes);
#ifdef JS_ION
    } else if (is<AsmJSModuleObject>()) {
        as<AsmJSModuleObject>().addSizeOfMisc(mallocSizeOf, &sizes->nonHeapCodeAsmJS,
                                              &sizes->mallocHeapAsmJSModuleData);
#endif
#ifdef JS_HAS_CTYPES
    } else {
        
        sizes->mallocHeapCtypesData +=
            js::SizeOfDataIfCDataObject(mallocSizeOf, const_cast<JSObject *>(this));
#endif
    }
}
